#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <execinfo.h>
#include <ctype.h>
#include <assert.h>

#include "debug.h"

#ifdef open
#undef open
#endif

#define HOOK_LOG    "/var/log/hook.log"

/*
 *  LD_PRELAOD=./hook.so a.out
 *  可以使用 LD_DEBUG=symbols  LD_PRELAOD=./hook.so a.out
 *  确定动态连接器使用为每个符号使用哪个lib
 *  使用 LD_DEBUG=help 查看其他调试方法
 */

// extern for debug.c
OPEN open_orign;

static void *libc_handle = NULL;	

static int hook_init()
{
	static int log_fd;

	if (libc_handle == NULL)
		libc_handle = dlopen("libc.so.6", RTLD_LAZY); 

	if (open_orign == NULL)
		open_orign = (OPEN)dlsym(libc_handle, "open");

	if (log_fd == 0) {
		if ((log_fd = open_orign(HOOK_LOG,
						O_CREAT | O_APPEND | O_RDWR, 0644)) < 0) {
			printf("open "HOOK_LOG" err : %s\n", strerror(errno));
			goto _err_;
		}
	}

	if (log_fd > 0 && (_dlog_fp == NULL || _dlog_fp == stderr)) {
		if ((_dlog_fp = fdopen(log_fd, "a+")) == NULL) {
			printf("fdopen error : %s", strerror(errno));
			goto _err_;
		}
	}

	return 0;

_err_:
	if (_dlog_fp == NULL)
		_dlog_fp = stderr;

	return -1;
}

/******************** hook open *********************/
static OPEN old_open;

inline static int 
is_target(const char *pathname)
{
	if (strstr(pathname, "./111") == NULL)
		return 0;
	
	return 1;
}

int open(const char *pathname,int flags, ...) 
{
	mode_t mode = 0;
	va_list ap;

	if (is_target(pathname)) {
		hook_init();
		debug("%s %s", __func__, pathname);
		show_prg_info(getpid());
	}

	if (libc_handle == NULL)
		libc_handle = dlopen("libc.so.6", RTLD_LAZY); 

	if (old_open == NULL)
		old_open = (OPEN)dlsym(libc_handle, "open");

	if (flags & O_CREAT) {
		va_start(ap, flags);	
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	if (mode == 0)
		return old_open(pathname, flags);
	else
		return old_open(pathname, flags, mode);
}
/******************** hook open end *********************/

/******************** hook open64 *********************/
static OPEN old_open64;

int open64(const char *pathname,int flags, ...) 
{
	mode_t mode = 0;
	va_list ap;

	if (is_target(pathname)) {
		hook_init();
		debug("%s %s", __func__, pathname);
		show_prg_info(getpid());
	}

	if (flags & O_CREAT) {
		va_start(ap, flags);	
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	if (libc_handle == NULL)
		libc_handle = dlopen("libc.so.6", RTLD_LAZY); 

	if (old_open64 == NULL)
		old_open64 = (OPEN)dlsym(libc_handle, "open64");

	if (flags & O_CREAT)
		return old_open64(pathname, flags, mode);
	else
		return old_open64(pathname, flags);
}
/******************** hook open64 end *********************/
