#include <alloca.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "buf.h"
#include "event.h"
#include "arr.h"
#include "logger.h"
#include "com_msg.h"
#include "assert.h"

struct stu {
	int age;
	char name[32];
};

static	int
open_listen_unix_socket(const char *path)
{
	int fd = -1, try = 3;
	struct sockaddr_un addr = {0};

    assert(path);

again:
	if (access(path, F_OK) == 0) {
		log_warn("'%s' is occupied", path);
		if (try--) {
			sleep(1);
			unlink(path);
			goto again;
		}
		goto open_listen_socket_err;
	}

	if ((fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		goto open_listen_socket_err;

	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, path);

	if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
		goto open_listen_socket_err;

	return fd;

open_listen_socket_err:
	if (fd >= 0)
		close(fd);

	return -1;
}

static	int
open_connect_unix_socket(const char *spath, const char *dpath)
{
	int fd = -1;
	struct sockaddr_un server_addr = {0};
	struct sockaddr_un client_addr = {0};

	if (access(dpath, F_OK) < 0) {
		log_message(ERR_LOG, "unix socket '%s' not exist\n",
				dpath);
		goto open_connect_socket_err;
	}

	if ((fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		goto open_connect_socket_err;

	client_addr.sun_family = AF_LOCAL;
	strcpy(client_addr.sun_path, spath);
	if (bind(fd, (struct sockaddr *)&client_addr,
				sizeof(client_addr)) < 0)
		goto open_connect_socket_err;

	server_addr.sun_family = AF_LOCAL;
	strcpy(server_addr.sun_path, dpath);

	if (connect(fd, (struct sockaddr *)&server_addr,
				sizeof(server_addr)) < 0)
		goto open_connect_socket_err;

	return fd;

open_connect_socket_err:
	if (fd >= 0)
		close(fd);
	return -1;
}

int cmsg_send(cmsg_ctx_t *ctx, unsigned int id, void *data, int len)
{
	cmsg_t *pos;
	int find = 0;
    char *buf;
    int buf_len;

    buf_len = sizeof(id) + sizeof(len) + len;
    buf = alloca(buf_len);
    memcpy(buf, &id, sizeof(id));
    memcpy(buf + sizeof(id), &len, sizeof(len));
    memcpy(buf + sizeof(id) + sizeof(len), data, len);

	if ((pos = cmsg_find(ctx, id)) == NULL) {
		log_err("com_msg_find id[%d] 不存在", id);
		goto err;
	}

    int cnt;
    if (ctx->type == cmsg_server) {
        if (cnt = sendto(ctx->wfd, buf, buf_len, 0, (struct sockaddr *)&ctx->peer, sizeof(ctx->peer)) < 0) {
            log_err("sendto");
            goto err;
        } 
    }
    else {
        if ((cnt = write(ctx->wfd, buf, buf_len)) < 0) {
            log_err("write id");
            goto err;
        }
    }
    log_debug("send bytes[%d]", cnt);

	return 0;	

err:

	return -1;
}


static int cmsg_data_get_from_buf(buf_t *pbuf, cmsg_data_t *pdata)
{
    cmsg_data_t data;
    buf_t buf;

    buf = *pbuf;

    if (buf_raw_data_len(&buf) < (sizeof(data.id) + sizeof(data.len))) {
        log_debug("buf raw data Insufficient: buf_raw_data_len(%d) < (%lu)", 
                buf_raw_data_len(&buf), sizeof(data.id) + sizeof(data.len));
        goto ret_fail;
    }

    data.id = *(typeof(data.id) *)(buf.pos);
    buf_data_push(&buf, sizeof(data.id));
    data.len = *(typeof(data.len) *)(buf.pos);
    buf_data_push(&buf, sizeof(data.len));

    if (data.len == 0)
        goto ret_ok;

    if (buf_raw_data_len(&buf) < data.len) {
        log_debug("buf raw data Insufficient: buf_raw_data_len(%d) < data.len(%d)", 
                buf_raw_data_len(&buf), data.len);
        goto ret_fail;
    }

    data.data = malloc(data.len);
    if (data.data == NULL) {
        log_err("malloc [%d] buf failed ", data.len);
        goto ret_err;
    }

    memcpy(data.data, buf.pos, data.len);
    buf_data_push(&buf, data.len);

    goto ret_ok;

ret_ok:
    *pdata = data;
    *pbuf = buf;

    return 1;

ret_err:
    return -1;

ret_fail:
    return 0;
}

inline static int cmsg_recv(cmsg_ctx_t *ctx)
{
    int cnt;
    struct sockaddr_un addr = {0};
    socklen_t addr_len = sizeof(addr);

    if (ctx->type == cmsg_server) {
        if ((cnt = recvfrom(ctx->rfd, ctx->recv_cache->tail, 
                        buf_room_len(ctx->recv_cache), 0, (struct sockaddr *)&addr, &addr_len)) < 0) {
            goto err;
            log_err("recvfrom");
        }
    }
    else {
        if ((cnt = read(ctx->rfd, ctx->recv_cache->tail, buf_room_len(ctx->recv_cache))) < 0) {
            log_err("read");
            goto err;
        }
    }

    log_debug("recv bytes[%d]", cnt);

    buf_cache_push(ctx->recv_cache, cnt);
    ctx->peer = addr;

    return cnt;
err:
    return -1;
}

static int cmsg_deal(cmsg_data_t *data, cmsg_ctx_t *ctx)
{
    cmsg_t *msg;

    if ((msg = cmsg_find(ctx, data->id)) == NULL) {
        log_err("com_msg_find [%d]", data->id);
        goto err;
    }

    if (ctx->type == cmsg_inner || ctx->type == cmsg_server) {
        if (msg->deal_req(data->data, msg) < 0) {
            log_err("msg->deal");
            goto err;
        }
    }
    else if (ctx->type == cmsg_client) {
        if (msg->deal_resp(data->data, msg) < 0) {
            log_err("msg->deal");
            goto err;
        }
    }

    return 0;
err:
    return -1;
}

int cmsg_recv_and_deal(cmsg_ctx_t *ctx)
{
    cmsg_data_t data;
    int ret;

    do {

        if (cmsg_recv(ctx) < 0) {
            log_err("cmsg_recv");
            goto err;
        }

        while (1) {
            ret = cmsg_data_get_from_buf(ctx->recv_cache, &data);
            if (ret < 0) {
                log_err("cmsg_data_get_from_buf");
                goto err;
            }

            if (ret == 0)
                break;

            if (cmsg_deal(&data, ctx) < 0) {
                log_err("cmsg_deal");
                goto err;
            }
        }

        buf_move(ctx->recv_cache);

    } while (buf_raw_data_len(ctx->recv_cache) > 0);


	return 0;	
err:
	return -1;
}

int hello_cmsg_build_req(void *org_data, void **pout, unsigned int *len)
{
	void *out;
	char *str = (char *)org_data;

    log_info("--");

	out = strdup(str);
	*pout = out;
	*len = strlen(out);

	return 0;
}

int stu_cmsg_build_resp(void *org_data, void **pout, unsigned int *len)
{
	void *out;
	struct stu * stu = org_data;

	out = malloc(sizeof(*stu));
	memcpy(out, stu, sizeof(*stu));
	*len = sizeof(*stu);

	*pout = out;

	return 0;
}



int hello_cmsg_build_resp(void *org_data, void **pout, unsigned int *len)
{
	void *out;
	char *str = (char *)org_data;

    log_info("--");

	out = strdup(str);
	*pout = out;
	*len = strlen(out);

	return 0;
}

int hello_com_msg_build(void *org_data, void **pout, unsigned int *len)
{
	void *out;
	char *str = (char *)org_data;

	out = strdup(str);
	*pout = out;
	*len = strlen(out);

	return 0;
}

int null_cmsg_deal_resp(char *data, cmsg_t *msg)
{
    log_info("null resp");
    return 0;
}

int stu_cmsg_deal_resp(char *data, cmsg_t *msg)
{
	struct stu *stu = (struct stu *)data;

	log_info("age[%d] name[%s]", stu->age, stu->name);


	return 0;
}



int hello_cmsg_deal_resp(char *data, cmsg_t *msg)
{
	char *str = data;

	log_info("%s", str);

	return 0;
}

int hello_cmsg_deal_req(char *data, cmsg_t *msg)
{
	char *str = data;

	log_info("%s", str);

    /*cmsg_send_str(msg->ctx, 0, "world");*/

	return 0;
}

int hello_com_msg_deal(char *data, cmsg_t *msg)
{
	char *str = data;

    log_info("--");
	log_info("%s", str);

	return 0;
}

int stu_cmsg_build_req(void *org_data, void **pout, unsigned int *len)
{
	void *out;
	struct stu * stu = org_data;

	out = malloc(sizeof(*stu));
	memcpy(out, stu, sizeof(*stu));
	*len = sizeof(*stu);

	*pout = out;

	return 0;
}

int stu_com_msg_build(void *org_data, void **pout, unsigned int *len)
{
	void *out;
	struct stu * stu = org_data;

	out = malloc(sizeof(*stu));
	memcpy(out, stu, sizeof(*stu));
	*len = sizeof(*stu);

	*pout = out;

	return 0;
}

int null_msg_deal(char *data, cmsg_t *msg)
{
    log_info("recv null msg");
    return 0;
}

int null_cmsg_deal_req(char *data, cmsg_t *msg)
{
    log_info("null req");
    /*cmsg_send_null(msg->ctx, 3);*/

    return 0;
}

int stu_cmsg_deal_req(char *data, cmsg_t *msg)
{
	struct stu *stu = (struct stu *)data;

	log_info("age[%d] name[%s]", stu->age, stu->name);
    stu->age = 100;
    strcpy(stu->name, "bbbbb");

    cmsg_send_obj(msg->ctx, 1, stu);

	return 0;
}

int stu_com_msg_deal(char *data, cmsg_t *msg)
{
	struct stu *stu = (struct stu *)data;

	log_info("age[%d] name[%s]", stu->age, stu->name);

	return 0;
}

static inline void
get_prg_name(char *buf, size_t buf_sz, pid_t pid)
{
	char file[128];
	int fd, cnt;

	sprintf(file, "/proc/%d/comm", pid);
	fd = open(file, O_RDONLY);
	cnt = read(fd, buf, buf_sz - 1);
	close(fd);
	buf[cnt-1] = '\0';
}

inline static char *
temp_file_un()
{
    pid_t pid;
    char buf[64] = {0};
    char filename[256] = {0};

    pid = getpid();
    get_prg_name(buf, sizeof(buf), pid);
    snprintf(filename, sizeof(filename) - 1, "/var/run/%s-%d.un", buf, pid);

    return strdup(filename);
}

cmsg_ctx_t *
cmsg_ctx_new(cmsg_type_t type, char *server_file, event_ctx_t *ev_ctx)
{
	cmsg_ctx_t *ctx = (cmsg_ctx_t *)malloc(sizeof(cmsg_ctx_t));
	struct sockaddr_un server_address;

    assert(ev_ctx);

	memset(ctx, 0, sizeof(*ctx));

    if (type == cmsg_inner) {
        int fds[2];
        if (pipe(fds) < 0)
            return NULL;
        ctx->rfd = fds[0];
        ctx->wfd = fds[1];
    }
    else if (type == cmsg_server) {
        if ((ctx->rfd = open_listen_unix_socket(server_file)) < 0) {
            log_err("open_listen_unix_socket [%s]", server_file);
            goto err;
        }
        ctx->wfd = ctx->rfd;
    }
    else if (type == cmsg_client) {
        ctx->client_file = temp_file_un();
        if ((ctx->rfd = open_connect_unix_socket(ctx->client_file, server_file)) < 0) {
            log_err("open_connect_unix_socket [%s]", server_file);
            goto err;
        }
        log_debug("connect ok");
        ctx->wfd = ctx->rfd;
    }

    ctx->type = type;
	ctx->msgs = arr_new(10, sizeof(cmsg_t));

    ctx->ev_read.fd = ctx->rfd;
	ctx->ev_read.pri = ctx;
	ctx->ev_read.read = cmsg_read_event;
	ctx->ev_read.name = (char *) "com_msg read";
	ctx->ev_read.events = EPOLLIN;

    ctx->recv_cache = buf_new(1024);

	event_add(ev_ctx, &ctx->ev_read);

    pthread_mutex_init(&ctx->mutex, NULL);

	return ctx;
err:
	return NULL;
}

int do_cmsg_register(cmsg_ctx_t *ctx)
{

    if (cmsg_register(ctx, NULL, "hello", 0, 
                hello_cmsg_deal_req, hello_cmsg_deal_resp) < 0) {
        log_err("com_msg_register");
        return -1;
    }

    if (cmsg_register(ctx, NULL, "stu", 1, 
                stu_cmsg_deal_req, stu_cmsg_deal_resp) < 0) {
        log_err("com_msg_register");
        return -1;
    }

    if (cmsg_register(ctx, NULL, "null", 3, 
                null_cmsg_deal_req, null_cmsg_deal_resp) < 0) {
        log_err("com_msg_register");
        return -1;
    }


    return 0;
}

void test_cmsg_fork()
{
	event_ctx_t ev_ctx;
    cmsg_ctx_t *ctx;

    enable_console_log();
	set_max_log_level(INFO_LOG);

	event_ctx_init(&ev_ctx);

    char *server_file = "/aaa1.un";

    pid_t pid;
    pid = fork();
    if (pid > 0) {

        if ((ctx = cmsg_ctx_new(cmsg_server, server_file, &ev_ctx)) == NULL) {
            log_err("cmsg_ctx_new");
            return;
        }

        do_cmsg_register(ctx);
    }
    else {
        sleep(2);
        if ((ctx = cmsg_ctx_new(cmsg_client, server_file, &ev_ctx)) == NULL) {
            log_err("cmsg_ctx_new");
            return;
        }

        do_cmsg_register(ctx);
        cmsg_send_str(ctx, 0, "hello");

	struct stu stu = {
		.age = 10,
		.name = "aaaaa",
	};

	       cmsg_send_obj(ctx, 1, &stu);

	       cmsg_send_null(ctx, 3);
    }

    while (1) {
            event_loop(&ev_ctx);
    }
}

int test_cmsg()
{
    /*test_cmsg_fork();*/

	event_ctx_t ev_ctx;
    cmsg_ctx_t *ctx;

    enable_console_log();
	set_max_log_level(INFO_LOG);

	event_ctx_init(&ev_ctx);

    ctx = cmsg_ctx_new(cmsg_inner, NULL, &ev_ctx);
    do_cmsg_register(ctx);

	cmsg_send_str(ctx, 0, "hello");
	cmsg_send_str(ctx, 0, "hello");
	cmsg_send_str(ctx, 0, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	cmsg_send_str(ctx, 0, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
	cmsg_send_str(ctx, 0, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
	cmsg_send_str(ctx, 0, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

    cmsg_send_null(ctx, 3);

	while (1) {
		event_loop(&ev_ctx);
	}


	return 0;
}

