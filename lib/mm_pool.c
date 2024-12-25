#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mm_pool.h"
//#include "memchk.h"

#define MM_BLOCK	(512)

#define _MM_DEBUG

#ifdef _MM_DEBUG
#undef _MM_DEBUG
#endif

#ifdef USE_MUTEX
#undef USE_MUTEX
#endif
#ifdef USE_SPINLOCK
#undef USE_SPINLOCK
#endif

#define USE_MUTEX      0
#define USE_SPINLOCK   1

#if USE_MUTEX & USE_SPINLOCK
#error "USE_MUTEX and USE_SPINLOCK can not be both 1"
#endif

/*
 * TODO :
 * 1. 使用malloc分配大块内存,并进行登记
 */

struct mpool {
	struct mpool *prev;	
	char *avail;
	char *limit;
	mpool_free_func_t *free_list;
#if USE_MUTEX
	pthread_mutex_t locker;
#endif
#if USE_SPINLOCK
    pthread_spinlock_t locker;
#endif
};

struct mpool_free_func_s {
	mpool_free_func_t *next;
	void (*free_func)(void *data);
	void *data;
};

union align {
	int i;
	long l;
	long *lp;
	void (*fp)(void);
	float d;
	double ld;
};

union header {
	union align a;
	struct mpool m;
};

struct mpool *
mpool_new(void) 
{
	struct mpool *mpool;

	mpool = malloc(sizeof(struct mpool));
	mpool->free_list = NULL;
	mpool->prev = NULL;
	mpool->avail = NULL;
	mpool->limit = NULL;
	/*mpool->locker = malloc(sizeof(*(mpool->locker)));*/

#if USE_MUTEX
	pthread_mutex_init(&mpool->locker, NULL);
#endif

#if USE_SPINLOCK
    pthread_spin_init(&mpool->locker, 0);
#endif

	return mpool;
}

void
_mpool_do_free_list(struct mpool *mpool)
{
	mpool_free_func_t *func;

	for (func = mpool->free_list; func; func = func->next) {
		func->free_func(func->data);
	}
	mpool->free_list = NULL;
}

void
mpool_do_free_list(struct mpool *mpool)
{
	assert(mpool);

#if USE_MUTEX
	pthread_mutex_lock(&mpool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_lock(&mpool->locker);
#endif


	_mpool_do_free_list(mpool);

#if USE_MUTEX
	pthread_mutex_unlock(&mpool->locker);
#endif

#if USE_SPINLOCK
    pthread_spin_unlock(&mpool->locker);
#endif
}

void 
_mpool_destroy(struct mpool **mpool) 
{
	struct mpool *ptr, *tmp;

	if (mpool == NULL)
		return;
	ptr = *mpool;
	if (ptr == NULL)
		return;

	_mpool_do_free_list(*mpool);

	do {
		tmp = ptr->prev;
#ifdef _MM_DEBUG
		free_cnt++;
#endif
		free(ptr);
		ptr = tmp;
	} while (ptr);
}

void 
mpool_destroy(struct mpool **mpool) 
{
	assert(mpool);
	assert(*mpool);

#if USE_MUTEX
	pthread_mutex_lock(&(*mpool)->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_lock(&(*mpool)->locker);
#endif

	_mpool_destroy(mpool);
#if USE_MUTEX
	pthread_mutex_unlock(&(*mpool)->locker);
	pthread_mutex_destroy(&(*mpool)->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_unlock(&(*mpool)->locker);
    pthread_spin_destroy(&(*mpool)->locker);
#endif
}

void *
_mpool_alloc(struct mpool *mpool, size_t nbytes) 
{
	struct mpool *ptr, *prev;
	unsigned int m;

	if (mpool == NULL)
		return NULL;
	if (nbytes <= 0)
		return NULL;

	nbytes = (nbytes + sizeof(union align) - 1)/
		sizeof(union align)*sizeof(union align);

	ptr = mpool;

__again__:
	if (ptr->avail) {
		if ((unsigned int)(ptr->limit - ptr->avail) > nbytes) {
			ptr->avail += nbytes;
			return ptr->avail - nbytes;
		}
		else {
			ptr = ptr->prev;
			goto __again__;
		}
	}
	else {
		m = sizeof(union header) + nbytes + MM_BLOCK;
#ifdef _MM_DEBUG
		alloc_cnt++;
#endif
		if ((ptr->prev = malloc(m)) == NULL)
			return NULL;
		prev = (struct mpool *)ptr->prev;
		prev->prev = NULL;
		prev->avail = NULL;
		prev->limit = NULL;
		ptr->avail = ((char *)ptr->prev) + sizeof(union header);
		ptr->limit = ((char *)ptr->prev) + m;

		goto __again__;
	}

	return NULL;
}

void *
mpool_alloc(struct mpool *mpool, size_t nbytes) 
{
	void *ret = NULL;

	assert(mpool);
	assert(nbytes > 0);

#if USE_MUTEX
	pthread_mutex_lock(&mpool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_lock(&mpool->locker);
#endif
	ret = _mpool_alloc(mpool, nbytes);
#if USE_MUTEX
	pthread_mutex_unlock(&mpool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_unlock(&mpool->locker);
#endif

	return ret;
}



void 
_mpool_free(struct mpool *mpool)
{
	struct mpool *ptr, *tmp;

	assert(mpool);

	ptr = mpool->prev;
	if (ptr == NULL)
		return;

	_mpool_do_free_list(mpool);

	do {
		tmp = ptr->prev;
#ifdef _MM_DEBUG
		free_cnt++;
#endif
		free(ptr);
		ptr = tmp;
	} while (ptr);
}

void 
mpool_free(struct mpool *mpool)
{
	assert(mpool);

#if USE_MUTEX
	pthread_mutex_lock(&mpool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_lock(&mpool->locker);
#endif
	_mpool_free(mpool);
#if USE_MUTEX
	pthread_mutex_unlock(&mpool->locker);
	pthread_mutex_destroy(&mpool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_unlock(&mpool->locker);
    pthread_spin_destroy(&mpool->locker);
#endif
}

void 
_mpool_clear(struct mpool *mpool)
{
	struct mpool *pos;

	if (mpool == NULL)
		return;

	_mpool_do_free_list(mpool);

	for (pos = mpool; pos; pos = pos->prev) {
		if (pos->avail) {
			pos->avail = ((char *)pos->prev) + sizeof(union header);
		}
	}
}

void 
mpool_clear(struct mpool *mpool)
{
	assert(mpool);
#if USE_MUTEX
	pthread_mutex_lock(&mpool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_lock(&mpool->locker);
#endif
	_mpool_clear(mpool);
#if USE_MUTEX
	pthread_mutex_unlock(&mpool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_unlock(&mpool->locker);
#endif
}

mpool_free_func_t *
_mpool_free_func_alloc(mpool_t *pool)
{
	mpool_free_func_t *func;

	func = _mpool_alloc(pool, sizeof(*func));
	memset(func, 0x0, sizeof(*func));

	func->next = pool->free_list;
	pool->free_list = func;

	return func;
}

mpool_free_func_t *
mpool_free_func_alloc(mpool_t *pool)
{
	mpool_free_func_t *func;

	assert(pool);
#if USE_MUTEX
	pthread_mutex_lock(&pool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif
	func = _mpool_free_func_alloc(pool);
#if USE_MUTEX
	pthread_mutex_unlock(&pool->locker);
#endif
#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
#endif

	return func;
}
