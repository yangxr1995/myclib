#include <string.h>
#include <execinfo.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "logger.h"

void print_stacktrace2()
{
    int size = 16;
    void * array[16];
    int stack_num = backtrace(array, size);
    char ** stacktrace = backtrace_symbols(array, stack_num);
	char info[1024] = {0};

	strcat(info, "stack : \n");

    for (int i = 1; i < stack_num; ++i)
    {
		strcat(info, stacktrace[i]);
		strcat(info, "\n");
    }
	strcat(info, "\n");

	log_message(ERR_LOG, "%s", info); 

    free(stacktrace);
}

void 
sigsegv_handler(int signum, siginfo_t* info, void*ptr)
{
	static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};
	int i;
	ucontext_t *ucontext = (ucontext_t*)ptr;
	void *bt[100];
	char **strings;
	char buf[1024] = {0}, tmp[128] = {0};

	sprintf(tmp, "Segmentation Fault Trace:\n");
	strcat(buf, tmp);
	sprintf(tmp, "info.si_signo = %d\n", signum);
	strcat(buf, tmp);
	sprintf(tmp, "info.si_errno = %d\n", info->si_errno);
	strcat(buf, tmp);
	sprintf(tmp, "info.si_code  = %d (%s)\n", info->si_code, si_codes[info->si_code]);
	strcat(buf, tmp);
	sprintf(tmp, "info.si_addr  = %p\n", info->si_addr);
	strcat(buf, tmp);

#ifndef ENABLE_DEBUG_GPS
	sprintf(tmp, "fp[0x%3x] ",ucontext->uc_mcontext.arm_fp);
	strcat(buf, tmp);
	sprintf(tmp, "sp[0x%3x] ",ucontext->uc_mcontext.arm_sp);
	strcat(buf, tmp);
	sprintf(tmp, "lr[0x%3x] ",ucontext->uc_mcontext.arm_lr);
	strcat(buf, tmp);
	sprintf(tmp, "pc[0x%3x] ",ucontext->uc_mcontext.arm_pc);
	strcat(buf, tmp);
	sprintf(tmp, "fault_address[0x%3x]\n",ucontext->uc_mcontext.fault_address);
	strcat(buf, tmp);
#endif
	log_message(ERR_LOG, "----------\n%s", buf);
	print_stacktrace2();

	exit(1);
}

int _assert()
{
	print_stacktrace2();
	exit(1);
	return 0;
}
