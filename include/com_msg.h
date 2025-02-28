#pragma once

#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "event.h"
#include "arr.h"
#include "logger.h"
#include "buf.h"

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
	int (*deal_req)(char *data, cmsg_t *);
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

    buf_t *recv_cache;
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
		int (*deal_req)(char *, cmsg_t *),
        int (*deal_resp)(char *data, cmsg_t *))
{
	cmsg_t *msg;

	if (cmsg_find(ctx, id)) {
        log_err("cmsg_find id 重复[%d]", id);
		return -1;
    }

	msg = (cmsg_t *)arr_push(ctx->msgs);
	msg->name = strdup(name);
	msg->deal_req = deal_req;
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

int cmsg_send(cmsg_ctx_t *ctx, unsigned int id, void *data, int len);

inline static int 
cmsg_send_str(cmsg_ctx_t *ctx, unsigned int id, const char *str)
{
    return cmsg_send(ctx, id, (void *)str, strlen(str)+ 1);
}

inline static int 
cmsg_send_ptr(cmsg_ctx_t *ctx, unsigned int id, void *ptr)
{
    return cmsg_send(ctx, id, &ptr, sizeof(void *));
}

inline static int 
cmsg_send_null(cmsg_ctx_t *ctx, unsigned int id)
{
    return cmsg_send(ctx, id, NULL, 0);
}

#define cmsg_send_obj(_ctx, _id, _ptr) \
    cmsg_send(_ctx, _id, _ptr, sizeof(*_ptr));


