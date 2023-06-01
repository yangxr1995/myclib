#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

extern FILE *_dlog_fp;
extern char _prg[32];
extern pid_t _prg_pid;

#define __DEBUG_

#ifdef __DEBUG_
#define debug(format, ...) do { \
	if (_prg[0] == '\0') \
		get_prg_name(_prg, sizeof(_prg)); \
	if (_prg_pid == 0) \
		_prg_pid = getpid(); \
	if (_dlog_fp == NULL) { \
		char _filename[256]; \
		snprintf(_filename, sizeof(_filename), "/var/run/%s-%d.log", _prg, _prg_pid); \
		_dlog_fp = fopen(_filename, "a"); \
	} \
	fprintf(stdout, "[%s]%s:%s+%d:" format "\n", _prg, __func__, __FILE__, __LINE__,  ## __VA_ARGS__); \
	fprintf(_dlog_fp, "[%s]%s:%s+%d:" format "\n", _prg, __func__, __FILE__, __LINE__,  ## __VA_ARGS__); \
} while(0)
#else
#define debug(format, ...) NULL
#endif

void __attribute__((__no_instrument_function__))show_prg_info(pid_t pid);
void __attribute__((__no_instrument_function__))print_stacktrace();
void __attribute__((__no_instrument_function__))get_filename_by_fd(int fd, char *buf, int sz);

static inline void  __attribute__((__no_instrument_function__))
get_prg_name(char *buf, size_t buf_sz)
{
	char file[128];
	pid_t pid;
	int fd, cnt;

	pid = getpid();
	sprintf(file, "/proc/%d/comm", pid);
	fd = open(file, O_RDONLY);
	cnt = read(fd, buf, buf_sz - 1);
	buf[cnt-1] = '\0';
}

#endif
