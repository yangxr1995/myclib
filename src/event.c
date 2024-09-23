#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include "event.h"
#include "logger.h"

#define MAX_EVENT 10

int
event_ctx_init(event_ctx_t *ctx)
{
	if ((ctx->efd = epoll_create(1)) < 0) {
        log_err("epoll_create1");
		return -1; 
	}
	
	return 0;
}

int
event_add(event_ctx_t *ctx, event_t *ev)
{
	struct epoll_event epoll_ev = {0};

	epoll_ev.data.ptr = ev;
	epoll_ev.events = ev->events;
	
	if (epoll_ctl(ctx->efd, EPOLL_CTL_ADD, ev->fd, &epoll_ev) < 0) {
        log_err("epoll_ctl EPOLL_CTL_ADD");
		return -1;
	}

	return 0;	
}

int
event_del(event_ctx_t *ctx, event_t *ev)
{
	if (epoll_ctl(ctx->efd, EPOLL_CTL_DEL, ev->fd, NULL) < 0) {
        log_err("epoll_ctl EPOLL_CTL_DEL");
		return -1;
	}

	return 0;	
}

int
event_loop(event_ctx_t *ctx)
{
	struct epoll_event evs[MAX_EVENT];
	event_t *ev;
	int i, n;

__again__:
	if ((n = epoll_wait(ctx->efd, evs, MAX_EVENT, -1)) < 0) {
		if (errno == EINTR) {
			errno = 0;
			goto __again__;
		}
        log_err("epoll_wait");
		return -1;
	}
	for (i = 0; i < n; i++) {
		if (evs[i].events & EPOLLIN) {
			ev = evs[i].data.ptr;
			assert(ev->read);
			ev->read(ev);
		}
	}

	return 0;
}
