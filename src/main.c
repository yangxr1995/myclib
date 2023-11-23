#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "memchk.h"
#include "assert.h"
#include "mm_pool.h"
#include "fmt.h"
#include "debug.h"
#include "logger.h"
#include "timer_list.h"
#include "thread_pool.h"


void test(void *arg)
{
	int i = (int)arg;

	sleep(1);
	printf("tid %d : %d\n", pthread_self(), i);
	sleep(1);
}

void thread_block_sig(void *arg)
{
	sigset_t *set;

	set = arg;

	printf("%d set sigmask\n", pthread_self());
	pthread_sigmask(SIG_BLOCK, set, NULL);
}

void do_alrm(int sig)
{
	printf("%d recv SIGALRM\n", pthread_self());
}

int main(int argc, const char *argv[])
{
	threadpool_t tp;
	sigset_t set;

	printf("main thread : %d\n", pthread_self());

	signal(SIGALRM, do_alrm);

	struct itimerval {
		struct timeval it_interval; /* Interval for periodic timer */
		struct timeval it_value;    /* Time until next expiration */
	};

	struct timeval {
		time_t      tv_sec;         /* seconds */
		suseconds_t tv_usec;        /* microseconds */
	};

	struct itimerval itval;

	itval.it_value.tv_usec = 0;
	itval.it_value.tv_sec = 3;

	itval.it_interval.tv_usec = 0;
	itval.it_interval.tv_sec = 3;

	setitimer(ITIMER_REAL, &itval, NULL);

		sigemptyset(&set);
	//	sigaddset(&set, SIGALRM);
	//	sigaddset(&set, SIGPIPE);

	tp = threadpool_new(3, 20, thread_block_sig, &set);
	if (tp == NULL) {
		printf("threadpool_new failed : %s", 
				strerror(errno));
		return -1;
	}

	int i;
	for (i = 0; i < 15; i++) {
		threadpool_append(tp, test, (void *)i);
	}

	int ret = 20;

	while ((ret = sleep(ret)) > 0) NULL;


	threadpool_delete(tp);

	mem_leak();

	return 0;
}
