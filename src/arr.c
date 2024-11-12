#include <stdlib.h>
#include <string.h>

#include "arr.h"
#include "mm_pool.h"

char *arr_to_buf_mp(arr_t *arr, char **pbuf, int *buf_sz, mpool_t *mp)
{
    char *buf, *to, *from;
    int sz;
    sz = arr->nelts * arr->size + sizeof(unsigned short);
    buf = mpool_alloc(mp, sz);
    *(unsigned short *)buf = arr->nelts;
    to = (char *)buf + sizeof(unsigned short);
    from = arr->elts;
    memcpy(to, arr->elts, arr->nelts * arr->size);
    *pbuf = buf;
    *buf_sz = sz;
    return buf;
}

int 
arr_init(arr_t *a, unsigned int n, unsigned int size)
{
	a->size = size;
	a->nelts = 0;
	a->nalloc = n;
	a->elts = malloc(n * size);
	if (a->elts == NULL)
		return -1;

	return 0;
}

arr_t *
arr_create(unsigned int n, unsigned int size)
{
	arr_t *a;	

	a = malloc(sizeof(*a));
	a->elts = malloc(n * size);
	a->nelts = 0;
	a->size = size;
	a->nalloc = n;

	return a;
}

void *
arr_push(arr_t *a)
{
	unsigned int size;
	void *elt, *new;

	if (a->nelts == a->nalloc)	{
		size = a->size * a->nalloc;

		new = malloc(2 * size);	
		if (new == NULL)
			return NULL;
		memcpy(new, a->elts, size);
		free(a->elts);
		a->elts = new;
		a->nalloc *= 2;
	}

	elt = (char *)a->elts + a->size * a->nelts;
	a->nelts++;

	return elt;
}

void arr_destroy(arr_t *a)
{
	free(a->elts);
	free(a);	
}

int
arr_del(arr_t *a, void *val, int (*cmp)(void *, void *))
{
    char *p, *end;

    for (p = a->elts, end = a->elts + a->nelts * a->size; p != end; p += a->size) {
        if (cmp(p, val) == 0) {
            memcpy(p, p + a->size, end - (p + a->size));
            --a->nelts;
            return 1;
        }
    }

    return 0;
}

