#ifndef __ARR_H__
#define __ARR_H__

#include <stdlib.h>
#include "mm_pool.h"

typedef struct arr_s arr_t;

struct arr_s {
	void 			*elts;
	unsigned int 	nelts;
	unsigned int	size;
	unsigned int	nalloc;
};


void *arr_push(arr_t *a);
arr_t *arr_create(unsigned int n, unsigned int size);
int arr_init(arr_t *a, unsigned int n, unsigned int size);
void arr_destroy(arr_t *a);
int arr_del(arr_t *a, void *val, int (*cmp)(void *, void *));

char *arr_to_buf_mp(arr_t *arr, char **pbuf, int *buf_sz, mpool_t *mp);

#define arr_for_each(arr, pos) \
	for (pos = (typeof(pos))((arr)->elts); pos != (typeof(pos))((arr)->elts) + (arr)->nelts; pos++)

#define is_arr_end(arr, pos) \
    (pos == ((typeof(pos))((arr)->elts) + (arr)->nelts))

inline static void
arr_clear(arr_t *arr)
{
    arr->nelts = 0;
}

static int 
cmp_ptr(void *a, void *b)
{
    return *(void **)a == *(void **)b ? 0 : 1;
}

#endif

