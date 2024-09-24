#ifndef __ASYNC_H__
#define __ASYNC_H__

#include <time.h>

#include "thread_pool.h"
#include "list_generic.h"

#define ASYNC_WORK_SUCCESS 0
#define ASYNC_WORK_FAIL   1
#define ASYNC_WORK_TIMEOUT 2

typedef struct async_work_s async_work_t;

typedef int (*async_work_func_t)(void *);

struct async_work_s {
    char *name;
    time_t deadline;
    char is_timeout;
    char ref;
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


#endif
