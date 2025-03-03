#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
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
    strcmp("aaa", "aaa");
    func1();
    sleep(100);

    int len = strlen("aaa");

    return 0;
}
