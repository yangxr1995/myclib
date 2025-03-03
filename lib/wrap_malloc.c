#ifdef DEBUG_WRAP_MALLOC

#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

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
    char filename[256];

    if (prg[0] == '\0')
        get_prg_name(prg, sizeof(prg), getpid());

    snprintf(filename, sizeof(filename), "/var/log/%s-malloc-%u-%ld", 
            prg, getpid(), pthread_self());

    FILE *fp;
    fp = fopen(filename, "a");
    fprintf(fp, "%s:%p:%p\n", prefix, call_addr, ptr);
    fclose(fp);
}

void *__real_realloc(void *ptr, size_t size);
void *__wrap_realloc(void *ptr, size_t size)
{
    void *ret_addr;
    ret_addr = __builtin_return_address(0);
    record_addr("free", ret_addr, ptr);
    ptr = __real_realloc(ptr, size);
    record_addr("alloc", ret_addr, ptr);

    return ptr;
}

void *__real_calloc(size_t nmemb, size_t size);
void *__wrap_calloc(size_t nmemb, size_t size)
{
    void *ptr, *ret_addr;

    ret_addr = __builtin_return_address(0);
    ptr = __real_calloc(nmemb, size);
    record_addr("alloc", ret_addr, ptr);

    return ptr;
}

void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    void *ptr;
    ptr = __real_malloc(size);

    // 需要加上 -no-pie 方便 addr2line获得调用信息
    void *ret_addr = __builtin_return_address(0);
    record_addr("alloc", ret_addr, ptr);

    return ptr;
}

void __real_free(void *ptr);
void __wrap_free(void *ptr)
{
    __real_free(ptr);

    void *ret_addr = __builtin_return_address(0);
    record_addr("free", ret_addr, ptr);
}

#endif
