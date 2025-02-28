#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "debug.h"
#include <errno.h>

int test_cmsg();
int main()
{
    return test_cmsg();
}
