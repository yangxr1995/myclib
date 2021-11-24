#ifndef _LOGGER_H
#define _LOGGER_H


#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>

#include "config.h"

#ifdef ENABLE_LOG_TO_FILE
#include <sys/stat.h>
#endif


#define	MAX_LOG_MSG	255

#ifdef ENABLE_LOG_TO_FILE
extern const char *log_file_name;
#endif

extern void enable_console_log(void);
#ifdef ENABLE_LOG_TO_FILE
extern void set_flush_log_file(void);
extern void close_log_file(void);
extern void open_log_file(const char *);
extern void flush_log_file(void);
extern void update_log_file_perms(mode_t);
#endif
extern void vlog_message(const int facility, const char* format, va_list args)
	__attribute__ ((format (printf, 2, 0)));
extern void log_message(int priority, const char* format, ...)
	__attribute__ ((format (printf, 2, 3)));
extern void conf_write(FILE *fp, const char *format, ...)
	__attribute__ ((format (printf, 2, 3)));


#define ERR_LOG		0
#define WARN_LOG	1
#define INFO_LOG	2

#endif
