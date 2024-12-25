#include "debug.h"

#if 0
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    void *ptr;
    ptr = __real_malloc(size);
    // TODO:
    /*mem_alloc(ptr, size, print_nobase_addr((void *)__builtin_return_address(0) - sizeof(void *)));*/
    return ptr;
}

void __real_free(void *ptr);
void __wrap_free(void *ptr)
{
    // TODO:
    /*mem_free(ptr, print_nobase_addr((void *)__builtin_return_address(0) - sizeof(void *)));*/
    __real_free(ptr);
}
#endif
