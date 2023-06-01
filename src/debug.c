#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "debug.h"

#define GET_ITEM(name, line)   _GET_ITEM(#name, name##_buf, line)
	
#define _GET_ITEM(name, buf, line) do { \
	char *_p, *_p2; \
	if (buf[0] == 0 && (_p = strstr(line, name))) { \
		for (_p += strlen(name) + 1; isspace(*_p); _p++) \
			NULL; \
		for (_p2 = _p + strlen(_p) - 1; _p2 > _p && isspace(*_p2); _p2--) \
			NULL;	 \
		strncpy(buf, _p, _p2 - _p + 1); \
	} \
} while(0)


#define PRINT_LOG(fmt, ...) do{ \
	if (_prg[0] == '\0') \
		get_prg_name(_prg, sizeof(_prg)); \
	if (_prg_pid == 0) \
		_prg_pid = getpid(); \
	if (_dlog_fp == NULL) { \
		char _filename[256]; \
		snprintf(_filename, sizeof(_filename), "/var/run/%s-%d.log", _prg, _prg_pid); \
		_dlog_fp = fopen(_filename, "a"); \
	} \
	fprintf(_dlog_fp, "[%s]" fmt, _prg, ## __VA_ARGS__); \
	fprintf(stdout, "[%s]" fmt, _prg, ## __VA_ARGS__); \
} while (0)


FILE *_dlog_fp;
char _prg[32];
pid_t _prg_pid;

int __attribute__((__no_instrument_function__))
_show_prg_info(pid_t pid)
{
	char file[128], line[256];
	FILE *fp;

	if (pid == 1)
		return 0;

	sprintf(file, "/proc/%d/status", pid);
	fp = fopen(file, "r");
	if (!fp) {
		perror("open proc status file");
		return -1;
	}

	char Name_buf[32] = {0};
	char PPid_buf[32] = {0};
	while (fgets(line, sizeof(line), fp)) {
		GET_ITEM(Name, line);
		GET_ITEM(PPid, line);
	}

	fclose(fp);

	sprintf(file, "/proc/%d/cmdline", pid);
	fp = fopen(file, "r");
	if (!fp) {
		perror("open proc cmdline");
		return -1;
	}
	
	char cmdline[1024] = {0}, *p1, *p2, *p3;
	int n;

	p1 = cmdline;
	p2 = p1 + sizeof(cmdline);
	while ((n = fread(p1, sizeof(char), p2 - p1 - 1, fp))) {
		for (p3 = p1; p3 < p2; p3++) {
			if (*p3 == '\0') {
				if (!isspace(p1[-1]))
					*p1++ = ' ';
			}
			else {
				if (!isspace(*p3))
					*p1++ = *p3;
				else if (!isspace(p1[-1]))
					*p1++ = *p3;
			}
		}
	}
	
	fclose(fp);

	PRINT_LOG("Name : %s,\tPid : %d,\tPPid : %s,\tcmdline : %s\n"
			, Name_buf, pid, PPid_buf, cmdline);	

	_show_prg_info(atoi(PPid_buf));

	return 0;
}

void __attribute__((__no_instrument_function__))
show_prg_info(pid_t pid)
{
	PRINT_LOG("############## Program Info ##############\n");
	_show_prg_info(pid);
	PRINT_LOG("\n");
}

void __attribute__((__no_instrument_function__))
print_stacktrace()
{
    int size = 16;
    void * array[16];
    int stack_num = backtrace(array, size);
    char ** stacktrace = backtrace_symbols(array, stack_num);
	PRINT_LOG("########## stack info   #############\n");
    for (int i = 1; i < stack_num; ++i)
    {
        PRINT_LOG("%s\n", stacktrace[i]);
    }
	PRINT_LOG("\n");
    free(stacktrace);
}

void __attribute__((__no_instrument_function__))
get_filename_by_fd(int fd, char *buf, int sz)
{
	char proc_item[256];
	snprintf(proc_item, sizeof(proc_item) - 1, "/proc/self/fd/%d", fd);
	readlink(proc_item, buf, sz);
}

void __attribute__((__no_instrument_function__))
__cyg_profile_func_enter(void *this, void *call)
{
	PRINT_LOG("Enter\n%p\n%p\n", call, this);
}

void __attribute__((__no_instrument_function__))
__cyg_profile_func_exit(void *this, void *call)
{
	PRINT_LOG("Exit\n%p\n%p\n", call, this);
}
