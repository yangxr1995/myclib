#ifdef WRAP_DEFINE

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

int __real_open(const char *path, int oflag, ...);
__attribute__((__no_instrument_function__))
int __wrap_open(const char *path, int oflag, ...)
{
    va_list args;
    mode_t mode = 0;
    int fd;

    va_start(args, oflag);
    if (oflag & O_CREAT)
        mode = va_arg(args, mode_t);
    va_end(args);

    if (oflag & O_CREAT) 
        fd = __real_open(path, oflag, mode) ;
    else
        fd = __real_open(path, oflag);

    log_wrap_lib_info("%d = open(%s)", fd, path);
    return fd;
}

wrap_define(int, close, int fd)
{
    log_wrap_lib_info("close(%d)", fd);
    return __real_close(fd);
}

wrap_define(FILE *, fopen, const char *pathname, const char *mode)
{
    FILE *ret;
    ret = __real_fopen(pathname, mode);
    log_wrap_lib_info("%p = fopen(%s, %s)", ret, pathname, mode);
    return ret;
}

wrap_define(int, fclose, FILE *stream)
{
    log_wrap_lib_info("fclose(%p)", stream);
    return __real_fclose(stream);
}

wrap_define(FILE *, popen, const char *command, const char *type)
{
    FILE *fp = __real_popen(command, type);
    log_wrap_lib_info("%p = popen(%s, %s)", fp, command, type);
    return fp;
}

wrap_define(int, pclose, FILE *stream)
{
    log_wrap_lib_info("pclose(%p)", stream);
    return __real_pclose(stream);
}

wrap_define(ssize_t, read, int fd, void *buf, size_t count)
{
    ssize_t ret = __real_read(fd, buf, count);
    log_wrap_lib_info("%ld = read(%d)", ret, fd);
    return ret;
}

wrap_define(ssize_t, write, const int fd, const void *buf, const size_t count)
{
    ssize_t ret = __real_write(fd, buf, count);
    log_wrap_lib_info("%ld = write(%d)", ret, fd);
    return ret;
}

wrap_define(size_t, fread, void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t ret = __real_fread(ptr, size, nmemb, stream);
    log_wrap_lib_info("%d = fread(%p, %d, %d, %p)", (int)ret, ptr, (int)size, (int)nmemb, stream);
    return ret;
}

wrap_define(size_t, fwrite, const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t ret = __real_fwrite(ptr, size, nmemb, stream);
    log_wrap_lib_info("%d = fwrite(%p, %d, %d, %p)", (int)ret, ptr, (int)size, (int)nmemb, stream);
    return ret;
}

#elif defined(WRAP_REPLACE)

#define open(path, oflag, ...) __real_open(path, oflag, ## __VA_ARGS__)
#define close(fd)              __real_close(fd)
#define fopen(pathname, mode)  __real_fopen(pathname, mode)
#define popen(command, type)   __real_popen(command, type)
#define pclose(stream)         __real_pclose(stream)
#define read(fd, buf, count)   __real_read(fd, buf, count)
#define write(fd, buf, count)  __real_write(fd, buf, count)
#define fread(ptr, size, nmemb, stream)   __real_fread(ptr, size, nmemb, stream)
#define fwrite(ptr, size, nmemb, stream)  __real_write(ptr, size, nmemb, stream)

#endif
