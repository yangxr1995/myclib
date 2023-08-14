#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "timer_list.h"

static pthread_t g_tid;
static pthread_mutex_t g_list_lock = PTHREAD_MUTEX_INITIALIZER;
static timer_list_t *g_head;

static void *timer_routine(void *arg)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	return NULL;
}

inline static timer_list_t *timer_list_alloc(unsigned int msec,
		int repeat, timer_call_t call, void *cb)
{
	timer_list_t *n;

	if ((n = malloc(sizeof(timer_list_t))) == NULL) {
		perror("malloc");
		exit(1);
	}
	n->sec = msec/1000;
	n->msec = msec%1000;
	n->repeat = repeat;
	n->call = call;
	n->cb = cb;
	n->next = NULL;
	gettimeofday(&n->ts, NULL);

	return n;
}

inline static int 
timer_cmp(timer_list_t *t1, timer_list_t *t2)
{
	unsigned int t1_sec, t1_usec;
	unsigned int t2_sec, t2_usec;

	t1_sec = t1->ts.tv_sec + t1->sec;
	t1_usec = t1->ts.tv_usec + t1->msec * 1000;

	t2_sec = t2->ts.tv_sec + t2->sec;
	t2_usec = t2->ts.tv_usec + t2->msec * 1000;

	if (t1_sec < t2_sec ||
		(t1_sec == t2_sec && 
		t1_usec < t2_usec))
		return -1;

	if (t1_sec == t2_sec && t1_usec == t2_usec)
		return 0;
	
	return 1;
}

static void 
__timer_list_add(timer_list_t *new)
{
	timer_list_t **p;	

	for (p = &g_head; *p; p = &(*p)->next) {
		if (timer_cmp(new, *p) <= 0)
			break;
	}
	new->next = *p;	
	*p = new;
}

static void 
timer_list_add(timer_list_t *new)
{
	pthread_mutex_lock(&g_list_lock);
	__timer_list_add(new);
	pthread_mutex_unlock(&g_list_lock);
}

int 
timer_list_create(unsigned int msec, int repeat, 
		timer_call_t call, void *cb)
{
	sigset_t set;
	if (g_tid == 0) {
		sigemptyset(&set);
		sigaddset(&set, SIGALRM);
		pthread_sigmask(SIG_BLOCK, &set, NULL);
		if (pthread_create(&g_tid, NULL, timer_routine, NULL) < 0) {
			perror("pthread_create : ");
			return -1;
		}
	}

	timer_list_t *new;
	new = timer_list_alloc(msec, repeat, call, cb);
	timer_list_add(new);

	return 0;
}

