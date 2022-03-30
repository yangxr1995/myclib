#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "memchk.h"
#include "assert.h"
#include "palloc.h"
#include "fmt.h"

int main(int argc, const char *argv[])
{
	mpool_t *pool;	

	pool = mpool_create(1024);

	for (int i = 0; i < 10; i++) {
		mpool_alloc(pool, 200);
	}
	mpool_clear(pool);
	for (int i = 0; i < 10; i++) {
		mpool_alloc(pool, 200);
	}
	mpool_alloc(pool, 666666);

	mpool_destory(pool);
	mem_leak();

	str_t a = str_new("hello world\n");
	printf("aaa %v\n", &a);

	return 0;
}
