#ifdef __cplusplus
extern "C" {
#endif

#ifdef WRAP_DEFINE

#include <pthread.h>

wrap_define(int, pthread_create, pthread_t *thread, const pthread_attr_t *attr,
                  void *(*start_routine) (void *), void *arg)
{
    char *this_sym; 
    void *this; 
    int ret;

    /*ret = __real_pthread_create(thread, attr, start_routine, arg);*/
    confirm_addr_info((void *)start_routine, &this, &this_sym); 
    log_wrap_lib_info("pthread_create([%s]%p tid[%lu]", this_sym, this, *thread);

    return ret;
}

#else

#define pthread_create(thread, attr, start_routine, arg)   __real_pthread_create(thread, attr, start_routine, arg)

#endif

#ifdef __cplusplus
}
#endif
