#include <stdlib.h>
#include <string.h>

#include "arr.h"

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
		a->nalloc = 2 * size;
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


