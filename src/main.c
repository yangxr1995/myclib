#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <bits/getopt_core.h>
#include <signal.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>

#include "com_msg.h"
#include "common.h"
#include "event.h"
#include "logger.h"
#include "task.h"
#include "args.h"
#include "assert.h"
#include "cmdline.h"

#include "sercmd.h"

#define LOG_FILE "/var/log/sercmd.log"

static void do_exit();

void 
sig_handler(int signum, siginfo_t *info, void *cl)
{
    do_exit();
    log_err("sig[%d]\n", signum);
    print_stacktrace2();
    _exit(1);
}

int init_sig()
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = sig_handler;
	action.sa_flags = SA_SIGINFO;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGABRT, &action, NULL);
	sigaction(SIGPIPE, &action, NULL);
	sigaction(SIGBUS, &action, NULL);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGKILL, &action, NULL);

    return 0;
}

static void do_exit()
{
}

int _sercmd_start(void *arg)
{
    args_t *args = (args_t *)arg;
    sercmd_ctx_t *ctx;

    init_sig();

    open_log_file(LOG_FILE);
    
    if ((ctx = sercmd_ctx_new(args)) == NULL) {
        log_err("sercmd_ctx_new");
        goto err;
    }

    while (1) {
        event_loop(&ctx->ev_ctx);
    }

    return 0;
err:
    return -1;

}

inline static int
check_args(args_t *args)
{
    if (args->mode == NULL)
        return -1;

    if (strcmp(args->mode, "server") == 0) {

    }
    else if (strcmp(args->mode, "cmdline") == 0) {
        if (args->cmd == NULL) {
            printf("cmdline must set\n");
            return -1;
        }
        cmdline_t *cmd;
        if ((cmd = cmdline_find(args->cmd)) == NULL) {
            printf("unknow cmd [%s]\n", args->cmd);
            return -1;
        }

        if (cmd->do_check && cmd->do_check(args) < 0) {
            printf("参数错误 : -c %s %s\n", cmd->name, cmd->help);
            return -1;
        }

    }
    else {
        printf("unknow mode '%s'\n", args->mode);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    args_t *args;

    args = args_parse(argc, argv);

    if (check_args(args) < 0) {
        printf("\nUsage:\n");
        print_help();
        return -1;
    }
    set_max_log_line(10240);

    if (strcmp(args->mode, "cmdline") == 0) {
        sercmd_ctx_t *ctx;

        enable_console_log();
        set_max_log_level(DEBUG_LOG);

        if ((ctx = sercmd_ctx_new(args)) == NULL) {
            log_err("sercmd_ctx_new");
            return -1;
        }

        cmd_obj_t obj;
        strcpy(obj.name, args->cmd);
        if (args->data)
            strcpy(obj.pri.data, args->data);
        cmsg_send_obj(ctx->cmsg_outer, cmsg_cmd, &obj);

        event_loop(&ctx->ev_ctx);
        unlink(ctx->cmsg_outer->client_file);
        return 0;
    }

    if (args->front)
        enable_console_log();

    set_max_log_level(NO_LOG);

    if (!args->front) {
#ifndef USE_TASK
        if (daemon(0, 0) < 0)
            log_err("daemon");
        _sercmd_start(args);
#else
        task_start(_sercmd_start, args, "sercmd");
#endif
    }
    else {
        return _sercmd_start(args);
    }

    return 0;
}
