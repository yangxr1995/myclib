#include <pthread.h>
#include "debug.h"

void func3()
{
    /*char buf[64];*/
    /*get_prg_name(buf, sizeof(buf), getpid());*/
    /*printf("[%ld] %s\n", pthread_self(), buf);*/
}

void *func1(void *arg)
{
    while (1) {
        func3();
        sleep(1);
    }

    return NULL;
}

void 
func2()
{
    /*char buf[64];*/
    /*get_prg_name(buf, sizeof(buf), getpid());*/
    /*printf("[%ld] %s\n", pthread_self(), buf);*/
}

int main()
{
    pthread_t tid;
    pthread_create(&tid, NULL, func1, NULL);

    while (1) {
        func2();
        sleep(1);
    }

    return 0;
}
