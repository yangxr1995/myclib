#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "assert.h"

const char *str_mem(void *ptr, size_t size);

void func2()
{

	assert(0 && "bad bad");
}

void func()
{
	func2();
}

int main(int argc, const char *argv[])
{
	func();
	return 0;
}
