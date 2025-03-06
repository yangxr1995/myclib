#if 0
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdbool.h>

struct hook_ctx_s {
    void *libc;

    void *(*malloc_orig)(size_t size);
    void (*free_orig)(void *ptr);
    void *(*calloc_orig)(size_t nmemb, size_t size);
    void *(*realloc_orig)(void *ptr, size_t size);
    FILE *(*fopen_orig)(const char *__restrict __filename,
		    const char *__restrict __modes);
    int (*fclose_orig)(FILE *stream);
    int (*fprintf_orig)(FILE *stream, const char *format, ...);

    char prgname[128];
};
typedef struct hook_ctx_s hook_ctx_t;

static hook_ctx_t ctx ={
    .is_init = ATOMIC_VAR_INIT(false),
};

static bool g_is_init;

// 优先于任何构造函数执行的初始化
__attribute__((constructor)) 
static void preinit_original_symbols() {

    write(STDOUT_FILENO, "init\n", 5);

    // 无锁状态下直接获取原始函数指针
    ctx.malloc_orig = dlsym(RTLD_NEXT, "malloc");
    ctx.calloc_orig = dlsym(RTLD_NEXT, "calloc");
    ctx.realloc_orig = dlsym(RTLD_NEXT, "realloc");
    ctx.free_orig = dlsym(RTLD_NEXT, "free");
    ctx.fopen_orig = dlsym(RTLD_NEXT, "fopen");
    ctx.fclose_orig = dlsym(RTLD_NEXT, "fclose");
    ctx.fprintf_orig = dlsym(RTLD_NEXT, "fprintf");

    write(STDOUT_FILENO, "init ok\n", 8);
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

static
void record_addr(const char *prefix, void *call_addr, void *ptr)
{
    static char prg[64];
    char filename[256] = "./malloc.log";

    if (prg[0] == '\0')
        get_prg_name(prg, sizeof(prg), getpid());

    snprintf(filename, sizeof(filename), "/var/log/%s-malloc-%u-%ld", 
            prg, getpid(), pthread_self());

    FILE *fp;
    fp = fopen(filename, "a");
    fprintf(fp, "%s:%p:%p\n", prefix, call_addr, ptr);
    fclose(fp);
}

void *malloc(size_t size)
{
    write(STDOUT_FILENO, "malloc 0\n", 11);
    void *ptr;
    ptr = ctx.malloc_orig(size);
    record_addr("alloc", __builtin_return_address(0), ptr);
    return ptr;
}

void free(void *ptr)
{
    ctx.free_orig(ptr);
    record_addr("free", __builtin_return_address(0), ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    /*write(STDOUT_FILENO, "bbb\n", 4);*/
    void *ptr;
    ptr = ctx.calloc_orig(nmemb, size);
    /*record_addr("alloc", __builtin_return_address(0), ptr);*/
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    record_addr("free", __builtin_return_address(0), ptr);
    ptr = ctx.realloc_orig(ptr, size);
    record_addr("alloc", __builtin_return_address(0), ptr);
    return ptr;
}

#endif
