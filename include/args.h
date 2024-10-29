#ifndef __ARGS_H__
#define __ARGS_H__

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

typedef struct args_s {
#define ARGS_DEF_STRUCT
#include "args_def.h"
} args_t;

args_t *args_parse(int argv, char **args);

void print_help();

#endif

