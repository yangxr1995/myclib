#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "memchk.h"
#include "assert.h"
#include "mm_pool.h"
#include "fmt.h"

void func2()
{
	assert(0);	
}


void func1();


int main(int argc, const char *argv[])
{
	mpool_t *pool;	

//	func1();

	pool = mpool_new();

	for (int i = 0; i < 10; i++) {
		mpool_alloc(pool, 200);
	}
	mpool_clear(pool);
	for (int i = 0; i < 10; i++) {
		mpool_alloc(pool, 200);
	}
	mpool_alloc(pool, 666666);

	mpool_debug();

	mpool_destroy(&pool);

	mpool_debug();
	mem_leak();

	func1();

	str_t a = str_new("hello world\n");
	printf("aaa %v\n", &a);

	return 0;
}

void func1()
{
	func2();
}
