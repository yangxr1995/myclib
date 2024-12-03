#ifndef __CLIENT_TEMP_H__
#define __CLIENT_TEMP_H__


#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "com_msg.h"
#include "args.h"
#include "timer_wheel.h"

enum sercmd_mode {
    sercmd_mode_server,
    sercmd_mode_cmdline,
};
typedef enum sercmd_mode sercmd_mode_t;

typedef struct sercmd_ctx_s sercmd_ctx_t;
struct sercmd_ctx_s {
    sercmd_mode_t mode;

    int sockfd;
    event_t dns_resp_ev;
    event_ctx_t ev_ctx;

    cmsg_ctx_t *cmsg_inner;
    cmsg_ctx_t *cmsg_outer;

    args_t *a;

    int tfd;
    timer_wheel_node_t *timer_node;
};

enum outer_cmsg_s {
    cmsg_cmd,
};

enum edge_inner_cmsg_s  {
    inner_cmsg_quit,
};

sercmd_ctx_t *sercmd_ctx_new(args_t *args);

#endif
