#include <pthread.h>
#include <stdlib.h>
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

static int max_level;

static bool log_console = false;
const char *log_file_name;
static FILE *log_file;
bool always_flush_log_file;
int max_line = 81920;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

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

void
set_flush_log_file(void)
{
	always_flush_log_file = true;
}

void
set_max_log_line(unsigned int line)
{
    max_line = line;
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

static int
writeback_file(FILE *fp, int max_cnt, int del_cnt)
{
	int fd;
	char *data = NULL, *end, *head, *p;
	struct stat st;
	int cnt = 0;

	fflush(fp);

	if ((fd = fileno(fp)) < 0) {
		log_message(ERR_LOG, "%s : fileno : %s", __func__, strerror(errno));
		goto err;
	}

	fstat(fd, &st);
	cnt = st.st_size;
	if (st.st_size <= max_cnt)
		goto end;

	if ((data = mmap(NULL, st.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		log_message(ERR_LOG, "%s : mmap : %s", __func__, strerror(errno));
		goto err;
	}

	head = NULL;
	for (end = data + st.st_size, p = data + del_cnt ; p < end; p++) {
		if (*p == '\n')
			break;
	}
	head = p + 1;

	cnt = end - head;

	memmove(data, head, cnt);

    if (ftruncate(fd, cnt)) 
        return -1;
	

	fseek(fp, 0, SEEK_SET);

end:
	if (data != MAP_FAILED && data != NULL)
		munmap(data, st.st_size);

	return cnt;
   	
err:
	if (data != MAP_FAILED && data != NULL)
		munmap(data, st.st_size);

	return -1;
}

void
vlog_message(const int level, const char* format, va_list *args)
{
	char buf[MAX_LOG_MSG+1];
	static int write_cnt;
	int cnt;

	if (level > max_level) {
		return;	
	}

	assert(level >= ERR_LOG && level <= DEBUG_LOG);

	vsnprintf(buf, sizeof(buf), format, *args);

    /*if (pthread_mutex_trylock(&mtx) < 0)*/
    /*    return;*/
    /**/
	/*pthread_mutex_lock(&mtx);*/

	if (
	    log_file ||
	    (log_console)) {

		/* timestamp setup */
		struct timespec ts;
		time_t t;
		char *p;

		clock_gettime(CLOCK_REALTIME, &ts);
		t = ts.tv_sec;
		struct tm tm;
		char timestamp[64];

		localtime_r(&t, &tm);

		if (log_console) {
			strftime(timestamp, sizeof(timestamp), "%c", &tm);
			fprintf(stdout, "[%s] %s: %s\n", level_str[level], timestamp, buf);
		}
		if (log_file) {

			if (writeback_file(log_file, max_line, 1024) < 0) {
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
	}

	/*pthread_mutex_unlock(&mtx);*/
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
