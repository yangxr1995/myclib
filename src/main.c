#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "memchk.h"
#include "fmt.h"
#include "arena.h"

const char *str_mem(void *ptr, size_t size);


int main(int argc, const char *argv[])
{
	FILE *fp;
	fp = fopen("./log", "a");
	mem_log(fp);

	Arena_T ar;
	ar = Arena_new();
	char *ptr = Arena_alloc(ar, 100, __FILE__, __LINE__);
	memset(ptr, 0x0, 100);
	Arena_free(ar);
	Arena_dispose(&ar);

	mem_leak();

	return 0;
}
