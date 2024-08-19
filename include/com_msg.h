#ifndef __COM_MSG_H__
#define __COM_MSG_H__

#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "event.h"
#include "arr.h"
#include "logger.h"

typedef struct cmsg_data_s cmsg_data_t;
struct cmsg_data_s {
	unsigned int id;
	unsigned int len;
	void *data;
};

typedef struct cmsg_s cmsg_t;
typedef struct cmsg_ctx_s cmsg_ctx_t;

struct cmsg_s {
	unsigned int id;
	const char *name;
    cmsg_ctx_t *ctx;
	void *pri;
	int (*build_req)(void *org_data, void **pout, unsigned int *len);
	int (*deal_req)(char *data, cmsg_t *);

	int (*build_resp)(void *org_data, void **pout, unsigned int *len);
	int (*deal_resp)(char *data, cmsg_t *);
};

enum cmsg_type {
    cmsg_inner,
    cmsg_server,
    cmsg_client,
};
typedef enum cmsg_type cmsg_type_t;

struct cmsg_ctx_s {
    int rfd;
    int wfd;
    cmsg_type_t type;
    arr_t *msgs;
    event_t ev_read;
    pthread_mutex_t mutex;
    struct sockaddr_un peer;

    char *client_file;
};

cmsg_ctx_t *cmsg_ctx_new(cmsg_type_t type, char *server_file, event_ctx_t *ev_ctx);
int cmsg_recv_and_deal(cmsg_ctx_t *ctx);

inline static cmsg_t *cmsg_find(cmsg_ctx_t *ctx, unsigned int id)
{
	cmsg_t *pos;

	arr_for_each(ctx->msgs, pos) {
		if (pos->id == id)
			return pos;
	}

	return NULL;
}

inline static int cmsg_register(cmsg_ctx_t *ctx, void *pri,
        const char *name, unsigned int id, 
		int (*build_req)(void *, void **, unsigned int *),
		int (*deal_req)(char *, cmsg_t *),
        int (*build_resp)(void *org_data, void **pout, unsigned int *len),
        int (*deal_resp)(char *data, cmsg_t *))
{
	cmsg_t *msg;

	if (cmsg_find(ctx, id)) {
        log_err("cmsg_find id 重复[%d]", id);
		return -1;
    }

	msg = (cmsg_t *)arr_push(ctx->msgs);
	msg->name = strdup(name);
	msg->build_req = build_req;
	msg->deal_req = deal_req;
    msg->build_resp = build_resp;
    msg->deal_resp = deal_resp;
	msg->id = id;
    msg->pri = pri;
    msg->ctx = ctx;

	return 0;
}


inline static int cmsg_read_event(event_t *ev)
{
	cmsg_ctx_t *ctx = (cmsg_ctx_t *)ev->pri;
	return cmsg_recv_and_deal(ctx);
}

int cmsg_send(cmsg_ctx_t *ctx, unsigned int id, void *org_data);

#endif
