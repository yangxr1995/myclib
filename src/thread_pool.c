#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "list_head.h"
#include "logger.h"
#include "thread_pool.h"
//#include "memchk.h"

typedef struct workqueue_s workqueue_t;

struct workqueue_s {
	process_t process;
	void *arg;
	list_head_t list;
};

struct threadpool_s {
	int thread_number;
	int max_requests;
	list_head_t requests;
	sem_t requests_sem;
	pthread_mutex_t requests_locker;
	char stop;
	void (*init_worker)(void *arg);
	void *init_worker_arg;
	pthread_t threads[0];
};

int 
threadpool_append(threadpool_t tp, process_t process, void *arg)
{
	int val;
	workqueue_t *work;

	pthread_mutex_lock(&tp->requests_locker);		
	sem_getvalue(&tp->requests_sem, &val);
	if (val >= tp->max_requests) {
		log_message(WARN_LOG, "%s : thread pool is full !",
				__func__);
		pthread_mutex_unlock(&tp->requests_locker);		
		return -1;
	}
	work = malloc(sizeof(*work));
	work->process = process;
	work->arg = arg;
	INIT_LIST_HEAD(&work->list);
	list_add_tail(&work->list, &tp->requests);
	sem_post(&tp->requests_sem);
	pthread_mutex_unlock(&tp->requests_locker);		

	return 0;
}

void
threadpool_delete(threadpool_t tp)
{
	tp->stop = 1;
	sem_destroy(&tp->requests_sem);
	pthread_mutex_destroy(&tp->requests_locker);
	free(tp);
}

static void *threadpool_worker(void *arg)
{
	threadpool_t tp;
	workqueue_t *work;

	tp = (threadpool_t )arg;

	if (tp->init_worker)
		tp->init_worker(tp->init_worker_arg);

	while (tp->stop == 0) {

		if (sem_wait(&tp->requests_sem) < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			log_message(ERR_LOG, "sem-wait error : %s\n", 
					strerror(errno));
			assert(0);
		}

		pthread_mutex_lock(&tp->requests_locker);
		if (list_empty(&tp->requests)) {
			log_message(ERR_LOG, "requests error\n");
					strerror(errno);
			assert(0);
			pthread_mutex_unlock(&tp->requests_locker);
			continue;
		}

		work = list_entry(tp->requests.next, workqueue_t, list);	
		list_del_init(tp->requests.next);

		pthread_mutex_unlock(&tp->requests_locker);

		work->process(work->arg);
		free(work);
	}

	return NULL;	
}

threadpool_t threadpool_new(unsigned int tp_num, 
		unsigned int max_requests,
		void (*init_worker)(void *arg),
		void *init_worker_arg)
{
	int i, ret;
	threadpool_t tp = NULL;

	assert(tp_num > 0);
	assert(max_requests > 0);

	tp = malloc(sizeof(*tp) + sizeof(pthread_t) * tp_num);

	memset(tp, 0x0, sizeof(*tp));
	tp->thread_number = tp_num;
	tp->max_requests = max_requests;
	sem_init(&tp->requests_sem, 1, 0);
	pthread_mutex_init(&tp->requests_locker, NULL);
	INIT_LIST_HEAD(&tp->requests);
	tp->stop = 0; 
	tp->init_worker = init_worker;
	tp->init_worker_arg = init_worker_arg;

	for (i = 0; i < tp_num; i++) {
		ret = pthread_create(tp->threads + i, NULL, threadpool_worker, tp);
		if (ret != 0)
			return NULL;

		ret = pthread_detach(tp->threads[i]);
		if (ret != 0)
			return NULL;
	}

	return tp;
}

