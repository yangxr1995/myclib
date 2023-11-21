#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H


typedef struct threadpool_s *threadpool_t;
typedef void (*process_t) (void *);

int threadpool_append(threadpool_t tp, process_t process, void *arg);
void threadpool_delete(threadpool_t tp);
threadpool_t threadpool_new(unsigned int tp_num, unsigned int max_requests, void (*init_worker)(void *), void *init_worker_arg);

#endif
