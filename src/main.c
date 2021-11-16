#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "except.h"
#include "memchk.h"

static void test_mem();
static void test_expect();

int main()
{
	test_expect();
	return 0;
}


static void test_mem()
{
	char *ptr = NULL;
	FILE *fp;

	fp = fopen("./log", "a");
	mem_log(fp);

	assert("result " && 1);

	ptr = malloc(100);
	strcpy(ptr, "hello world\n");
	printf("ptr : %s", ptr);
	ptr = realloc(ptr, 1000);
	ptr = calloc(11, 11);
	mem_leak();

	fclose(fp);
}

struct Except_T E1 = {.reason = "except e1 for test"};

static void 
test_expect_func()
{
	RAISE(E1);
}

static void 
test_expect()
{
TRY
	test_expect_func();
END_TRY;
}
