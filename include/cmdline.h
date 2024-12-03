#ifndef __CMDLINE_H__
#define __CMDLINE_H__

#include "args.h"

#define CMD_NAME_MAX_LEN 32

#define CMD_DATA_MAX_LEN 256

typedef struct {
    char name[32];
} cmd_arg_add_del_t;

typedef struct {
    char name[CMD_NAME_MAX_LEN];
    union {
        char data[CMD_DATA_MAX_LEN];
    } pri;
} cmd_obj_t;

typedef struct cmdline_s cmdline_t;
struct cmdline_s {
    char *name;
    int (*do_exec)(cmdline_t *cmd, cmd_obj_t *obj, void *pri);
    int (*do_check)(args_t *arg);
    char *help;
};

cmdline_t *cmdline_find(const char *name);

char *cmdline_help();

#endif
