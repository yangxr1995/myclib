#ifndef __TIMER_LIST_H__
#define __TIMER_LIST_H__

#include <sys/time.h>

typedef void (*timer_call_t)(void *);

typedef struct timer_list_s timer_list_t;
struct timer_list_s {
	timer_call_t call;
	void *cb;
	int repeat;
	timer_list_t *next;
	unsigned int sec;    // 剩余等待的时间
	unsigned int msec;   // 毫秒
	struct timeval ts;   // 时间戳指向创建节点的时间
	char discard;
};

timer_list_t *timer_list_create(unsigned int msec, int repeat, timer_call_t call, void *cb);
int timer_list_start(unsigned int msec);
int timer_list_discard_target(void *target);

#endif
