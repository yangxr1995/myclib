#include <alloca.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

#include "logger.h"
#include "table.h"
#include "assert.h"
#include "com_msg.h"

int table_test();
int test_timer_wheel();
int test_cmsg();
int test_aw(int argc, char **argv);

int main(int argc, char *argv[])
{
    enable_console_log();
    test_aw(argc, argv);
    /*test_timer_wheel();*/
    return EXIT_SUCCESS;
}

