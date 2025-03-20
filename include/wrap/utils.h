#ifdef __cplusplus
extern "C" {
#endif

#ifndef __WRAP_UTILS_H__
#define __WRAP_UTILS_H__

extern int __real_access(const char *path, int amode);

#define log_wrap_lib_info(format, ...) do { \
    if (__real_access(ctx.trace_on, F_OK) == 0) { \
        char buf[256] = {0}, *call_sym; \
        void *call; \
        confirm_addr_info(__builtin_return_address(0) - sizeof(void *), &call, &call_sym); \
        snprintf(buf, sizeof(buf) - 1, "%s::::%p::::" format "\n", call_sym, call, ## __VA_ARGS__); \
        log_append(buf); \
    } \
} while (0)

#define wrap_define(_ret, _name, ...) \
_ret __real_##_name(__VA_ARGS__); \
__attribute__((__no_instrument_function__)) \
_ret __wrap_##_name(__VA_ARGS__)


#endif

#ifdef __cplusplus
}
#endif
