#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "assert.h"
#include "timer_wheel.h"
#include "list_generic.h"

#define TIMER_WHEEL_TEST 0

#define USE_SEC_INTERVAL  0
#define USE_MSEC_INTERVAL 1

#if USE_MSEC_INTERVAL & USE_SEC_INTERVAL
#error "USE_MSEC_INTERVAL and USE_SEC_INTERVAL can not be both 1"
#endif

#define TW_DEBUG 1

#if TW_DEBUG == 0
#define debug(fmt, ...) NULL
#else
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

static timer_wheel_t *tw;

union align {
    int i;
    long l;
    long *lp;
    void (*fp)(void);
    float d;
    double ld;
};

inline static unsigned int align_up(unsigned int size, unsigned int align)
{
    return (size + align - 1) / align * align;
}

inline static void deep_sleep(unsigned int sec)
{
    while (sec) {
        sec = sleep(sec);
    }
}

/*
 * 构建时间轮。
 * 该时间轮转动solt_nb次为一圈,
 * 每tick_interval间隔cur_ts加1
 */
int
timer_wheel_init(unsigned int solt_nb, unsigned int tick_interval)
{
    unsigned int sz;

    sz = align_up(sizeof(*tw), sizeof(union align));
    tw = malloc(sz + sizeof(*tw->solts) * solt_nb);
    tw->solts = (timer_solt_t *)((char *)tw + sz);
    tw->solt_nb = solt_nb;
    tw->tick_interval = tick_interval;
    atomic_init(&tw->cur_ts, 0);
    for (int i = 0; i < tw->solt_nb; ++i) {
        pthread_spin_init(&tw->solts[i].lock, PTHREAD_PROCESS_PRIVATE);
        list_head_init(&tw->solts[i].nodes);
    }

    return 0;
}

timer_wheel_node_t *
__timer_wheel_add(timer_wheel_node_t *node)
{
    timer_wheel_node_t *pos;
    int idx;

    node->tigger_ts = atomic_load(&tw->cur_ts) + node->cycle;
    idx = node->tigger_ts % tw->solt_nb;
    node->solt = tw->solts + idx;
    pthread_spin_lock(&node->solt->lock);
    list_for_each_entry(pos, &node->solt->nodes, list) {
        if (pos->tigger_ts >= node->tigger_ts)
            break;
    }
    list_add_tail(&node->list, &pos->list);
    pthread_spin_unlock(&node->solt->lock);

    return node;
}

timer_wheel_node_t *
timer_wheel_add(unsigned int cycle, bool is_repeat, timer_wheel_call_t call, void *cb)
{
    timer_wheel_node_t *node;

    node = malloc(sizeof(*node));
    node->cycle = cycle;
    node->is_repeat = is_repeat;
    node->call = call;
    node->cb = cb;
    atomic_store(&node->is_deleted, false);
    list_head_init(&node->list);

    return __timer_wheel_add(node);
}

void 
timer_wheel_tick()
{
    atomic_fetch_add(&tw->cur_ts, 1);

    long long cur_ts;
    int idx;

    cur_ts = atomic_load(&tw->cur_ts);

    debug("%s : cur_ts[%lld]\n", __func__, cur_ts);

    idx = cur_ts % tw->solt_nb;

    timer_solt_t *solt;
    timer_wheel_node_t *pos, *tmp;
    list_head_t tigger_head;

    list_head_init(&tigger_head);

    solt = tw->solts + idx;
    pthread_spin_lock(&solt->lock);
    list_for_each_entry_safe(pos, tmp, &solt->nodes, list) {

        if (atomic_load(&pos->is_deleted)) {
            list_head_del(&pos->list);
            debug("free by delete\n");
            free(pos);
            continue;
        }

        if (pos->tigger_ts > cur_ts)
            break;

        list_head_del(&pos->list);
        list_head_add(&pos->list, &tigger_head);
    }
    pthread_spin_unlock(&solt->lock);

    list_for_each_entry_safe(pos, tmp, &tigger_head, list) {
        list_head_del(&pos->list);
        pos->call(pos->cb);
        if (pos->is_repeat)
            __timer_wheel_add(pos);
        else {
            free(pos);
            debug("free by timeout\n");
        }
    }
}

/*
 * timer_wheel_node_t的释放由tw模块管理 ,
 * 外部模块只能给tw模块建议，
 * 这导致了危险指针，当tw模块释放了node,
 * 但外部模块无法获知，
 *
 * 为了避免危险指针导致的bug，调用者需要遵守如下约定，
 * 1. 当is_repeat为true时，调用者可以访问node，
 *    并调用timer_wheel_del以通知tw模块延迟释放node
 * 2. 当is_repeat为false时，调用者通常不应该考虑对node的访问
 */
int 
timer_wheel_del(timer_wheel_node_t *node)
{
    atomic_store(&node->is_deleted, true);

    return 0;
}

void 
timer_wheel_tick_sig(int sig)
{
    timer_wheel_tick();
}

int 
timer_wheel_start_by_timerfd()
{
    struct itimerspec val = {0};

    tw->tick_mode = tick_mode_timerfd;

    tw->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

#if USE_MSEC_INTERVAL
    int msec = tw->tick_interval;
    val.it_value.tv_sec = msec / 1000;
    val.it_value.tv_nsec = msec % 1000 * 1000 * 1000;
#endif

#if USE_SEC_INTERVAL
    val.it_value.tv_sec = tw->tick_interval;
    val.it_value.tv_nsec = 0;
#endif

    val.it_interval = val.it_value;

    timerfd_settime(tw->timerfd, 0, &val, NULL);

    return tw->timerfd;
}

/*
 * 不推荐将timer_wheel_tick交给信号处理函数，
 * 因为timer_wheel_tick会调用用户的回调，
 * 但是信号上下文有很多限制，并容易导致死锁，
 * 更好的策略是将信号转为IO事件，以在普通上下文
 * 调用timer_wheel_tick
 */
int 
timer_wheel_start_by_sig()
{
    struct sigaction act = {0};

    tw->tick_mode = tick_mode_signal;

    act.sa_handler = timer_wheel_tick_sig;
    if (sigaction(SIGALRM, &act, NULL) < 0)
        return -1;

    struct itimerval val = {0};

#if USE_SEC_INTERVAL
    int sec = tw->tick_interval;
    val.it_value.tv_usec = 0;
    val.it_value.tv_sec = sec;
#endif

#if USE_MSEC_INTERVAL
    int msec = tw->tick_interval;
    val.it_value.tv_usec = msec % 1000 * 1000;
    val.it_value.tv_sec = msec / 1000;
#endif

    val.it_interval = val.it_value;

    if (setitimer(ITIMER_REAL, &val, NULL) < 0)
        return -1;

    return 0;
}

void 
timer_wheel_destroy()
{
    struct sigaction act = {0};
    struct itimerval val = {0};

    if (tw->tick_mode == tick_mode_signal) {
        signal(SIGALRM, SIG_DFL);
        setitimer(ITIMER_REAL, &val, NULL);
    }
    else if (tw->tick_mode == tick_mode_timerfd) {
        struct itimerspec val = {0};
        timerfd_settime(tw->timerfd, 0, &val, NULL);
    }

    debug("%s : cur_ts[%lld] solt_nb[%d]\n", __func__, tw->cur_ts, tw->solt_nb);

    for (int i = 0; i < tw->solt_nb; ++i) {
        timer_solt_t *solt = tw->solts + i;
        timer_wheel_node_t *pos, *tmp;
        list_for_each_entry_safe(pos , tmp, &solt->nodes, list) {
            debug("%s : is_repeat[%d] is_deleted[%d] tigger_ts[%lld]\n", 
                    __func__, pos->is_repeat, pos->is_deleted, pos->tigger_ts);
            free(pos);
        }
    }
    free(tw);
}

#if TIMER_WHEEL_TEST

#define WRITER_NB 20
#define SOLT_NB 10
#define TICK_INTERVAL 1000

typedef struct thread_ctx {
    pthread_t tid;
    int id;
    int sec;
} thread_ctx_t;

static void writer_call(void *arg)
{
    thread_ctx_t *ctx = (thread_ctx_t *)arg;
    debug("%s : id[%d] sec[%d]\n", __func__, ctx->id, ctx->sec);
}

static void *writer(void *arg)
{
    thread_ctx_t *ctx = (thread_ctx_t *)arg;

    timer_wheel_node_t *node;

    debug("%s : id[%d] sec[%d]\n", __func__, ctx->id, ctx->sec);
    node = timer_wheel_add(ctx->sec, true, writer_call, ctx);

    int to_sleep = 2 * ctx->sec + 3;
    deep_sleep(to_sleep);

    timer_wheel_del(node);

    debug("id[%d] exit\n", ctx->id);

    return NULL;
}

static int timer_wheel_get_fd()
{
    assert(tw->tick_mode == tick_mode_timerfd);
    return tw->timerfd;
}

static void *timer_tick(void *arg)
{
    int fd;

    fd = timer_wheel_start_by_timerfd();
    while (1) {
        char val[8];
        int cnt;
        cnt = read(fd, val, sizeof val); // 必须读8字节
        if (cnt == sizeof val) {
            timer_wheel_tick();
        }
    }

    return NULL;
}

int test_timer_wheel()
{
    thread_ctx_t ths[WRITER_NB];
    pthread_t th_timer;

    timer_wheel_init(SOLT_NB, TICK_INTERVAL);

    pthread_create(&th_timer, NULL, timer_tick, NULL);

    for (int i = 0; i < WRITER_NB; ++i) {
        ths[i].sec = rand() % 10 + 3;
        ths[i].id = i;
        pthread_create(&ths[i].tid, NULL, writer, &ths[i]);
    }

    for (int i = 0; i < WRITER_NB; ++i) {
        pthread_join(ths[i].tid, NULL);
    }

    deep_sleep(tw->solt_nb);

    timer_wheel_destroy();

    return 0;
}

#endif
