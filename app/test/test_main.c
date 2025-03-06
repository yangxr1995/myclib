#include <fcntl.h>
#include <string.h>
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

void func11()
{
    void *ptr = malloc(1);
    void *ptr2 = malloc(1);
    void *ptr3 = malloc(1);
}

int main()
{
        printf("%d\n", getpid());
    func1();
    if (fork() == 0) {
        printf("%d\n", getpid());
        func1();
    }
    else {
    }
    sleep(20);
    return 0;
}
