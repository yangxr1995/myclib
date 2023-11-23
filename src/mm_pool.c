#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mm_pool.h"

#define MM_BLOCK	(512)

#define _MM_DEBUG

#ifdef _MM_DEBUG
#undef _MM_DEBUG
#endif

#ifdef _MM_DEBUG
static int alloc_cnt;
static int free_cnt;
#endif

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

#ifdef _MM_DEBUG
	alloc_cnt++;
#endif

	mpool = malloc(sizeof(struct mpool));
	mpool->prev = NULL;
	mpool->avail = NULL;
	mpool->limit = NULL;
	mpool->locker = malloc(sizeof(*(mpool->locker)));
	pthread_mutex_init(mpool->locker, NULL);

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
	pthread_mutex_lock(mpool->locker);
	_mpool_do_free_list(mpool);
	pthread_mutex_unlock(mpool->locker);
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
	pthread_mutex_t *locker;

	assert(mpool);
	assert(*mpool);

	locker = (*mpool)->locker;

	pthread_mutex_lock(locker);
	_mpool_destroy(mpool);
	pthread_mutex_unlock(locker);
	pthread_mutex_destroy(locker);
	free(locker);
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

	pthread_mutex_lock(mpool->locker);
	ret = _mpool_alloc(mpool, nbytes);
	pthread_mutex_unlock(mpool->locker);

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
	pthread_mutex_t *locker;

	assert(mpool);

	locker = mpool->locker;
	pthread_mutex_lock(locker);
	_mpool_free(mpool);
	pthread_mutex_unlock(locker);
	pthread_mutex_destroy(locker);
	free(locker);
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
	pthread_mutex_lock(mpool->locker);
	_mpool_clear(mpool);
	pthread_mutex_unlock(mpool->locker);
}

void mpool_debug(void)
{
#ifdef _MM_DEBUG
	printf("alloc : %d, free : %d\n", alloc_cnt, free_cnt);
#endif
}

void 
mpool_init(mpool_t *pool)
{
	memset(pool, 0x0, sizeof(*pool));
	pool->locker = malloc(sizeof(*(pool->locker)));
	pthread_mutex_init(pool->locker, NULL);
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
	pthread_mutex_lock(pool->locker);
	func = _mpool_free_func_alloc(pool);
	pthread_mutex_unlock(pool->locker);

	return func;
}
