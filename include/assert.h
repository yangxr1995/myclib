#ifndef __ASSERT_H_
#define __ASSERT_H_

#include <stdio.h>
#include <signal.h>


#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else

#include "logger.h"
int _assert(void);
#define assert(e) ((void)((e)|| (log_message(ERR_LOG, "assert at %s +%d : %s\n", __FILE__, __LINE__, #e), 1) && _assert()))

#endif

void sigsegv_handler(int signum, siginfo_t* info, void*ptr);
void print_stacktrace2();

#endif
