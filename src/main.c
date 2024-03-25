#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "memchk.h"
#include "assert.h"
#include "mm_pool.h"
#include "fmt.h"
#include "debug.h"
#include "logger.h"
#include "timer_list.h"
#include "thread_pool.h"
#include "task.h"

static void sigsegv_handler(int signum, siginfo_t* info, void*ptr)
{
	static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};
	int i;
	ucontext_t *ucontext = (ucontext_t*)ptr;
	void *bt[100];
	char **strings;

	printf("Segmentation Fault Trace:\n");
	printf("info.si_signo = %d\n", signum);
	printf("info.si_errno = %d\n", info->si_errno);
	printf("info.si_code  = %d (%s)\n", info->si_code, si_codes[info->si_code]);
	printf("info.si_addr  = %p\n", info->si_addr);

	/*for arm*/
	printf("the arm_fp 0x%3x\n",ucontext->uc_mcontext.arm_fp);
	printf("the arm_ip 0x%3x\n",ucontext->uc_mcontext.arm_ip);
	printf("the arm_sp 0x%3x\n",ucontext->uc_mcontext.arm_sp);
	printf("the arm_lr 0x%3x\n",ucontext->uc_mcontext.arm_lr);
	printf("the arm_pc 0x%3x\n",ucontext->uc_mcontext.arm_pc);
	printf("the arm_cpsr 0x%3x\n",ucontext->uc_mcontext.arm_cpsr);
	printf("the falut_address 0x%3x\n",ucontext->uc_mcontext.fault_address);

	printf("Stack trace (non-dedicated):");
	int sz = backtrace(bt, 20);
	printf("the stack trace is %d\n",sz);
	strings = backtrace_symbols(bt, sz);
	for(i = 0; i < sz; ++i)
	{
		printf("%s\n", strings[i]);
	}
	_exit (-1);
}


static int task_main(int argc, char **argv)
{    
	char array[2] = {0};

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = sigsegv_handler;
	action.sa_flags = SA_SIGINFO;
	if(sigaction(SIGSEGV, &action, NULL) < 0)
	{
		perror("sigaction");
	}

	*(int *)0x1 = 1;

	return 0;
}

int main(int argc, char **argv)
{
	enable_console_log();
	set_max_log_level(DEBUG_LOG);
	log_message(INFO_LOG, "pid %d", getpid());
	task_start(task_main, argc, argv);

	return 0;
}
