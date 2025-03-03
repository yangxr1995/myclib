#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdatomic.h>

struct hook_ctx_s {
    atomic_bool is_init;
    pthread_spinlock_t lock;
    void *libc;
    void *(*malloc_orig)(size_t size);
    void (*free_orig)(void *ptr);
    void *(*calloc_orig)(size_t nmemb, size_t size);
    void *(*realloc_orig)(void *ptr, size_t size);
    char prgname[128];
};
typedef struct hook_ctx_s hook_ctx_t;

static hook_ctx_t ctx ={
    .is_init = ATOMIC_VAR_INIT(false),
};

inline static int hook_init()
{
    atomic_bool expected = ATOMIC_VAR_INIT(false);

    printf("hook 1\n");
    if (!atomic_compare_exchange_strong(&ctx.is_init, &expected, true))
        return 0;
    printf("hook 2\n");

    pthread_spin_init(&ctx.lock, PTHREAD_PROCESS_PRIVATE);
    ctx.libc = dlopen("libc.so.6", RTLD_LAZY);
    ctx.malloc_orig = dlsym(ctx.libc, "malloc");
    ctx.free_orig = dlsym(ctx.libc, "free");
    ctx.calloc_orig = dlsym(ctx.libc, "calloc");
    ctx.realloc_orig = dlsym(ctx.libc, "realloc");
    
    return 0;
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

void *malloc(size_t size)
{
    printf("malloc 1\n");
    void *ptr;
    hook_init();
    ptr = ctx.malloc_orig(size);
    printf("malloc (%d) : %p\n", size, ptr);
    record_addr("alloc", __builtin_return_address(0), ptr);
    return ptr;
}

void free(void *ptr)
{
    hook_init();
    ctx.free_orig(ptr);
    record_addr("free", __builtin_return_address(0), ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr;
    hook_init();
    ptr = ctx.calloc_orig(nmemb, size);
    record_addr("alloc", __builtin_return_address(0), ptr);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    hook_init();
    record_addr("free", __builtin_return_address(0), ptr);
    ptr = ctx.realloc_orig(ptr, size);
    record_addr("alloc", __builtin_return_address(0), ptr);
    return ptr;
}

typedef int (*strcmp_t)(const char *s1, const char *s2);
strcmp_t old_strcmp;

static void *libc_handle = NULL;	
int strcmp(const char *s1, const char *s2)
{
	if (libc_handle == NULL)
		libc_handle = dlopen("libc.so.6", RTLD_LAZY); 

	if (old_strcmp == NULL)
		old_strcmp = (strcmp_t)dlsym(libc_handle, "strcmp");

	printf("strcmp %s %s\n", s1, s2);

	return old_strcmp(s1, s2);
}
