#ifndef __ARR_H__
#define __ARR_H__

#include <stdlib.h>

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




#endif

