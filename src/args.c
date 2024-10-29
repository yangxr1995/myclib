#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdlib.h>
#include <assert.h>

#include "args.h"

__attribute__((weak))  const char *args_cmd_list()
{
    return "[]";
}

inline static struct option *
option_find(struct option *option_arr, int val)
{
    int i = 0;
    for (i = 0; option_arr[i].val; ++i) {
        if (option_arr[i].val == val)
            return option_arr + i;
    }
    return NULL;
}

void
print_help()
{
    printf("options : \n");
#define ARGS_DEF_HELP
#include "args_def.h"

    exit(0);
}

args_t *args_parse(int argc, char **argv)
{
    args_t default_a = {
#define ARGS_DEF_DEFAULT
#include "args_def.h"
    };

    args_t *a;

    a = malloc(sizeof(*a));
    memcpy(a, &default_a, sizeof(*a));

    int opt;
    int option_index = 0;
    int input_file = 0;
    struct option longopts[] = {
#define ARGS_DEF_OPTION
#include "args_def.h"
        {NULL,       0,                 NULL, 0}
    };

    char shortopts[256] = {0};
    char tmp[2] = {0};

    int i = 0;
    for (i = 0; longopts[i].val; ++i) {
        tmp[0] = longopts[i].val;
        strcat(shortopts, tmp);
        if (longopts[i].has_arg == required_argument)
            strcat(shortopts, ":");
    }

    struct option *popt;



    while ((opt = getopt_long(argc, argv, shortopts, longopts, &option_index)) != -1) {

        if ((popt = option_find(longopts, opt)) == NULL)
            print_help();

#define ARGS_DEF_PARSE
#include "args_def.h"

        print_help();
    }

#define ARGS_DEF_PRINT
#include "args_def.h"

    return a;
}

