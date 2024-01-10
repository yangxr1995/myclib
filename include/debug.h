#ifdef __cplusplus
extern "C" {
#endif

#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __HOOK_LIB
#include "hook.h"
#endif

extern FILE *_dlog_fp;
extern char _prg[32];
extern pid_t _prg_pid;

#define __DEBUG_

#ifdef __DEBUG_
#define debug(format, ...) do { \
	__env_init(); \
	fprintf(stdout, "[%s]%s:%s+%d:" format "\n", _prg, __func__, __FILE__, __LINE__,  ## __VA_ARGS__); \
	fprintf(_dlog_fp, "[%s]%s:%s+%d:" format "\n", _prg, __func__, __FILE__, __LINE__,  ## __VA_ARGS__); \
} while(0)
#else
#define debug(format, ...) NULL
#endif

void __attribute__((__no_instrument_function__))show_prg_info(pid_t pid);
void __attribute__((__no_instrument_function__))print_stacktrace();
void __attribute__((__no_instrument_function__))print_stack();
void __attribute__((__no_instrument_function__))get_filename_by_fd(int fd, char *buf, int sz);
void __attribute__((__no_instrument_function__))debug_on();
void __attribute__((__no_instrument_function__))debug_off();
void __attribute__((__no_instrument_function__))__env_init();

static inline void  __attribute__((__no_instrument_function__))
get_prg_name(char *buf, size_t buf_sz, pid_t pid)
{
	char file[128];
	int fd, cnt;

	sprintf(file, "/proc/%d/comm", pid);
	fd = open(file, O_RDONLY);
	cnt = read(fd, buf, buf_sz - 1);
	close(fd);
	buf[cnt-1] = '\0';
}

#endif

#ifdef __cplusplus
}
#endif
