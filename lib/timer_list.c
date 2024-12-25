#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "logger.h"
#include "timer_list.h"

static timer_list_t *g_head;
static char is_init;
static pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;

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
	n->discard = 0;
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
__timer_list_add(timer_list_t **phead, timer_list_t *new)
{
	timer_list_t **p;	

	for (p = phead; *p; p = &(*p)->next) {
		if (timer_cmp(new, *p) <= 0)
			break;
	}
	new->next = *p;	
	*p = new;
}

static void 
timer_list_add(timer_list_t *new)
{
	pthread_mutex_lock(&locker);
	__timer_list_add(&g_head, new);
	pthread_mutex_unlock(&locker);
}

timer_list_t *
timer_list_create(unsigned int msec, int repeat, 
		timer_call_t call, void *cb)
{
	timer_list_t *new;
	log_message(DEBUG_LOG, "%s cb %p", __func__, cb);
	new = timer_list_alloc(msec, repeat, call, cb);
	timer_list_add(new);

	return new;
}

int timer_cmp_tv(timer_list_t *timer, struct timeval *val)
{
	unsigned int sec, msec, msec2;

	sec = timer->ts.tv_sec + timer->sec;
	if (sec == val->tv_usec) {
		msec = timer->ts.tv_usec / 1000 + timer->msec;
		msec2 = val->tv_usec / 1000;
		if (msec == msec2)
			return 0;
		return msec > msec2 ? 1 : -1;
	}
	return sec > val->tv_sec ? 1 : -1;
}

void timer_list_tick(int sig)
{
	struct timeval cur;	
	timer_list_t *timeout_list = NULL, **ppos, *tmp;

	gettimeofday(&cur, NULL);

	if (pthread_mutex_trylock(&locker) < 0) {
		log_message(DEBUG_LOG, "%s try locker failed, return",
				__func__);
		return;
	}

	if (g_head == NULL) {
		log_message(DEBUG_LOG, "%s g_head == NULL",
				__func__);
	}

	for (ppos = &g_head; *ppos; ) {

		if ((*ppos)->discard ||
				timer_cmp_tv(*ppos, &cur) <= 0) {
			tmp = *ppos;
			*ppos = (*ppos)->next;
			__timer_list_add(&timeout_list, tmp);
		}
		else {
			ppos = &(*ppos)->next;
		}
	}

	pthread_mutex_unlock(&locker);	

	for (ppos = &timeout_list; *ppos; ppos = &(*ppos)->next) {
		if (!(*ppos)->discard)
			(*ppos)->call((*ppos)->cb);	
		else if ((*ppos)->discard && (*ppos)->exec_on_discard)
			(*ppos)->call((*ppos)->cb);	
	}

	for (ppos = &timeout_list; *ppos; ) {
		tmp = *ppos;
		*ppos = (*ppos)->next;

		if (tmp->discard == 0 && tmp->repeat) {
			tmp->ts = cur;
			timer_list_add(tmp);
		}
		else {
			free(tmp);
		}

	}

}

int 
timer_list_start(unsigned int msec)
{
	struct sigaction act = {0};

	assert(msec >= 0);
	
	if (is_init == 0) {
		is_init = 1;	
		act.sa_handler = timer_list_tick;
		if (sigaction(SIGALRM, &act, NULL) < 0)
			return -1;
	}

	struct itimerval val = {0};
	val.it_value.tv_usec = msec % 1000 * 1000;
	val.it_value.tv_sec = msec / 1000;
	val.it_interval = val.it_value;

	if (setitimer(ITIMER_REAL, &val, NULL) < 0)
		return -1;
	
	return 0;
}

int
timer_list_discard_target(void *target, char exec_on_discard)
{
	log_message(DEBUG_LOG, "%s begin", __func__);

	timer_list_t *pos;
	pthread_mutex_lock(&locker);
	for (pos = g_head; pos; pos = pos->next) {
		if (pos->cb == target) {
			log_message(DEBUG_LOG, "%s find target %p", 
					__func__, pos);
			pos->discard = 1;
			pos->exec_on_discard = exec_on_discard;
		}
		else {
			log_message(DEBUG_LOG, "%s cb %p != %p", 
					__func__, target, pos->cb);
		}
	}
	pthread_mutex_unlock(&locker);

	log_message(DEBUG_LOG, "%s end", __func__);
	return 0;
}
