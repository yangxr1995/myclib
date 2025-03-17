#ifdef __cplusplus
extern "C" {

#define this _this

#endif

#include <sys/types.h>
#include <unistd.h>
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

#include "trace.h"

__attribute__((__no_instrument_function__))
static void log_append(const char *str);

__attribute__((__no_instrument_function__))
inline static int
confirm_addr_info(void *orig_addr, void **paddr, char **psym);

#ifdef TRACE_CLIB_STR

int __real_strcmp(const char *s1, const char *s2);
int __real_strncmp(const char *s1, const char *s2, size_t n);

__attribute__((__no_instrument_function__))
int __wrap_strcmp(const char *s1, const char *s2)
{
    char buf[256] = {0};
    char *call_sym;
    void *call;
    confirm_addr_info(__builtin_return_address(0) - sizeof(void *), &call, &call_sym);
    snprintf(buf, sizeof(buf) - 1, "%s:%p:strcmp(%s, %s)\n", call_sym, call, s1, s2);
    log_append(buf);
    return __real_strcmp(s1, s2);
}

__attribute__((__no_instrument_function__))
int __wrap_strncmp(const char *s1, const char *s2, size_t n)
{
    char buf[256] = {0};
    char *call_sym;
    void *call;
    confirm_addr_info(__builtin_return_address(0) - sizeof(void *), &call, &call_sym);
    snprintf(buf, sizeof(buf) - 1, "%s:%p:strncmp(%s, %s, %lu)\n", call_sym, call, s1, s2, n);
    log_append(buf);
    return __real_strncmp(s1, s2, n);
}

#define strcmp(s1, s2) __real_strcmp(s1, s2)
#define strncmp(s1, s2, n) __real_strncmp(s1, s2, n)

#endif

typedef struct map_s {
	char *name;
	unsigned long begin;
	unsigned long end;
} map_t;

typedef struct trace_ctx_s trace_ctx_t;
struct trace_ctx_s {
    pid_t pid;
    char name[128];
    char trace_on[256];

    map_t *text_maps;
    unsigned int text_map_max_num;
    unsigned int text_map_num;
};

static trace_ctx_t ctx;

pid_t __real_fork();
__attribute__((__no_instrument_function__))
pid_t __wrap_fork()
{
    int ret;
    char *call_sym;
    void *call;
    char buf[256] = {0};
    confirm_addr_info(__builtin_return_address(0) - sizeof(void *), &call, &call_sym);
    snprintf(buf, sizeof(buf) - 1, "%s:%p:fork()\n", call_sym, call);
    log_append(buf);
    if ((ret = __real_fork()) == 0)
        ctx.pid = getpid();
    return ret;
}

__attribute__((__no_instrument_function__))
static void
log_append(const char *str)
{
    int fd;
    char filename[256];
    pthread_t tid = pthread_self();
    snprintf(filename, sizeof(filename), "/var/run/%s-%d-%ld.log", ctx.name, ctx.pid, tid);
    if ((fd = open(filename, O_APPEND | O_CREAT | O_WRONLY, 0644)) == 0) {
        perror("log_append : open");
        exit(1);
    }
    write(fd, str, strlen(str));
    close(fd);
}

__attribute__((__no_instrument_function__))
inline static int
confirm_addr_info(void *orig_addr, void **paddr, char **psym)
{
    unsigned int i;
    void *addr;
    char *sym;
    char is_find = 0;

	for (i = 0; i < ctx.text_map_num; i++) {

		if (ctx.text_maps[i].begin < (unsigned long)orig_addr &&	
				ctx.text_maps[i].end > (unsigned long)orig_addr) {

            is_find = 1;
			sym = ctx.text_maps[i].name;

            addr = orig_addr;
            // 动态库需要偏移
			if (strcmp(sym, ctx.name) != 0)
				addr -= ctx.text_maps[i].begin;
#ifdef USE_PIE
            // 使用pie时，进程.text需要偏移
            else
                addr -= text_maps[i].begin;
#endif
            break;
		}

    }

    if (!is_find) {
        printf("WARNING:can't find %p\n", orig_addr);
        return 1;
    }

    *paddr = addr;
    *psym = sym;
    return 0;
}

static inline void __attribute__((__no_instrument_function__))
__trace_running(const char *msg, void *this, void *call)
{
	int i;
	char *this_sym = NULL, *call_sym = NULL;
	int call_set = 0, this_set = 0;;

    confirm_addr_info(this, &this, &this_sym);
    confirm_addr_info(call, &call, &call_sym);

// 	for (i = 0; i < ctx.text_map_num; i++) {
//
// 		if (this_set == 0 && 
// 				ctx.text_maps[i].begin < (unsigned long)this &&	
// 				ctx.text_maps[i].end > (unsigned long)this) {
//
// 			this_sym = ctx.text_maps[i].name;
//
//             // 动态库需要偏移
// 			if (strcmp(this_sym, ctx.name) != 0)
// 				this -= ctx.text_maps[i].begin;
// #ifdef USE_PIE
//             // 使用pie时，进程.text需要偏移
//             else
//                 this -= text_maps[i].begin;
// #endif
// 			this_set = 1;
// 		}
//
// 		if (call_set == 0 && 
// 				ctx.text_maps[i].begin < (unsigned long)call &&	
// 				ctx.text_maps[i].end > (unsigned long)call) {
//
// 			call_sym = ctx.text_maps[i].name;
//
// 			if (strcmp(call_sym, ctx.name) != 0)
// 				call -= ctx.text_maps[i].begin;
// #ifdef USE_PIE
//             else
//                 call -= text_maps[i].begin;
// #endif
// 			call_set = 1;
// 		}
//
// 	}

	// if (call_set == 0)
	// 	printf("WARNING call %p\n", call);
	//
	// if (this_set == 0)
	// 	printf("WARNING this %p\n", this);
	//

    char data[256];
    snprintf(data, sizeof(data), "%s\n%s:%p\n%s:%p\n", msg, call_sym, call, this_sym, this);
    log_append(data);
}

static inline void __attribute__((__no_instrument_function__))
trace_running(const char *msg, void *this, void *call)
{
	struct stat st;

	if (stat(ctx.trace_on, &st) == 0)
		__trace_running(msg, this, call);
}

void __attribute__((__no_instrument_function__))
__cyg_profile_func_enter(void *this, void *call)
{
    trace_running("Enter", this, call);
}

void __attribute__((__no_instrument_function__))
__cyg_profile_func_exit(void *this, void *call)
{
    trace_running("Exit", this, call);
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
	snprintf(maps, sizeof(maps), "/proc/%d/maps", ctx.pid);

	FILE *fp;

	fp = fopen(maps, "r");
	assert(fp != NULL && "can't open proc maps");
	char *line = NULL;
	size_t n = 0;

	while (getline(&line, &n, fp) > 0) {

		if (!is_text(line))
			goto __next__;
		
		if (ctx.text_map_max_num <= ctx.text_map_num) {
			ctx.text_map_max_num += 32;
			ctx.text_maps = (map_t *)realloc(ctx.text_maps, sizeof(*ctx.text_maps) * ctx.text_map_max_num);
		}

		for (ptr = line + strlen(line); *ptr != '/' && *ptr != ' '; ptr--)
			NULL;
		if (*ptr == ' ')
			goto __next__;

		ptext_map = ctx.text_maps + ctx.text_map_num;
		ctx.text_map_num++;
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

__attribute__((__no_instrument_function__))
inline static void 
prg_info_init()
{
    ctx.pid = getpid();
    get_prg_name(ctx.name, sizeof(ctx.name), ctx.pid);
    snprintf(ctx.trace_on, sizeof(ctx.trace_on) - 1, "/var/run/trace_%s", ctx.name);
    get_task_maps();
}

__attribute__((constructor)) 
__attribute__((__no_instrument_function__))
inline static void 
init()
{
    prg_info_init();
}

#ifdef __cplusplus
}
#endif
