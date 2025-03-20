#ifdef __cplusplus
extern "C" {
#endif

#ifdef WRAP_DEFINE

#include <string.h>

wrap_define(int, system, const char *command)
{
    log_wrap_lib_info("system(%s)", command);
    return __real_system(command);
}

wrap_define(unsigned int, sleep, unsigned int seconds)
{
    log_wrap_lib_info("sleep(%d)", seconds);
    return __real_sleep(seconds);
}

wrap_define(int, execv, const char *path, char *const argv[])
{
    char str[256] = {0};
    int i, space_len;

    space_len = sizeof(str);
    strncat(str, path, space_len - 1);
    space_len -= strlen(str) + 1;
    for (i = 0; space_len > 2 && argv[i]; ++i) {
        strcat(str, " ");
        strncat(str, argv[i], space_len);
        space_len -= strlen(argv[i]) + 1;
    }
    log_wrap_lib_info("execv(%s)", str);
    return __real_execv(path, argv);
}

wrap_define(pid_t, wait, int *wstatus)
{
    log_wrap_lib_info("wait()");
    return __real_wait(wstatus);
}

wrap_define(pid_t, waitpid, pid_t pid, int *wstatus, int options)
{
    log_wrap_lib_info("waitpid(%d)", pid);
    return __real_waitpid(pid, wstatus, options);
}

#else

#define system(command)     __real_system(command)
#define sleep(seconds)      __real_sleep(seconds)
#define execv(path, argv)   __real_execv(path, argv)
#define wait(wstatus)       __real_wait(wstatus)
#define waitpid(pid, wstatus, options) __real_waitpid(pid, wstatus, options)

#endif

#ifdef __cplusplus
}
#endif
