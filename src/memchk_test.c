#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "memchk.h"

int main()
{
	char *ptr = NULL;
	FILE *fp;
	int i, count = 0xfff;

	fp = fopen("./log", "a");
	mem_log(fp);

	assert("result " && 1);

	ptr = malloc(100);
	for (i = 0; i < count; i++) {
	ptr = realloc(ptr,i+1);
	}

	mem_leak();

	fclose(fp);

	return 0;
}

