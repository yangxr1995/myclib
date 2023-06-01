#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "list_head.h"
#include "memchk.h"

#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

#ifdef realloc
#undef realloc
#endif

#ifdef calloc
#undef calloc 
#endif

#define hash(p, t) ((unsigned long)(p) >> 3) & (sizeof(t) / sizeof(*(t)) - 1)

typedef struct mem_descriptor_s {
	hlist_node_t list;
	size_t size;
	const char *file;
	unsigned int line;
	struct mem_descriptor_s *next;
	long double *start_space;
	long double *last_space;
	void *ptr;
} mem_descriptor_t;

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

static hlist_head_t free_list;
static hlist_head_t htab[2047];
//static mem_descriptor_t *htab[2047];
FILE *log_fp;

#define RAW_MEM			0x3A
#define INVAILED_MEM	0x3B

void *mem_alloc(size_t size, const char *file, unsigned int line)
{
	unsigned int idx;
	mem_descriptor_t *dp;
	size_t tolsize;
	hlist_node_t *node;

	assert(size > 0);

	
	if (hlist_empty(&free_list)) {
		dp = (mem_descriptor_t *) malloc(sizeof(mem_descriptor_t));
	}
	else {
		node = hlist_push(&free_list);
		assert(node != NULL);
		dp = hlist_entry(node, typeof(*dp), list);
	}

	dp->size = size;
	dp->file = file;
	dp->line = line;

	tolsize = size + sizeof(*(dp->start_space)) + sizeof(*(dp->last_space));
	dp->start_space = malloc(tolsize);
	memset(dp->start_space, RAW_MEM, tolsize);
	dp->ptr = (void *)((void *)(dp->start_space) + sizeof(*(dp->start_space)));
	dp->last_space = (long double *)((void *)(dp->ptr) + size);
	INIT_HLIST_NODE(&dp->list);
	idx = hash(dp->ptr, htab);

	hlist_add_head(&dp->list, htab + idx);
 
	return dp->ptr;
}

inline static int is_dirty(void *ptr, size_t size)
{
	int i;
	unsigned char *pch;
	for (i = 0, pch = (unsigned char *)ptr; i < size; i++) {
		if (pch[i] != RAW_MEM) {
			return 1;
		}
	}
	return 0;
}

const char *str_mem(void *ptr, size_t size)
{
	static char str[1024], bstr[8];
	int i;
	unsigned char *pch;

	assert(size*2 < sizeof(str));

	memset(str, 0x0, sizeof(str));
	for (i = 0, pch = (unsigned char *)ptr; i < size; i++) {
		sprintf(bstr, "%X ", pch[i]);
		strcat(str, bstr);
	}

	return str;
}

inline static int dp_dirty(mem_descriptor_t *dp)
{
	FILE *fp;
	int ret;

	fp = log_fp ? log_fp : stderr;

	ret = 0;
	if (is_dirty(dp->start_space, sizeof(*(dp->start_space)))) {
		fprintf(fp, "Mem out of Upper bounry. mem at 0x%p, size is %d, alloced from %s +%d\n",
				dp->ptr, (int)dp->size, dp->file, dp->line);	
		fprintf(fp, "Dirty Mem peak : %s\n", str_mem(dp->start_space, sizeof(*(dp->start_space))));

		ret = 1;
	}
	if (is_dirty(dp->last_space, sizeof(*(dp->last_space)))) {
		fprintf(fp, "Mem out of Bottom bounry. mem at 0x%p, size is %d, alloced from %s +%d\n",
				dp->ptr, (int)dp->size, dp->file, dp->line);	
		fprintf(fp, "Dirty Mem peak : %s\n", str_mem(dp->last_space, sizeof(*(dp->last_space))));
		ret = 1;
	}
	return ret;
}

static int _mem_free(void *ptr)
{
	unsigned int idx;
	mem_descriptor_t *dp, *prev;

	idx = hash(ptr, htab);

	if (hlist_empty(htab + idx))
		return -1;
	
	mem_descriptor_t *tpos; 
	hlist_node_t *pos, *n;

	hlist_for_each_entry_safe(tpos, pos, n, htab + idx, list) {
		if (tpos->ptr == ptr) {
			free(tpos->start_space);
			hlist_del_init(&tpos->list);
			hlist_add_head(&tpos->list, &free_list);
			return 0;
		}		
	}

	return -1;
}

void mem_free(void *ptr, const char *file, unsigned int line)
{
	FILE *fp;

	assert(ptr != NULL);

	fp = log_fp ? log_fp : stderr;
	if (_mem_free(ptr) < 0) {
		fprintf(fp, "Error free invailed mem %p, error at %s +%d\n", ptr, file, line);
		exit(1);
	}
}

int mem_leak()
{
	unsigned int i, count;
	FILE *fp;

	fp = log_fp ? log_fp : stderr;

	for (i = 0, count = 0; i < sizeof(htab)/sizeof(*htab); i++) {
		mem_descriptor_t *tpos;
		hlist_node_t *pos;
		hlist_for_each_entry(tpos, pos, htab + i, list) {
			dp_dirty(tpos);
			count++;
			fprintf(fp, "Unfree memory at %p, size %ld , allced from %s +%d\n", 
					tpos->ptr, tpos->size, tpos->file, tpos->line);
		}
	}
	if (count == 0)
		fprintf(fp, "No memory leak\n");
	else
		fprintf(fp, "Memory leak : %d block\n", count);

	return count;
}

void mem_log(FILE *fp)
{
	if (fp)
		log_fp = fp;
	else
		log_fp = stderr;

	if (log_fp)
		fprintf(fp, "\n\nStart to check ...\n");

}

void *mem_calloc(size_t nmemb, size_t size, const char *file, unsigned int line)
{
	void *ptr;
	size_t tolsize;

	tolsize = nmemb * size;	
	assert(tolsize > 0);

	ptr = mem_alloc(tolsize, file, line);
	memset(ptr, 0x0, tolsize);
	return ptr;
}

void *mem_realloc(void *ptr, size_t size, const char *file, unsigned int line)
{
	FILE *fp;

	assert(ptr != NULL);
	assert(size > 0);

	fp = log_fp ? log_fp : stderr;

	if (_mem_free(ptr) < 0) {
		fprintf(fp, "Realloc error, mem at 0x%p, alloced from %s +%d\n", ptr, file, line);
		exit(1);
	}
	return mem_alloc(size, file, line);	
}
