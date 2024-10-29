#ifndef _LOGGER_H
#define _LOGGER_H


#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define ENABLE_LOG_TO_FILE 1

#ifdef ENABLE_LOG_TO_FILE
#include <sys/stat.h>
#endif


#define	MAX_LOG_MSG	2047

extern const char *log_file_name;

extern void enable_console_log(void);
extern void set_flush_log_file(void);
extern void open_log_file(const char *);
extern void flush_log_file(void);
extern void update_log_file_perms(mode_t);
extern void vlog_message(const int facility, const char* format, va_list *args);
extern void log_message(int priority, const char* format, ...);
extern void conf_write(FILE *fp, const char *format, ...);

extern void set_max_log_level(int level);
extern void set_max_log_line(unsigned int line);

#define ERR_LOG		0
#define WARN_LOG	1
#define INFO_LOG	2
#define DEBUG_LOG	3

#define	log_err(fmt, ...) do { \
	char msg[1024] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	if (errno != 0) \
		log_message(ERR_LOG, "%s[%d] : %s : %s", __func__, __LINE__, msg, strerror(errno)); \
	else \
		log_message(ERR_LOG, "%s[%d] : %s", __func__, __LINE__, msg); \
	errno = 0; \
}  while(0)

#define	log_debug(fmt, ...) do { \
	char msg[1024] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	log_message(DEBUG_LOG, "%s[%d] : %s", __func__, __LINE__, msg); \
}  while(0)

#define	log_warn(fmt, ...) do { \
	char msg[1024] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	log_message(WARN_LOG, "%s[%d] : %s", __func__, __LINE__, msg); \
}  while(0)

#define	log_info(fmt, ...) do { \
	char msg[1024] = {0}; \
	snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__); \
	log_message(INFO_LOG, "%s[%d] : %s", __func__, __LINE__, msg); \
}  while(0)

#endif
