#ifdef __cplusplus
extern "C" {
#endif

#ifdef WRAP_DEFINE
#include <string.h>
#include <stdio.h>

wrap_define(int, strcmp, const char *s1, const char *s2)
{
    log_wrap_lib_info("strcmp(%s, %s)", s1, s2);
    return __real_strcmp(s1, s2);
}

wrap_define(int, strncmp, const char *s1, const char *s2, size_t n)
{
    log_wrap_lib_info("strncmp(%s, %s, %lu)", s1, s2, n);
    return __real_strncmp(s1, s2, n);
}

wrap_define(char *, strcpy, char *dest, const char *src)
{
    log_wrap_lib_info("strcpy(%p, %s)", dest, src);
    return __real_strcpy(dest, src);
}

wrap_define(char *, strncpy, char *dest, const char *src, size_t n)
{
    log_wrap_lib_info("strncpy(%p, %s, %lu)", dest, src, n);
    return __real_strncpy(dest, src, n);
}


wrap_define(char *, strstr, const char *haystack, const char *needle)
{
    char *ret = __real_strstr(haystack, needle);
    log_wrap_lib_info("strstr(haystack[%s], needle[%s]) == %p", haystack, needle, ret);
    return ret;
}

wrap_define(char *, strcasestr, const char *haystack, const char *needle)
{
    char *ret = __real_strcasestr(haystack, needle);
    log_wrap_lib_info("strcasestr(haystack[%s], needle[%s]) == %p", haystack, needle, ret);
    return ret;
}

#elif defined(WRAP_REPLACE)
#define strcmp(s1, s2)                    __real_strcmp(s1, s2)
#define strncmp(s1, s2, n)                __real_strncmp(s1, s2, n)
#define strcpy(dest, src)                 __real_strcpy(dest, src)
#define strncpy(dest, src, n)             __real_strncpy(dest, src, n)
#define strstr(haystack, needle)          __real_strstr(haystack, needle)
#define strcasestr(haystack, needle)      __real_strcasestr(haystack, needle)

#endif

#ifdef __cplusplus
}
#endif
