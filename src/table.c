#include <alloca.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "arr.h"

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define TB_DEBUG 0

#if TB_DEBUG == 0
#define debug(fmt, ...) NULL
#else
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

union align {
    int i;
    long l;
    long *lp;
    void (*fp)(void);
    float d;
    double ld;
};

inline static unsigned int align_up(unsigned int size, unsigned int align)
{
    return (size + align - 1) / align * align;
}

typedef struct entry_s{
    struct entry_s *next;
    const void *key;
    void *value;
} entry_t;

typedef struct bucket_s {
    entry_t *entrys;
    pthread_spinlock_t lock;
} bucket_t;

typedef struct table_s {
    _Atomic(int) size;
    _Atomic(int) length;
    int (*cmp)(const void *x, const void *y);
    unsigned int (*hash)(const void *key);
    bucket_t *buckets;
} *table_t;

table_t table_new(int hint, int (*cmp)(const void *x, const void *y), 
        unsigned int (*hash)(const void *key))
{
    table_t tb;
    unsigned int primes[] = {
        1, 3 ,11, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX
    };
    unsigned int i;

    assert(cmp != NULL);
    assert(hash != NULL);
    assert(hint >= 0);

    for (i = 1; primes[i] < hint; i++)
        NULL;

    unsigned int sz = align_up(sizeof(*tb), sizeof(union align));
    
    tb = (table_t) malloc( sz + sizeof(*tb->buckets) * primes[i-1]);
    atomic_init(&tb->size, primes[i - 1]);
    atomic_init(&tb->length, 0);
    tb->cmp = cmp;
    tb->hash = hash;
    tb->buckets = (bucket_t *)((char *)tb + sz);
    atomic_thread_fence(memory_order_release);
    for (i = 0; i< tb->size; i++) {
        tb->buckets[i].entrys = NULL;
        pthread_spin_init(&tb->buckets[i].lock, PTHREAD_PROCESS_PRIVATE);
    }

    return tb;
}

void table_free(table_t *ptb)
{
    entry_t *entry, *tmp;
    unsigned int i;
    table_t tb;
    bucket_t *bucket;

    assert(ptb && *ptb);
    tb = *ptb;
    for (i = 0; i < tb->size; ++i) {
        bucket = tb->buckets + i;
        pthread_spin_lock(&bucket->lock);
        for (entry = bucket->entrys; entry; entry = tmp) {
            tmp = entry->next;
            free(tmp);
        }
        pthread_spin_unlock(&bucket->lock);
    }

    debug("Free table, size : %d, length : %d\n ", tb->size, tb->length);

    free(tb);
    *ptb = NULL;
}

#define bucket_find(_key) \
    ({ \
     idx = (*tb->hash)(_key)%tb->size; \
     tb->buckets + idx; \
     })

#define entry_find(_key) \
    ({ \
     entry_t *____pos; \
     for (____pos = bucket->entrys; ____pos; ____pos = ____pos->next) { \
     if ((*tb->cmp)(____pos->key, key) == 0) \
     break; \
     } \
     ____pos; \
     })

/*
 * key必须唯一
 * 返回 NULL，插入新元素成功
 * 返回 != NULL, 插入新元素失败，返回已存在的元素
 */
void *table_put(table_t tb, const void *key, void *value)
{
    unsigned int idx;
    entry_t *entry;
    void *prev;
    bucket_t *bucket;

    assert(tb);
    /*assert(key);*/

    bucket = bucket_find(key);

    pthread_spin_lock(&bucket->lock);

    if (LIKELY(((entry = entry_find(key)) == NULL))) 
        prev = NULL;
    else
        prev = entry->value;

    if (prev == NULL) {
        entry = (entry_t *)malloc(sizeof(entry_t));	
        entry->key = key;
        entry->value = value;
        entry->next = bucket->entrys;
        bucket->entrys = entry;
        atomic_fetch_add(&tb->length, 1);
    }

    pthread_spin_unlock(&bucket->lock);

    debug("Put entry into table, size : %d, idx : %d, length : %d\n ", 
               atomic_load(&tb->size), idx, atomic_load(&tb->length));

    return prev;
}

void *table_get(table_t tb, const void *key)
{
    unsigned int idx;
    entry_t *entry;
    bucket_t *bucket;
    void *ret = NULL;

    assert(tb);
    assert(key);

    bucket = bucket_find(key);

    pthread_spin_lock(&bucket->lock);
    if ((entry = entry_find(key)) != NULL)
        ret = entry->value;
    pthread_spin_unlock(&bucket->lock);

    debug("Get entry from table, idx : %d, length : %d\n ", idx, atomic_load(&tb->length));

    return ret;
}

int table_length(table_t tb)
{
    assert(tb);
    return atomic_load(&tb->length);
}

void *table_remove(table_t tb, const void *key)
{
    unsigned int idx;
    entry_t **pp, *p;
    bucket_t *bucket;
    void *ret = NULL;

    assert(tb);
    assert(key);

    bucket = bucket_find(key);
    pthread_spin_lock(&bucket->lock);
    for (pp = (entry_t **)&bucket->entrys; *pp; pp = &(*pp)->next) {
        p = *(pp);
        if (tb->cmp(p->key, key) == 0) {
            *pp = p->next;
            ret = p->value;
            free(p);
            atomic_fetch_sub(&tb->length, 1);
            break;
        }
    }
    pthread_spin_unlock(&bucket->lock);

    debug("Remove entry from table, idx : %d, length : %d\n ", idx, atomic_load(&tb->length));

    return ret;
}

/*
 * apply不应该阻塞，否则会导致bucket被锁很久
 */
void table_map(table_t tb, void (*apply)(const char *key, void **value, void *cl), void *cl)
{
    entry_t *entry;
    unsigned int i, stamp;
    bucket_t *bucket;
    int cnt, sum;

    assert(tb);
    assert(apply);

    cnt = sum = 0;
    for (i = 0; i < tb->size; ++i) {
        bucket = tb->buckets + i;
        cnt = 0;
        pthread_spin_lock(&bucket->lock);
        for (entry = bucket->entrys; entry; entry = entry->next) {
            apply(entry->key, &entry->value, cl);
            ++cnt;
        }
        pthread_spin_unlock(&bucket->lock);
        sum += cnt;
        debug("Map bucket idx[%d] cnt[%d]\n", i, cnt);
    }
    debug("Map all bucket cnt[%d]\n", sum);
}

arr_t *table_to_array(table_t tb, void *end)
{
    unsigned int idx, j, nb;
    entry_t *entry;
    bucket_t *bucket;
    arr_t *arr;
    void **pbuf;

    assert(tb);
    arr = arr_create(atomic_load(&tb->length), sizeof(void *));
    for (idx = 0; idx < tb->size; ++idx) {
        bucket = tb->buckets + idx;
        pthread_spin_lock(&bucket->lock);
        for (entry = bucket->entrys; entry; entry = entry->next) {
            pbuf = arr_push(arr);
            *pbuf = entry->value;
        }
        pthread_spin_unlock(&bucket->lock);
    }
    debug("Get array from table, length : %d\n ", arr->nelts);

    return arr;
}

#include <pthread.h>

static const int writer_nb = 10;
static const int reader_nb = 5;
static const int deleter_nb = 10;
static const int item_max_nb = 10240;
static table_t tb;

static int cmp_int(const void *a, const void *b)
{
    long long ia, ib;

    ia = *(long long *)&a;
    ib = *(long long *)&b;

    return ia == ib ? 0 : 1;
}

static unsigned int hash_int(const void *a)
{
    return *(long long *)&a;
}

static void *writer_run(void *arg)
{
    long long i;
    long long start = *(long long *)&arg, n;
    start = start * item_max_nb;


    for (i = 0; i < item_max_nb; ++i) {
        n = start + i;
        table_put(tb, *(void **)&n, *(void **)&n);
    }

    return NULL;
}

int cmp_uint (const void *a, const void *b)
{
    unsigned int ca, cb;
    ca = *(unsigned int *)a;
    cb = *(unsigned int *)b;
    if (ca == cb)
        return 0;
    if (ca > cb)
        return 1;
    return -1;
}

static void *reader_run(void *arg)
{
    arr_t *arr;
    unsigned int *map;
    int nb = item_max_nb * writer_nb;

    map = alloca(nb * sizeof(*map));
    memset(map, 0x0, sizeof(*map)*nb);

    arr = table_to_array(tb, NULL);

    arr_destroy(arr);

    return NULL;
}

static void *checker_run(void *arg)
{
    arr_t *arr;
    unsigned int *map;
    int nb = item_max_nb * writer_nb;

    map = malloc(nb *sizeof(*map));
    memset(map, 0x0, sizeof(*map)*nb);

    arr = table_to_array(tb, NULL);

    void **pos;
    long long val;
    int i = 0;
    arr_for_each(arr , pos) {
        val = *(long long *)pos;
        map[i++] = val;
    }

    debug("nb : %d\n", nb);
    qsort(map, nb, sizeof(*map), cmp_uint);
    for (i = 0; i < nb; ++i) {
        debug("[%d] ", map[i]);
        if (i != map[i]) {
            debug("%s %d, i[%d] != map[%d]\n", __func__, __LINE__, i, map[i]);
            exit(-1);
        }
    }
    debug("\n");

    arr_destroy(arr);
    free(map);

    return NULL;
}


int table_test()
{
    pthread_t wtid[writer_nb], rtid[reader_nb];
    long long i;

    tb = table_new(1, cmp_int, hash_int);

    for (i = 0; i < writer_nb ; ++i) {
        pthread_create(wtid + i, NULL, writer_run, *(void **)&i);
    }

    for (i = 0; i < reader_nb ; ++i) {
        pthread_create(rtid + i, NULL, reader_run, *(void **)&i);
    }

    for (i = 0; i < writer_nb ; ++i) {
        pthread_join(wtid[i], NULL);
    }
    for (i = 0; i < reader_nb ; ++i) {
        pthread_join(rtid[i], NULL);
    }

    checker_run(tb);

    return 0;
}
