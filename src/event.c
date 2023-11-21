#include <sys/epoll.h>
#include <assert.h>

#include "event.h"

#define MAX_EVENT 10

int
event_ctx_init(event_ctx_t *ctx)
{
	if ((ctx->efd = epoll_create(0)) < 0)
		return -1; 
	
	return 0;
}

int
event_add(event_ctx_t *ctx, event_t *ev)
{
	struct epoll_event epoll_ev = {0};

	epoll_ev.data.ptr = ev;
	epoll_ev.events = ev->events;
	
	if (epoll_ctl(ctx->efd, EPOLL_CTL_ADD, ev->fd, &epoll_ev) < 0)
		return -1;

	return 0;	
}


int
event_loop(event_ctx_t *ctx)
{
	struct epoll_event evs[MAX_EVENT];
	event_t *ev;
	int i, n;

	if ((n = epoll_wait(ctx->efd, evs, MAX_EVENT, -1)) < 0) 
		return -1;
	for (i = 0; i < n; i++) {
		if (evs[i].events & EPOLLIN) {
			ev = evs[i].data.ptr;
			assert(ev->read);
			ev->read(ev);
		}
	}

	return 0;
}
