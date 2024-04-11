#ifndef __EVENT_H
#define __EVENT_H

#include <sys/epoll.h>

typedef struct event_s event_t;
typedef struct event_ctx_s event_ctx_t;

struct event_ctx_s {
	int efd;
};

struct event_s {
	int fd;	
	char *name;
	unsigned int events;
	int (*read)(event_t *ev);
	void *pri;
};

int event_ctx_init(event_ctx_t *ctx);
int event_add(event_ctx_t *ctx, event_t *ev);
int event_del(event_ctx_t *ctx, event_t *ev);
int event_loop(event_ctx_t *ctx);

#define event_init(_ev, _fd, _events, _name, _read, _pri)   \
do { \
	event_t *__ev = _ev; \
	__ev->fd = _fd; \
	__ev->events = _events; \
	__ev->name = _name; \
	__ev->read = _read; \
	__ev->pri = _pri; \
} while (0)

#endif
