#include "mm_pool.h"

#include <stdio.h>

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
mpool_destroy(struct mpool **mpool) 
{
	struct mpool *ptr, *tmp;

	if (mpool == NULL)
		return;
	ptr = *mpool;
	if (ptr == NULL)
		return;

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
	struct mpool *head, *pos;

	if (mpool == NULL)
		return;

	head = mpool->prev;

	for (pos = head->prev; pos; pos = pos->prev) {
		free_cnt++;
		free(pos);
	}

	return;
}

void 
mpool_clear(struct mpool *mpool)
{
	struct mpool *pos;

	if (mpool == NULL)
		return;

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
