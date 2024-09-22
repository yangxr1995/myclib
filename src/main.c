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

int table_test();
int test_timer_wheel();

int main(int argc, char *argv[])
{
    enable_console_log();
    /*test_timer_wheel();*/
    return EXIT_SUCCESS;
}

#include "args.h"
#include "tracer.h"
#include "rcu.h"

#include "list_rcu.h"

struct stu {
    int a;
    int b;
    int c;
    list_head_t list;
};

struct stu *gp;

LIST_HEAD_INITIALIZE(stu_list);

pthread_spinlock_t spinlock;
pthread_rwlock_t rwlock;
pthread_mutex_t mutex;

atomic_int is_over;

args_t *arg;
void *(*reader_run)(void *);
void *(*writer_run)(void *);
void *(*deleter_run)(void *);

#define SLEEP_NB 200000

#define THREAD_NB 10

#define COUNT_MAX 1000000

#define MAX_NUM 200000
#define DELETE_MAX_NUM 2000

inline static void stu_insert()
{
    struct stu *new;

    new = malloc(sizeof(*new));
    list_init_rcu(&new->list);
    list_add_tail(&new->list, &stu_list);
}

static inline int list_length(list_head_t *head)
{
    list_head_t *pos;
    int cnt = 0;
    list_for_each(pos, head) {
        cnt++;
    }
    return cnt;
}

void *reader_run_rcu(void *arg1)
{
    rcu_init();

    int end;
    end = (arg->nb_writer * MAX_NUM) - (arg->nb_deleter * DELETE_MAX_NUM);

    for (;;) {
        rcu_read_lock();

        int len = list_length(&stu_list);
        printf("r:len[%d] end[%d] over[%d]\n", len, end, is_over);
        if (len == end) {
            rcu_read_unlock();
            printf("len[%d], end[%d]\n", len, end);
            break;
        } 

        rcu_read_unlock();
    }

    /*atomic_fetch_add(&is_over, 1);*/

    return NULL;
}

void *reader_run_rwlock(void *arg1)
{

    for (;;) {

        pthread_rwlock_rdlock(&rwlock);

        printf("r:a[%d] b[%d] c[%d]\n", gp->a, gp->b, gp->c);
       assert(gp->a == gp->b && gp->b == gp->c);
        /*usleep(SLEEP_NB);*/
        if (gp->a == MAX_NUM * arg->nb_writer) {
            pthread_rwlock_unlock(&rwlock);
            break;
        }

        pthread_rwlock_unlock(&rwlock);

    }

    return NULL;
}

void *reader_run_mutex(void *arg1)
{

    for (;;) {

        pthread_mutex_lock(&mutex);

        printf("r:a[%d] b[%d] c[%d]\n", gp->a, gp->b, gp->c);
        assert(gp->a == gp->b && gp->b == gp->c);
        /*usleep(SLEEP_NB);*/
        if (gp->a == MAX_NUM * arg->nb_writer) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        pthread_mutex_unlock(&mutex);

    }

    return NULL;
}

void *reader_run_spinlock(void *a)
{

    for (;;) {

        pthread_spin_lock(&spinlock);

        int len = list_length(&stu_list);
        /*printf("r:len[%d]\n", len);*/

        int cur_nb = atomic_load(&is_over);
        if (cur_nb == arg->nb_deleter + arg->nb_writer) {
            len = list_length(&stu_list);
            printf("r:len[%d]\n", len);
            if (len != (arg->nb_writer * MAX_NUM) - (arg->nb_deleter * DELETE_MAX_NUM))
                printf("-----error len[%d]\n", len);
            pthread_spin_unlock(&spinlock);

            break;
        }

        pthread_spin_unlock(&spinlock);

        usleep(1);

    }

    return NULL;
}

void *writer_run_rwlock(void *arg)
{
    int i;

    for (i = 1; i <= MAX_NUM; ++i) {

        pthread_rwlock_wrlock(&rwlock);

        gp->a = i;
        gp->b = i;
        gp->c = i;
        /*printf("w:a[%d] b[%d] c[%d]\n", gp->a, gp->b, gp->c);*/
        /*usleep(SLEEP_NB);*/
        /**/
        pthread_rwlock_unlock(&rwlock);
    }

    return NULL;
}

void *writer_run_mutex(void *arg)
{
    int i;

    for (i = 1; i <= MAX_NUM; ++i) {

        pthread_mutex_lock(&mutex);

        gp->a = i;
        gp->b = i;
        gp->c = i;
        /*printf("w:a[%d] b[%d] c[%d]\n", gp->a, gp->b, gp->c);*/
        /*usleep(SLEEP_NB);*/

        pthread_mutex_unlock(&mutex);

    }

    return NULL;
}

inline static struct stu *stu_delete_head()
{
    if (list_empty(&stu_list)) {
        return NULL;
    }
    list_head_t *pos = stu_list.next;
    list_head_del(stu_list.next);
    return list_entry(pos, struct stu, list);
}

void *deleter_run_spinlock(void *arg)
{
    int i;
    char buf[256] = {0};

    for (i = 1; i <= DELETE_MAX_NUM; ++i) {

        pthread_spin_lock(&spinlock);

        struct stu *stu;
        stu = stu_delete_head();
        if (stu == NULL) {
            --i;
            pthread_spin_unlock(&spinlock);
            continue;
        }
        free(stu);

        pthread_spin_unlock(&spinlock);

    }

    atomic_fetch_add(&is_over, 1);

    return NULL;
}

void *writer_run_spinlock(void *arg)
{
    int i;
    char buf[256] = {0};

    for (i = 1; i <= MAX_NUM; ++i) {

        pthread_spin_lock(&spinlock);

        stu_insert();

        pthread_spin_unlock(&spinlock);

    }

    atomic_fetch_add(&is_over, 1);

    return NULL;
}

void *deleter_run_rcu(void *arg)
{
    rcu_init();

    int i;
    struct stu *new, *old;

    for (i = 1; i <= DELETE_MAX_NUM; ++i) {

        pthread_spin_lock(&spinlock);

        struct stu *stu;
        stu = stu_delete_head();
        if (stu == NULL) {
            printf("--%d-\n", i);
            --i;
            pthread_spin_unlock(&spinlock);
            usleep(1);
            continue;
        }

        pthread_spin_unlock(&spinlock);

        /*synchronize_rcu();*/
        /*free(stu);*/
    }

    atomic_fetch_add(&is_over, 1);

    return NULL;
}

void *writer_run_rcu(void *arg)
{
    rcu_init();

    int i;
    struct stu *new, *old;

    for (i = 1; i <= MAX_NUM; ++i) {

        pthread_spin_lock(&spinlock);

        stu_insert();

        pthread_spin_unlock(&spinlock);
    }

    atomic_fetch_add(&is_over, 1);

    return NULL;
}


void test()
{
    int i;

    pthread_t *reader = alloca(sizeof(*reader) * arg->nb_reader);
    pthread_t *writer = alloca(sizeof(*writer) * arg->nb_writer);
    pthread_t *deleter = alloca(sizeof(*deleter) * arg->nb_deleter);

    for (i = 0; i < arg->nb_reader; ++i) {
        pthread_create(reader + i, NULL, reader_run, NULL);
    }

    for (i = 0; i < arg->nb_writer; ++i) {
        pthread_create(writer + i, NULL, writer_run, NULL);
    }

    for (i = 0; i < arg->nb_deleter; ++i) {
        pthread_create(deleter + i, NULL, deleter_run, NULL);
    }


    for (i = 0; i < arg->nb_deleter; ++i) {
        pthread_join(deleter[i], NULL);
    }

    for (i = 0; i < arg->nb_reader; ++i) {
        pthread_join(reader[i], NULL);
    }

    for (i = 0; i < arg->nb_writer; ++i) {
        pthread_join(writer[i], NULL);
    }
}


int main2(int argc, char *argv[])
{
    arg = args_parse(argc, argv);

    if (strcmp(arg->mode, "spinlock") == 0) {
        reader_run = reader_run_spinlock;
        writer_run = writer_run_spinlock;
        deleter_run = deleter_run_spinlock;
        gp = malloc(sizeof(*gp));
    }
    else if (strcmp(arg->mode, "rcu") == 0) {
        reader_run = reader_run_rcu;
        writer_run = writer_run_rcu;
        deleter_run = deleter_run_rcu;
        gp = malloc(sizeof(*gp));
        memset(gp, 0x0, sizeof(*gp));
    }
    else {
        printf("unknown mode[%s]\n", arg->mode);
        return -1;
    }

    pthread_spin_init(&spinlock, 0);

    time_check_loop(test(), 1);

    return EXIT_SUCCESS;
}
