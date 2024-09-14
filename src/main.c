#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>


#include <alloca.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "list_cas.h"
#include "args.h"

args_t *cmdline;
pthread_spinlock_t spinlock;
list_head_def(stu);

#define THREAD_INSTER_NB 10
#define THREAD_DELETER_NB 10
#define INSERT_NB 10000
#define DELETE_NB 100

struct stu {
    int age;
    list_head_t list;
};

int list_num(struct list_head *head)
{
    int cnt = 0;
    list_head_t *pos;

    list_for_each(pos, head) {
        ++cnt;
    }

    return cnt;
}

void *th_delete(void *arg)
{
    int i;
    struct stu *new;

    for (i = 0; i < DELETE_NB; ++i) {
        list_del(stu.next);
    }

    return NULL;
}

void *th_insert(void *arg)
{
    int i;
    struct stu *new;

    for (i = 0; i < INSERT_NB; ++i) {
        new = malloc(sizeof(*new));
        new->age = i;
        list_head_init(&new->list);
        list_add_head(&new->list , &stu);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t th_inserter[THREAD_INSTER_NB];
    pthread_t th_deleter[THREAD_DELETER_NB];

    cmdline = args_parse(argc, argv);

    pthread_spin_init(&spinlock, 0);

    int i;
    for (i = 0; i < THREAD_INSTER_NB; ++i) {
        pthread_create(th_inserter + i, NULL, th_insert, *(void **)&i);
    }
    for (i = 0; i < THREAD_DELETER_NB; ++i) {
        pthread_create(th_deleter + i, NULL, th_delete, *(void **)&i);
    }

    for (i = 0; i < THREAD_INSTER_NB; ++i) {
        pthread_join(th_inserter[i], NULL);
    }

    for (i = 0; i < THREAD_DELETER_NB; ++i) {
        pthread_join(th_deleter[i], NULL);
    }

    if (THREAD_INSTER_NB * INSERT_NB == list_num(&stu)) {
        printf("right : %d\n", list_num(&stu));
    }
    else {
        printf("error : %d\n", list_num(&stu));
    }

    return 0;
}
