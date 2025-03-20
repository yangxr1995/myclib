#ifdef __cplusplus
extern "C" {
#endif

#ifdef WRAP_DEFINE

#include <sys/epoll.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "wrap/utils.h"

wrap_define(char *, fgets, char *s, int size, FILE *stream)
{
    char *ret = __real_fgets(s, size, stream);
    log_wrap_lib_info("fgets(stream[%p]):%s", stream, ret);
    return ret;
}

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
    char str[32] = {0};
    size_t ret = __real_fread(ptr, size, nmemb, stream);
    if (size == 1) {
        strncpy(str, (char *)ptr, nmemb > sizeof(str) - 1 ? sizeof(str) - 1 : nmemb);
        log_wrap_lib_info("ret[%d] = fread(ptr[%s], nmemb[%d], stream[%p])", (int)ret, str, (int)nmemb, stream);
    }
    else {
        log_wrap_lib_info("ret[%d] = fread(ptr[%p], size[%d], nmemb[%d], stream[%p])", (int)ret, ptr, (int)size, (int)nmemb, stream);
    }
    return ret;
}

wrap_define(size_t, fwrite, const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    char str[32] = {0};
    size_t ret = __real_fwrite(ptr, size, nmemb, stream);
    if (size == 1) {
        strncpy(str, (char *)ptr, nmemb > sizeof(str) - 1 ? sizeof(str) - 1 : nmemb);
        log_wrap_lib_info("ret[%d] = fwrite(ptr[%s], nmemb[%d], stream[%p])", (int)ret, str, (int)nmemb, stream);
    }
    else {
        log_wrap_lib_info("ret[%d] = fwrite(ptr[%p], size[%d], nmemb[%d], stream[%p])", (int)ret, ptr, (int)size, (int)nmemb, stream);
    }
    return ret;
}

wrap_define(int, unlink, const char *pathname)
{
    log_wrap_lib_info("unlink(%s)", pathname);
    return __real_unlink(pathname);
}

wrap_define(int, access, const char *pathname, int mode)
{
    log_wrap_lib_info("access(%s, %d)", pathname, mode);
    return __real_access(pathname, mode);
}

wrap_define(int, select, int nfds, fd_set *readfds, fd_set *writefds, 
          fd_set *exceptfds, struct timeval *timeout)
{
    log_wrap_lib_info("select(nfds[%d], readfds[%p], writefds[%p], exceptfds[%p], timeout[%p])", 
            nfds, readfds, writefds, exceptfds, timeout);
    return __real_select(nfds, readfds, writefds, exceptfds, timeout);
}

wrap_define(int, epoll_wait, int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    log_wrap_lib_info("epoll_wait(epfd[%d], events[%p], maxevents[%d], timeout[%d])", epfd, events, maxevents, timeout);
    return __real_epoll_wait(epfd, events, maxevents, timeout);
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
#define fgets(s, size, stream) __real_fgets(s, size, stream)
#define unlink(pathname)       __real_unlink(pathname)
#define access(pathname, mode) __real_access(pathname, mode)
#define select(nfds, readfds, writefds, exceptfds, timeout) __real_select(nfds, readfds, writefds, exceptfds, timeout)
#define epoll_wait(epfd, events, maxevents, timeout)        __real_epoll_wait(epfd, events, maxevents, timeout)
#define fclose(stream)         __real_fclose(stream)

#endif

#ifdef __cplusplus
}
#endif
