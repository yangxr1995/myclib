#include <linux/if_ether.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <linux/sockios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/if.h>

#include "arr.h"
#include "com_msg.h"
#include "common.h"
#include "event.h"
#include "logger.h"
#include "sercmd.h"
#include "args.h"
#include "assert.h"
#include "cmdline.h"
#include "timer_wheel.h"

#define TIMER_WHEEL_CLCY    10

#define CLIENT_TEMP_UN_FILE "/var/run/sercmd.un"

int cmsg_deal_cmd_req(char *data, cmsg_t *cmsg)
{
    cmd_obj_t *obj = (cmd_obj_t *)data;
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)cmsg->pri;
    char *p = "ok";
    char *buf = NULL;
    cmdline_t *cmd;

    if ((cmd = cmdline_find(obj->name)) == NULL) {
        p = "unknow cmd";
        goto ret;
    }

    if (cmd->do_exec(cmd, obj, ctx)  < 0) {
        p = "err";
        goto ret;
    }

ret:
    cmsg_send_str(cmsg->ctx, cmsg_cmd, p);
    if (buf)
        free(buf);

    return 0;
}

    static int 
cmsg_build_cmd_req(void *org_data, void **pout, unsigned int *len)
{
    void *out;
    char *str = (char *)org_data;

    out = strdup(str);
    *pout = out;
    *len = strlen(out) + 1;

    return 0;
}

    static int 
cmsg_build_cmd_resp(void *org_data, void **pout, unsigned int *len)
{
    void *out;
    char *str = (char *)org_data;

    out = strdup(str);
    *pout = out;
    *len = strlen(out) + 1;

    return 0;
}

const char *args_cmd_list()
{
    return cmdline_help();
}

int cmsg_deal_cmd_resp(char *data, cmsg_t *cmsg)
{
    log_info("%s", data);
    return 0;
}

    static int 
cmsg_deal_quit(char *data, cmsg_t *cmsg)
{
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)cmsg->pri;

    log_info("程序退出");
    exit(0);

    return 0;
}


sercmd_ctx_t *
sercmd_ctx_new(args_t *args) {

    sercmd_ctx_t *ctx;

    ctx = (sercmd_ctx_t *)malloc(sizeof(*ctx));
    memset(ctx, 0x0, sizeof(*ctx));

    ctx->a = args;

    event_ctx_init(&ctx->ev_ctx);

    if (strcmp(args->mode, "server") == 0)
        ctx->mode = sercmd_mode_server;
    else if (strcmp(args->mode, "cmdline") == 0)
        ctx->mode = sercmd_mode_cmdline;
    else
        assert(0 && "未知mode");

    if ((ctx->cmsg_inner = cmsg_ctx_new(cmsg_inner, 
                    NULL, &ctx->ev_ctx)) == NULL) {
        log_err("cmsg_ctx_new");
        goto err;
    }

    if (ctx->mode == sercmd_mode_cmdline) {
        if ((ctx->cmsg_outer = cmsg_ctx_new(cmsg_client, 
                        CLIENT_TEMP_UN_FILE, &ctx->ev_ctx)) == NULL) {
            log_err("cmsg_ctx_new");
            goto err;
        }
    }
    else {
        if ((ctx->cmsg_outer = cmsg_ctx_new(cmsg_server, 
                        CLIENT_TEMP_UN_FILE, &ctx->ev_ctx)) == NULL) {
            log_err("cmsg_ctx_new");
            goto err;
        }
    }

    if (cmsg_register(ctx->cmsg_outer, ctx, "sercmd cmd", cmsg_cmd, 
                cmsg_deal_cmd_req, cmsg_deal_cmd_resp) < 0) {
        log_err("cmsg_register reconnect");
        goto err;
    }

    if (cmsg_register(ctx->cmsg_inner, ctx, 
                "quit", inner_cmsg_quit, 
                cmsg_deal_quit, NULL) < 0) {
        log_err("cmsg_register quit");
        goto err;
    }

    if (ctx->mode == sercmd_mode_server) {
        
    }

    return ctx;
err:
    return NULL;
}
