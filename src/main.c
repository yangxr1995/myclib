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

int main(int argc, const char *argv[])
{
	threadpool_t tp;

	tp = threadpool_new(3, 20);
	if (tp == NULL) {
		printf("threadpool_new failed : %s", 
				strerror(errno));
		return -1;
	}

	int i;
	for (i = 0; i < 15; i++) {
		threadpool_append(tp, test, (void *)i);
	}

	sleep(20);
	threadpool_exit(tp);
	
	mem_leak();

	return 0;
}
