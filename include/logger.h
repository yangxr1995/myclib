#ifndef _LOGGER_H
#define _LOGGER_H


#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>
#include "fmt.h"


#define ENABLE_LOG_TO_FILE 1

#ifdef ENABLE_LOG_TO_FILE
#include <sys/stat.h>
#endif


#define	MAX_LOG_MSG	2047

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
extern void vlog_message(const int facility, const char* format, va_list *args);
extern void log_message(int priority, const char* format, ...);
extern void conf_write(FILE *fp, const char *format, ...);

extern void set_max_log_level(int level);

#define ERR_LOG		0
#define WARN_LOG	1
#define INFO_LOG	2
#define DEBUG_LOG	3

#define	log_err(fmt, ...) do { \
	char msg[256] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	log_message(ERR_LOG, "%s : %s : %s", __func__, msg, strerror(errno)); \
	errno = 0; \
}  while(0)

#define	log_debug(fmt, ...) do { \
	char msg[256] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	log_message(DEBUG_LOG, "%s : %s", __func__, msg); \
}  while(0)

#define	log_warn(fmt, ...) do { \
	char msg[256] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	log_message(WARN_LOG, "%s : %s", __func__, msg); \
}  while(0)

#define	log_info(fmt, ...) do { \
	char msg[256] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	log_message(INFO_LOG, "%s : %s", __func__, msg); \
}  while(0)

#endif
