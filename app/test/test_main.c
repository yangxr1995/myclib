#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "debug.h"
#include <errno.h>

void func3()
{
    void *ret = malloc(10);
}

void func2()
{
    void *ret = malloc(10);
    func3();
    free(ret);
}

void func1()
{
    void *ret = malloc(10);
    func2();
    free(ret);
}

int main()
{
    func1();
    sleep(100);

    return 0;
}
