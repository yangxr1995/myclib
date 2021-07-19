#include <stdlib.h>
#include <stdio.h>

#include "memchk.h"

#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

typedef struct mem_descriptor_s {
	void *ptr;
	size_t size;
	const char *file;
	unsigned int line;
	struct mem_descriptor_s *next;
} mem_descriptor_t;

static mem_descriptor_t *htab[2047];

union align {
	int i;
	long l;
	long *lp;
	void *p;
	void (*fp)(void);
	float f;
	double d;
	long double ld;
};

#define hash(p, t) ((unsigned long)(p) >> 3) & (sizeof(t) / sizeof(*(t)) - 1)


void *mem_alloc(size_t size, const char *file, unsigned int line)
{
	size_t alloc_size;
	void *ptr;
	unsigned int idx;
	mem_descriptor_t *dp;

	alloc_size = (size + sizeof(mem_descriptor_t) + 
			sizeof(union align)) / sizeof(union align) * sizeof(union align);

	dp = (mem_descriptor_t *) malloc(alloc_size);

	dp->size = size;
	dp->file = file;
	dp->line = line;
	
	dp->ptr = (void *) (((unsigned long)dp + sizeof(mem_descriptor_t) + 
				sizeof(union align) - 1) / sizeof(union align) * sizeof(union align));
	idx = hash(dp->ptr, htab);
	dp->next = htab[idx];		
	htab[idx] = dp;
 
	return dp->ptr;
}

void mem_free(void *ptr, const char *file, unsigned int line)
{
	unsigned int idx;
	mem_descriptor_t *dp, *tmp;

	idx = hash(ptr, htab);

	if (htab[idx] == NULL) {
		fprintf(stderr, "error free %p, %s +%d\n", ptr, file, line);
		exit(1);
	}

	if (htab[idx]->ptr == ptr) {
		htab[idx] = htab[idx]->next;
		free(htab[idx]);
		return;
	}

	for (tmp = htab[idx], dp = htab[idx]->next; dp; tmp = dp, dp = dp->next) {
		if (dp->ptr == ptr) {
			tmp->next = dp->next;
			free(dp);
			return;
		}
	}
	
	fprintf(stderr, "error free %p, %s +%d\n", ptr, file, line);
	exit(1);
}

int mem_leak()
{
	mem_descriptor_t *dp;
	unsigned int i, count;

	for (i = 0, count = 0; i < sizeof(htab)/sizeof(*htab); i++) {
		for (dp = htab[i]; dp; dp = dp->next) {
			count++;
			fprintf(stderr, "Unfree memory in %p, size %ld , malloc from %s +%d\n", 
					dp->ptr, dp->size, dp->file, dp->line);
		}
	}
	if (count == 0)
		fprintf(stderr, "No memory leak\n");
	else
		fprintf(stderr, "Memory leak : %d block\n", count);

	return 0;	
}
