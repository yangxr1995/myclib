#include <execinfo.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void print_stacktrace2()
{
    int size = 16;
    void * array[16];
    int stack_num = backtrace(array, size);
    char ** stacktrace = backtrace_symbols(array, stack_num);
	printf("stack : \n");
    for (int i = 1; i < stack_num; ++i)
    {
        printf("%s\n", stacktrace[i]);
    }
	printf("\n");
    free(stacktrace);
}

int _assert()
{
	print_stacktrace2();
	abort();
	return 0;
}
