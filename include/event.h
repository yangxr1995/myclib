#ifndef __EVENT_H
#define __EVENT_H


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
};


#endif
