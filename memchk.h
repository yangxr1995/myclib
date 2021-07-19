#ifndef __MEMCHK_H
#define __MEMCHK_H

void *mem_alloc(size_t size, const char *file, unsigned int line);
void mem_free(void *ptr, const char *file, unsigned int line);
int mem_leak();

#define malloc(size)	mem_alloc(size, __FILE__, __LINE__)
#define free(ptr)	mem_free(ptr, __FILE__, __LINE__)


#endif
