#pragma once

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <setjmp.h>

#include "thread_pool.h"
#include "list_generic.h"

#define ASYNC_WORK_SUCCESS 0
#define ASYNC_WORK_FAIL   1
#define ASYNC_WORK_TIMEOUT 2

typedef struct async_work_s async_work_t;

typedef int (*async_work_func_t)(void *);

struct async_work_s {
    char *name;
    _Atomic time_t deadline;
    _Atomic bool is_timeout;
    _Atomic unsigned char ref;
    _Atomic pthread_t tid;
    _Atomic bool is_set_jmp;
    jmp_buf jmp_env;
    list_head_t list;
    async_work_func_t work;
    async_work_func_t work_success_hook;
    async_work_func_t work_fail_hook;
    async_work_func_t work_timeout_hook;
    async_work_func_t work_free_hook;
    void *pri;
};

int async_work_init();
int async_work_assign(char *name, int max_ts, async_work_func_t work,
        async_work_func_t wtimeout, 
        async_work_func_t wsuccess, 
        async_work_func_t wfail, 
        async_work_func_t wfree,
        void *data, threadpool_t tp);


