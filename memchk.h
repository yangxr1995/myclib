#ifndef __MEMCHK_H
#define __MEMCHK_H

/*
 * 模块介绍：内存检测
 * 设计目标：检测最常发生的内存使用问题，
 * 包括：内存泄漏，野指针free
 */

#include <stdio.h>

void *mem_alloc(size_t size, const char *file, unsigned int line);
void mem_free(void *ptr, const char *file, unsigned int line);
int mem_leak();
void mem_log(FILE *fp);

#define malloc(size)	mem_alloc(size, __FILE__, __LINE__)
#define free(ptr)	mem_free(ptr, __FILE__, __LINE__)


#endif
