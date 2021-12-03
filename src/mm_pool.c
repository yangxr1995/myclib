#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "mm_pool.h"
//#include "memchk.h"

static int alloc_cnt;
static int free_cnt;

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

	alloc_cnt++;

	mpool = malloc(sizeof(struct mpool));
	mpool->prev = NULL;
	mpool->avail = NULL;
	mpool->limit = NULL;

	return mpool;
}

void
mpool_do_free_list(struct mpool *mpool)
{
	mpool_free_func_t *func;

	for (func = mpool->free_list; func; func = func->next) {
		func->free_func(func->data);
	}
	mpool->free_list = NULL;
}

void 
mpool_destroy(struct mpool **mpool) 
{
	struct mpool *ptr, *tmp;

	if (mpool == NULL)
		return;
	ptr = *mpool;
	if (ptr == NULL)
		return;

	mpool_do_free_list(*mpool);

	do {
		tmp = ptr->prev;
		free_cnt++;
		free(ptr);
		ptr = tmp;
	} while (ptr);
}

void *
mpool_alloc(struct mpool *mpool, size_t nbytes) 
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
		alloc_cnt++;
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

void 
mpool_free(struct mpool *mpool)
{
	struct mpool *ptr, *tmp;

	assert(mpool);

	ptr = mpool->prev;
	if (ptr == NULL)
		return;

	mpool_do_free_list(mpool);

	do {
		tmp = ptr->prev;
		free_cnt++;
		free(ptr);
		ptr = tmp;
	} while (ptr);
}

void 
mpool_clear(struct mpool *mpool)
{
	struct mpool *pos;

	if (mpool == NULL)
		return;

	mpool_do_free_list(mpool);

	for (pos = mpool; pos; pos = pos->prev) {
		if (pos->avail) {
			pos->avail = ((char *)pos->prev) + sizeof(union header);
		}
	}
}

void mpool_debug(void)
{
	printf("alloc : %d, free : %d\n", alloc_cnt, free_cnt);
}

void 
mpool_init(mpool_t *pool)
{
	memset(pool, 0x0, sizeof(*pool));
}


mpool_free_func_t *
mpool_free_func_alloc(mpool_t *pool)
{
	mpool_free_func_t *func;
	func = mpool_alloc(pool, sizeof(*func));
	memset(func, 0x0, sizeof(*func));

	func->next = pool->free_list;
	pool->free_list = func;

	return func;
}
