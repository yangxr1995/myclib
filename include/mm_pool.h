#pragma once

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct mpool_free_func_s mpool_free_func_t;
typedef struct mpool mpool_t;

extern struct mpool *mpool_new(void);
extern void mpool_destroy(struct mpool **mpool);
extern void *mpool_alloc(struct mpool *mpool, size_t nbytes);
extern void mpool_free(struct mpool *mpool);
extern void mpool_clear(struct mpool *mpool);

extern mpool_free_func_t *mpool_free_func_alloc(mpool_t *pool);
extern void mpool_do_free_list(struct mpool *mpool);

inline static char *strdup_mp(const char *ptr, mpool_t *mp)
{
	char *data;
	int len = strlen(ptr);
	data = (char *)mpool_alloc(mp, len + 1);
	memcpy(data, ptr, len);
	data[len] = '\0';
	return data;
}

