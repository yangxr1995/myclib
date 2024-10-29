#include <alloca.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

#include "debug.h"

#include <stdio.h>

void func() {
    printf("Return address of func: %p\n", 
            print_nobase_addr((void *)__builtin_return_address(0) - sizeof(void *)));
}

int main() {
    /*get_task_maps();*/
    /*show_prg_info(getpid());*/
    func();
    return 0;
}
