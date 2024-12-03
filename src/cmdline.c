#include <stdio.h>
#include <string.h>

#include "sercmd.h"
#include "com_msg.h"
#include "logger.h"
#include "cmdline.h"

static cmdline_t cmdlist[];

cmdline_t *
cmdline_find(const char *data)
{
    cmdline_t *cmd;

    for (cmd = cmdlist; cmd->name; ++cmd) {
        log_debug("cmp [%s] [%s]", data, cmd->name);
        if (strncmp(cmd->name, data, strlen(cmd->name)) == 0)
            return cmd;
    }

    return NULL;
}

static int 
cmd_quit(cmdline_t *cmd, cmd_obj_t *obj, void *pri)
{
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)pri;

    return cmsg_send_null(ctx->cmsg_inner, inner_cmsg_quit);
}

static int 
cmd_set_log_level(cmdline_t *cmd, cmd_obj_t *obj, void *pri)
{
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)pri;
    log_debug("set log level");
    if (obj->pri.data)
        set_max_log_level(log_level_map(obj->pri.data));

    return 0;
}

static int 
cmd_list(cmdline_t *cmd, cmd_obj_t *obj, void *pri)
{
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)pri;
    log_debug("show");

    return 0;
}

static int 
cmd_del(cmdline_t *cmd, cmd_obj_t *obj, void *pri)
{
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)pri;
    log_info("%s : %s", obj->name, obj->pri.data);
    return 0;
}

static int 
cmd_add(cmdline_t *cmd, cmd_obj_t *obj, void *pri)
{
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)pri;
    log_info("%s : %s", obj->name, obj->pri.data);
    return 0;
}

static int 
cmd_restart(cmdline_t *cmd, cmd_obj_t *obj, void *pri)
{
    sercmd_ctx_t *ctx = (sercmd_ctx_t *)pri;

    return 0;
}

int
cmd_set_log_level_check(args_t *arg)
{
    if (arg->data && log_level_map(arg->data) > 0)
        return 0;
    return -1;
}

static cmdline_t cmdlist[] = {
    {
        .name = "quit",
        .do_exec = cmd_quit,
        .do_check = NULL,
        .help = "no args\n\t退出服务进程",
    },
    {
        .name = "restart",
        .do_exec = cmd_restart,
        .do_check = NULL,
        .help = "no args\n\t重启服务进程",
    },
    {
        .name = "add",
        .do_exec = cmd_add,
        .do_check = NULL,
        .help = "-d <data>\n\t添加"
    },
    {
        .name = "del",
        .do_exec = cmd_del,
        .do_check = NULL,
        .help = "-d <data>\n\t删除"
    },
    {
        .name = "list",
        .do_exec = cmd_list,
        .do_check = NULL,
        .help = "no args\n\t列出"
    },
    {
        .name = "setLogLevel",
        .do_exec = cmd_set_log_level,
        .do_check = cmd_set_log_level_check,
        .help = "-d <Error|Warn|Info|Debug>\n\t设置最大日志等级"
    },
    {
        NULL,
    }
};

char *
cmdline_help()
{
    int num = sizeof(cmdlist)/sizeof(*cmdlist);
    char *buf = malloc(num * 64);
    char tmp[64] = {0};
    cmdline_t *cmd = cmdlist;

    buf[0] = 0;
    
    for (cmd = cmdlist; cmd->name; ++cmd) {
        snprintf(tmp, sizeof(tmp) - 1, "  %s:%s\n", cmd->name, cmd->help);
        strcat(buf, tmp);
    }
    buf[strlen(buf) - 1] = '\0';

    return buf;
}

