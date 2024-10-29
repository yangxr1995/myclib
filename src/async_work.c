#include <stdatomic.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/syslog.h>
#include <time.h>

#include "event.h"
#include "async_work.h"
#include "list_generic.h"
#include "logger.h"
#include "thread_pool.h"
#include "timer_wheel.h"
#include "arr.h"
#include "com_msg.h"
#include "event.h"

int pthread_sigqueue(pthread_t thread, int sig,
                    const union sigval value);

#define AW_DEBUG 1

#if AW_DEBUG == 0
#ifdef log_debug
#undef log_debug
#endif
#define log_debug(fmt, ...) NULL
#else

#endif

#define AW_TEST 1


typedef struct async_work_ctx_s async_work_ctx_t;
typedef struct async_work_cmsg_s async_work_cmsg_t;

struct async_work_ctx_s {
    list_head_t list;
    _Atomic time_t ts;
    pthread_spinlock_t lock;
};

struct async_work_cmsg_s {
    int result;
    int id;
    cmsg_ctx_t *cmsg;
    void *pri;
};

static async_work_ctx_t g_async_ctx;

inline static void
async_work_free(async_work_t *aw)
{
    log_debug("free");
    free(aw->name);
    free(aw);
}

inline static void
async_work_get(async_work_t *aw)
{
    atomic_fetch_add(&aw->ref, 1);
}

inline static void
async_work_append(async_work_t *aw)
{
    async_work_t *insert_pos;

    async_work_get(aw);
    pthread_spin_lock(&g_async_ctx.lock);
    list_for_each_entry(insert_pos, &g_async_ctx.list , list) {
        if (atomic_load(&insert_pos->deadline) > atomic_load(&aw->deadline))
            break;
    }
    list_add_tail(&aw->list, &insert_pos->list);
    pthread_spin_unlock(&g_async_ctx.lock);
}

inline static void
async_work_put(async_work_t *aw)
{
    if (atomic_fetch_sub(&aw->ref, 1) == 1) {
        if (aw->work_free_hook)
            aw->work_free_hook(aw->pri);
        async_work_free(aw);
    }
}

static void 
break_timeout_work(int signo, siginfo_t *info, void *arg)
{
    async_work_t *aw = info->si_ptr;

    log_debug("break work : %ld", aw->tid);

    if (atomic_load(&aw->is_set_jmp))
        longjmp(aw->jmp_env , 1);
    else
        async_work_put(aw);
}

static void
async_work_timeout_check(void *arg)
{
    async_work_t *pos, *n, **ptimeout_pos;
    arr_t *timeout_aw_arr = arr_create(10 , sizeof(async_work_t *));

    atomic_fetch_add(&g_async_ctx.ts, 1);

    pthread_spin_lock(&g_async_ctx.lock);
    list_for_each_entry_safe(pos , n , &g_async_ctx.list, list) {
        if (atomic_load(&g_async_ctx.ts) >= atomic_load(&pos->deadline)) {
            atomic_store(&pos->is_timeout, true);
            list_head_del(&pos->list);
            ptimeout_pos = arr_push(timeout_aw_arr);
            *ptimeout_pos = pos;
        }
        else {
            break;
        }
    }
    pthread_spin_unlock(&g_async_ctx.lock);

    arr_for_each(timeout_aw_arr, ptimeout_pos) {
        pos = *ptimeout_pos;

        async_work_get(pos);
        union sigval sigval;
        sigval.sival_ptr = pos;
        pthread_sigqueue(pos->tid, SIGUSR1, sigval);

        pos->work_timeout_hook(pos->pri);

        async_work_put(pos); // for async_work_ctx_t.list
    }
    arr_destroy(timeout_aw_arr);
}

int
async_work_init()
{
    INIT_LIST_HEAD(&g_async_ctx.list);
    pthread_spin_init(&g_async_ctx.lock, 0);
    atomic_init(&g_async_ctx.ts, 0);
    timer_wheel_add(1, 1, async_work_timeout_check , NULL);

    /*占用SIGUSR1，用于中断已超时的工作*/
    struct sigaction sa;
    sa.sa_sigaction = break_timeout_work;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    return 0;
}

inline static void
async_work_del_from_timeout_list(async_work_t *aw)
{
    pthread_spin_lock(&g_async_ctx.lock);
    list_head_del(&aw->list);
    pthread_spin_unlock(&g_async_ctx.lock);
    async_work_put(aw); // for async_work_ctx_t.list
}

static void
async_work_work(void *arg)
{
    async_work_t *aw = (async_work_t *)arg;
    int ret;

    atomic_store(&aw->tid, pthread_self());

    if (setjmp(aw->jmp_env) == 0) {
        log_debug("do work");
        atomic_store(&aw->is_set_jmp, true);
        ret = aw->work(aw->pri);
        atomic_store(&aw->is_set_jmp, false);
    }
    else {
        log_debug("jump back");
        async_work_put(aw); // for break_timeout_work
        // FIXME:
        // work若分配了资源，如何回收?
    }

    log_debug("tid: %ld", aw->tid);

    if (!atomic_load(&aw->is_timeout)) {
        async_work_del_from_timeout_list(aw);
        if (ret == 0)
            aw->work_success_hook(aw->pri);
        else
            aw->work_fail_hook(aw->pri);
    }

    async_work_put(aw); // for thread_pool
}

int
async_work_assign(char *name, int max_ts, async_work_func_t work,
        async_work_func_t wtimeout, 
        async_work_func_t wsuccess, 
        async_work_func_t wfail, 
        async_work_func_t wfree,
        void *data, threadpool_t tp)
{
    async_work_t *aw;

    aw = malloc(sizeof(*aw));
    memset(aw, 0x0, sizeof(*aw));

    aw->name = strdup(name);
    atomic_store(&aw->deadline, atomic_load(&g_async_ctx.ts) + max_ts);
    INIT_LIST_HEAD(&aw->list);
    aw->work = work;
    aw->work_fail_hook = wfail;
    aw->work_success_hook = wsuccess;
    aw->work_timeout_hook = wtimeout;
    aw->work_free_hook = wfree;
    aw->pri = data;
    atomic_store(&aw->tid, 0);
    atomic_store(&aw->is_set_jmp, false);
    async_work_append(aw);

    async_work_get(aw);
    if (threadpool_append(tp, async_work_work , aw) < 0) {
        log_err("threadpool_append");
        return -1;
    }

    return 0;
}

static int 
async_work_timeout_cmsg(void *arg)
{
    async_work_cmsg_t *awcmsg = (async_work_cmsg_t *)arg;

    awcmsg->result = ASYNC_WORK_TIMEOUT;
    cmsg_send_ptr(awcmsg->cmsg, awcmsg->id, awcmsg);
    return 0;
}

static int 
async_work_success_cmsg(void *arg)
{
    async_work_cmsg_t *awcmsg = (async_work_cmsg_t *)arg;

    awcmsg->result = ASYNC_WORK_SUCCESS;
    cmsg_send_ptr(awcmsg->cmsg, awcmsg->id, awcmsg);
    return 0;
}

static int 
async_work_fail_cmsg(void *arg)
{
    async_work_cmsg_t *awcmsg = (async_work_cmsg_t *)arg;
    awcmsg->result = ASYNC_WORK_FAIL;
    cmsg_send_ptr(awcmsg->cmsg, awcmsg->id, awcmsg);
    return 0;
}

int 
async_work_assign_cmsg(char *name, int max_ts, async_work_func_t work,
        int id, cmsg_ctx_t *cmsg, void *pri, threadpool_t tp)
{
    async_work_cmsg_t *awcmsg = malloc(sizeof(*awcmsg));

    awcmsg->result = 0;
    awcmsg->id = id;
    awcmsg->pri = pri;
    awcmsg->cmsg = cmsg;

    log_debug("name[%s] awcmsg : %p", name, awcmsg);

    return async_work_assign(name, max_ts, work, 
            async_work_timeout_cmsg, async_work_success_cmsg, 
            async_work_fail_cmsg, NULL, 
            awcmsg , tp);
}

#if AW_TEST

int test1(void *arg)
{
    int cnt = 6;
    log_info("%s %d", __func__, __LINE__);
    while (cnt) {
        cnt = sleep(cnt);
    }
    log_info("%s %d", __func__, __LINE__);
    return 0;
}

int test1_fail(void *arg)
{
    log_info("%s %d", __func__, __LINE__);
    return 0;
}

int test1_free(void *arg)
{
    log_info("%s %d", __func__, __LINE__);
    return 0;
}

int test1_timeout(void *arg)
{
    log_info("%s %d", __func__, __LINE__);
    return 0;
}

int test1_success(void *arg)
{
    log_info("%s %d", __func__, __LINE__);
    return 0;
}

enum {
    cmsg_connect,
    cmsg_connect2,
};

int test2_do_connect2(void *arg)
{
    async_work_cmsg_t *awcmsg = (async_work_cmsg_t *)arg;
    void *client_ctx = awcmsg->pri;

    // connect
    sleep(8);

    return 0;
    /*return -1;*/
}

int test2_do_connect(void *arg)
{
    async_work_cmsg_t *awcmsg = (async_work_cmsg_t *)arg;
    void *client_ctx = awcmsg->pri;

    // connect
    sleep(4);

    return 0;
    /*return -1;*/
}

int cmsg_connect_deal_req2(char *data, cmsg_t *msg)
{
    async_work_cmsg_t *awcmsg = *(async_work_cmsg_t **)data;

    if (awcmsg->result == ASYNC_WORK_FAIL) {
        log_info("fail");
    } 
    else if (awcmsg->result == ASYNC_WORK_SUCCESS) {
        log_info("success");
    }
    else if (awcmsg->result == ASYNC_WORK_TIMEOUT) {
        log_info("timeout");
    }

    free(awcmsg);

    return 0;
}

int cmsg_connect_deal_req(char *data, cmsg_t *msg)
{
    async_work_cmsg_t *awcmsg = *(async_work_cmsg_t **)data;

    if (awcmsg->result == ASYNC_WORK_FAIL) {
        log_info("fail");
    } 
    else if (awcmsg->result == ASYNC_WORK_SUCCESS) {
        log_info("success");
    }
    else if (awcmsg->result == ASYNC_WORK_TIMEOUT) {
        log_info("timeout");
    }

    free(awcmsg);

    return 0;
}

int test_aw(int argc, char **argv)
{
    threadpool_t tp;
    event_ctx_t ev_ctx;

    enable_console_log();
    set_max_log_level(DEBUG_LOG);

    timer_wheel_init(10, 1000);
    tp = threadpool_new(4, 10 , NULL , NULL);
    async_work_init();
    event_ctx_init(&ev_ctx);

    cmsg_ctx_t *cmsg_ctx = cmsg_ctx_new(cmsg_inner, NULL, &ev_ctx);
    cmsg_register(cmsg_ctx, NULL, "cmsg_connect", cmsg_connect, cmsg_connect_deal_req, NULL);
    cmsg_register(cmsg_ctx, NULL, "cmsg_connect2", cmsg_connect2, cmsg_connect_deal_req2, NULL);
    void *client_ctx = NULL;

    async_work_assign_cmsg("test cmsg2", 2, test2_do_connect2 , 
            cmsg_connect2, cmsg_ctx, client_ctx, tp);

    async_work_assign_cmsg("test cmsg", 3, test2_do_connect , 
            cmsg_connect, cmsg_ctx, client_ctx, tp);

    timer_wheel_start_by_timerfd_ev(&ev_ctx);

    while (1) {
        event_loop(&ev_ctx);
    }

    return 0;
}

#endif
