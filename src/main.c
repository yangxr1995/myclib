#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "memchk.h"

int main()
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

	return 0;
}

