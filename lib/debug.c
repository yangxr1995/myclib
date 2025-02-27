#ifdef __cplusplus
extern "C" {

#define this _this

#endif

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <execinfo.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>

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
	fprintf(_dlog_fp, "[%s]" fmt, _prg, ## __VA_ARGS__); \
	fprintf(stdout, "[%s]" fmt, _prg, ## __VA_ARGS__); \
} while (0)

static int stack_enable = 1;

typedef struct call_item_s call_item_t;
struct call_item_s {
	void *this;
	void *call;
};

typedef struct call_stack_s call_stack_t;
struct call_stack_s {
	call_item_t *arr;	
	int size;
	int top;
};
static call_stack_t cstack;

static __thread FILE *_dlog_fp = NULL;
static char _prg[64];
static __thread pthread_t _tid;
static __thread pid_t _prg_pid;
static char trace_on[256];

typedef struct map_s {
	char *name;
	unsigned long begin;
	unsigned long end;
} map_t;

#define MAX_MAP_NUM  32

static __thread map_t *text_maps;
static __thread unsigned int text_map_max_num;
static __thread unsigned int text_map_num;
static __thread int debug_enable = 1;

void __attribute__((__no_instrument_function__))
debug_on()
{
	debug_enable = 1;
}

void __attribute__((__no_instrument_function__))
debug_off()
{
	debug_enable = 0;
}

static void  __attribute__((__no_instrument_function__))
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

static inline char  __attribute__((__no_instrument_function__))
is_text(const char *str)
{
	return strstr(str, "r-xp") != NULL ? 1 : 0;
}

static void __attribute__((__no_instrument_function__))
get_task_maps()
{
	map_t *ptext_map;
	char *ptr;
	char maps[256];	
	snprintf(maps, sizeof(maps), "/proc/%d/maps", _prg_pid);

	FILE *fp;

	fp = fopen(maps, "r");
	assert(fp != NULL && "can't open proc maps");
	char *line = NULL;
	size_t n = 0;

	while (getline(&line, &n, fp) > 0) {

		if (!is_text(line))
			goto __next__;
		
		if (text_map_max_num <= text_map_num) {
			text_map_max_num += 32;
			text_maps = (map_t *)realloc(text_maps, sizeof(*text_maps) * text_map_max_num);
		}

		for (ptr = line + strlen(line); *ptr != '/' && *ptr != ' '; ptr--)
			NULL;
		if (*ptr == ' ')
			goto __next__;

		ptext_map = text_maps + text_map_num;
		text_map_num++;
		ptext_map->name = strdup(ptr + 1);	
		ptext_map->name[strlen(ptext_map->name) - 1] = '\0';

		ptext_map->begin = strtoul(line, &ptr, 16);
		ptext_map->end = strtoul(ptr + 1, NULL, 16);

__next__:
		if (line) {
			free(line);
			line = NULL;
		}
	}

	fclose(fp);
#if 0
	int i;
	for (i = 0; i < text_map_num; i++)
		printf("%p-%p %s\n", (void *)text_maps[i].begin, (void *)text_maps[i].end, text_maps[i].name);
#endif

}

inline static int __attribute__((__no_instrument_function__))
is_fd_closed(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1 && errno == EBADF)
        return 1;
    return 0;
}

inline static int __attribute__((__no_instrument_function__))
is_fp_closed(FILE *fp)
{
	int fd;
	if ((fd = fileno(fp)) < 0)
		return -1;	
	return is_fd_closed(fd);
}

inline static void __attribute__((__no_instrument_function__))
fp_add_flags(FILE *fp, int add_flags)
{
	int fd, flags;

	fd = fileno(fp);	
	flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | add_flags);
}

inline static void __attribute__((__no_instrument_function__))
text_maps_free(map_t *maps, int num)
{
	int i;
	for (i = 0; i < num; i++)
		free(maps[i].name);
	free(maps);
}

inline static void __attribute__((__no_instrument_function__))
__env_init()
{
    _tid = pthread_self();
    _prg_pid = getpid();

    if (_prg[0] == '\0')
        get_prg_name(_prg, sizeof(_prg), _prg_pid);

	if (trace_on[0] == '\0')
		snprintf(trace_on, sizeof(trace_on) - 1, "/var/run/trace_%s", _prg);

	if (text_map_num == 0)
		get_task_maps();
}

static int __attribute__((__no_instrument_function__))
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
	__env_init();
	PRINT_LOG("############## Program Info ##############\n");
	_show_prg_info(pid);
	PRINT_LOG("\n");
}

void __attribute__((__no_instrument_function__))
print_stacktrace()
{
    int size = 16, i;
    void * array[16];
    int stack_num = backtrace(array, size);
    char ** stacktrace = backtrace_symbols(array, stack_num);

	__env_init();
	PRINT_LOG("########## stack info   #############\n");
    for (i = 1; i < stack_num; ++i)
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

	__env_init();
	snprintf(proc_item, sizeof(proc_item) - 1, "/proc/self/fd/%d", fd);
	readlink(proc_item, buf, sz);
}

void * __attribute__((__no_instrument_function__))
print_nobase_addr(void *this)
{
    int i;
	char *this_sym = NULL;
	int this_set = 0;;

	__env_init();

	for (i = 0; i < text_map_num; i++) {

		if (this_set == 0 && 
				text_maps[i].begin < (unsigned long)this &&	
				text_maps[i].end > (unsigned long)this) {

			this_sym = text_maps[i].name;
			if (strcmp(this_sym, _prg) != 0) // 进程的.text段不需要偏移
				this -= text_maps[i].begin;
			this_set = 1;
		}
	}

    if (this_set == 0) {
        printf("WARNING: can't find %p", this);
        return NULL;
    }

    return this;
}

static inline void __attribute__((__no_instrument_function__))
print_running_info(const char *msg, void *this, void *call)
{
	int i;
	char *this_sym = NULL, *call_sym = NULL;
	int call_set = 0, this_set = 0;;

	for (i = 0; i < text_map_num; i++) {

		if (this_set == 0 && 
				text_maps[i].begin < (unsigned long)this &&	
				text_maps[i].end > (unsigned long)this) {

			this_sym = text_maps[i].name;
            // LDFLAGS += -no-pie
			// if (strcmp(this_sym, _prg) != 0) { // 进程的.text段不需要偏移
			// 	this -= text_maps[i].begin;
            // } 

            // LDFLAGS 不加 -no-pie
			if (strcmp(this_sym, _prg) == 0) {
                if ((unsigned long)(this - text_maps[i].begin) > 0xfffff)
                    this -= text_maps[i].begin;
            } 


			this_set = 1;
		}

		if (call_set == 0 && 
				text_maps[i].begin < (unsigned long)call &&	
				text_maps[i].end > (unsigned long)call) {

			call_sym = text_maps[i].name;

            // LDFLAGS += -no-pie
			// if (strcmp(call_sym, _prg) != 0) {
			// 	call -= text_maps[i].begin;
            //     printf("----------call[%p]--------\n", call);
            // }

			if (strcmp(call_sym, _prg) == 0) {
                if ((unsigned long)(call - text_maps[i].begin) > 0xfffff)
                    call -= text_maps[i].begin;
            }

			call_set = 1;
		}

	}

	if (call_set == 0)
		printf("WARNING call %p\n", call);

	if (this_set == 0)
		printf("WARNING this %p\n", this);

    FILE *fp;
    char filename[256];
    snprintf(filename, sizeof(filename), "/var/run/%s-%d-%ld.log", _prg, _prg_pid, _tid);
    fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("failed to open %s\n", filename);
        exit(1);
    }
	fprintf(fp, "%s\n%s:%p\n%s:%p\n", msg, call_sym, call, this_sym, this);
    fclose(fp);
}

static inline void __attribute__((__no_instrument_function__))
print_running_info_stack(const char *msg, void *this, void *call)
{
	if (stack_enable)
		print_running_info(msg, this, call);
}

static inline void __attribute__((__no_instrument_function__))
print_running_info_trace(const char *msg, void *this, void *call)
{
	struct stat st;

	__env_init();

	if (stat(trace_on, &st) == 0)
		print_running_info(msg, this, call);
}

static void __attribute__((__no_instrument_function__))
record_push(void *this, void *call)
{
	assert(this != NULL );
	assert(call != NULL );

	if (cstack.size <= cstack.top) {
		cstack.size += 1024;
		cstack.arr = (call_item_t *)realloc(cstack.arr, cstack.size * sizeof(call_item_t));
	}

	cstack.arr[cstack.top].call = call;
	cstack.arr[cstack.top].this = this;
	cstack.top++;
}

static void __attribute__((__no_instrument_function__))
record_pop()
{
	if (cstack.top <= 0)
		printf("cstack.top = %d\n", cstack.top);
	assert(cstack.top > 0);
	cstack.top--;
}

/*int mem_leak();*/

void __attribute__((__no_instrument_function__))
__cyg_profile_func_enter(void *this, void *call)
{
#if 0
	if (mem_leak())
		print_running_info_trace("Enter", this, call);
#endif

	if (debug_enable)
		print_running_info_trace("Enter", this, call);

	if (stack_enable)
		record_push(this, call);
}

void __attribute__((__no_instrument_function__))
__cyg_profile_func_exit(void *this, void *call)
{
#if 0
	if (mem_leak())
		print_running_info_trace("Exit", this, call);
#endif
	if (debug_enable)
		print_running_info_trace("Exit", this, call);

	if (stack_enable)
		record_pop();
}

void __attribute__((__no_instrument_function__))
print_stack()
{
	if (stack_enable == 0)
		return ;
	fprintf(_dlog_fp, "------------ STACK ----------\n");
	for (int i = 0; i < cstack.top; i++) {
		print_running_info_stack("Enter", cstack.arr[i].this, cstack.arr[i].call);
	}
}

#ifdef __cplusplus
}
#endif
