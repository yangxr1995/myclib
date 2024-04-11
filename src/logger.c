#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <syslog.h>
#include <errno.h>
#include <sys/mman.h>

#include "logger.h"
#include "assert.h"
#include "common.h"

static int max_level;

static bool log_console = false;

#ifdef ENABLE_LOG_TO_FILE
const char *log_file_name;
static FILE *log_file;
bool always_flush_log_file;
#endif

static char *level_str[] = {
	"Error",
	"Warn",
	"Info",
	"Debug",
};

void
enable_console_log(void)
{
	log_console = true;
}

#ifdef ENABLE_LOG_TO_FILE
void
set_flush_log_file(void)
{
	always_flush_log_file = true;
}

void
close_log_file(void)
{
	if (log_file) {
		fclose(log_file);
		log_file = NULL;
	}
}

void
open_log_file(const char *name)
{
	const char *file_name;

	if (log_file) {
		fclose(log_file);
		log_file = NULL;
	}

	if (!name)
		return;

	file_name = name;

	log_file = fopen(file_name, "a+");
	if (log_file) {
		int n = fileno(log_file);
		if (fcntl(n, F_SETFD, FD_CLOEXEC | fcntl(n, F_GETFD)) == -1)
			log_message(LOG_INFO, "Failed to set CLOEXEC on log file %s", file_name);
		if (fcntl(n, F_SETFL, O_NONBLOCK | fcntl(n, F_GETFL)) == -1)
			log_message(LOG_INFO, "Failed to set NONBLOCK on log file %s", file_name);
	}

}

void
flush_log_file(void)
{
	if (log_file)
		fflush(log_file);
}

void
update_log_file_perms(mode_t umask_bits)
{
	if (log_file)
		fchmod(fileno(log_file), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) & ~umask_bits);
}

#endif

void
vlog_message(const int level, const char* format, va_list *args)
{
	char buf[MAX_LOG_MSG+1];
#ifdef ENABLE_LOG_TO_FILE
	static int write_cnt;
	int cnt;
#endif

	if (level > max_level) {
		return;	
	}

	assert(level >= ERR_LOG && level <= DEBUG_LOG);

	vsnprintf(buf, sizeof(buf), format, *args);

	if (
#ifdef ENABLE_LOG_TO_FILE
	    log_file ||
#endif
	    (log_console)) {

		/* timestamp setup */
#ifdef ENABLE_LOG_TO_FILE
		struct timespec ts;
		time_t t;
		char *p;

		clock_gettime(CLOCK_REALTIME, &ts);
		t = ts.tv_sec;
#else
		time_t t = time(NULL);
#endif
		struct tm tm;
		char timestamp[64];

		localtime_r(&t, &tm);

		if (log_console) {
			strftime(timestamp, sizeof(timestamp), "%c", &tm);
			fprintf(stdout, "[%s] %s: %s\n", level_str[level], timestamp, buf);
		}
#ifdef ENABLE_LOG_TO_FILE
		if (log_file) {

			if (writeback_file(log_file, 81920, 1024) < 0) {
				fprintf(stderr, "%s : writeback_file err : %s",
						__func__, strerror(errno));
			}
			
			p = timestamp;
			p += strftime(timestamp, sizeof(timestamp), "%a %b %d %T", &tm);
			p += snprintf(p, timestamp + sizeof(timestamp) - p, ".%9.9ld", ts.tv_nsec);
			strftime(p, timestamp + sizeof(timestamp) - p, " %Y", &tm);
			write_cnt += fprintf(log_file, "[%s] %s: %s\n", level_str[level], timestamp, buf);
			if (always_flush_log_file)
				fflush(log_file);
		}
#endif
	}

}

void
log_message(const int facility, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vlog_message(facility, format, &args);
	va_end(args);
}

void
conf_write(FILE *fp, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	if (fp) {
		vfprintf(fp, format, args);
		fprintf(fp, "\n");
	}
	else
		vlog_message(LOG_INFO, format, &args);

	va_end(args);
}

void 
set_max_log_level(int level)
{
	max_level = level;
}
