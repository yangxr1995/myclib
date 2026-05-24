#include <signal.h>
#ifdef DEBUG_WRAP_MALLOC

#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>

#include "table.h"

void *__real_malloc(size_t size);

#define LIKELY(x) __builtin_expect(!!(x), 1)

struct wm_ctx_s {
    bool init;
    table_t tb;
    char prgname[64];
};
typedef struct wm_ctx_s wm_ctx_t;
wm_ctx_t ctx;

struct alloc_info {
    void *addr;
    void *caller;
};
typedef struct alloc_info alloc_info_t;

static int ptr_cmp(const void *a, const void *b)
{
    if (a == b)
        return 0;
    return a < b ? -1 : 0;
}

static unsigned int ptr_hash(const void *key)
{
    return *(unsigned int *)&key;
}

static void dump_alloc_info(const char *key, void **val, void *cl)
{
    char filename[256] = {0};
    alloc_info_t *info = *(alloc_info_t **)val;

    snprintf(filename, sizeof(filename), "/var/log/%s-malloc-%u", 
            ctx.prgname, getpid());

    FILE *fp;
    fp = fopen(filename, "a");
    fprintf(fp, "%p\n", info->caller);
    fclose(fp);
}

static void dump_alloc_infos(int sig)
{
    table_map(ctx.tb, dump_alloc_info, NULL);
}

static void
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

__attribute__((constructor)) 
static void wrap_malloc_init()
{
    ctx.tb = table_new(1024, ptr_cmp, ptr_hash);
    signal(SIGUSR1, dump_alloc_infos);
    get_prg_name(ctx.prgname, sizeof(ctx.prgname), getpid());
    ctx.init = true;
}



void *__real_realloc(void *ptr, size_t size);
void *__wrap_realloc(void *ptr, size_t size)
{
    void *ret_addr;

    ret_addr = __builtin_return_address(0);
    ptr = __real_realloc(ptr, size);

    return ptr;
}

void *__real_calloc(size_t nmemb, size_t size);
void *__wrap_calloc(size_t nmemb, size_t size)
{
    void *ptr, *ret_addr;

    ret_addr = __builtin_return_address(0);
    ptr = __real_calloc(nmemb, size);

    return ptr;
}

static void alloc_info_add(void *addr, void *caller)
{
    alloc_info_t *info;

    info = __real_malloc(sizeof(*info));
    info->addr = addr;
    info->caller = caller;
    table_put(ctx.tb, addr, info);
}

void *__wrap_malloc(size_t size)
{
    void *ptr;

    ptr = __real_malloc(size);
    void *ret_addr = __builtin_return_address(0);

    if (LIKELY(ctx.init)) {
        alloc_info_add(ptr, ret_addr);
    }

    return ptr;
}

static void alloc_info_del(void *addr, void *caller)
{
    alloc_info_t *info;
    if ((info = table_remove(ctx.tb, addr)) == NULL) {
        printf("free null ptr\n");
        printf("caller:%p\n", caller);
        exit(1);
    }
}

void __real_free(void *ptr);
void __wrap_free(void *ptr)
{
    if (LIKELY(ctx.init)) {
        void *caller = __builtin_return_address(0);
        alloc_info_del(ptr, caller);
    }

    __real_free(ptr);
}

#endif
