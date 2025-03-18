#ifdef WRAP_DEFINE

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

#else

#define system(command)     __real_system(command)
#define sleep(seconds)      __real_sleep(seconds)

#endif
