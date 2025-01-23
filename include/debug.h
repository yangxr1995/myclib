#ifdef __cplusplus
extern "C" {
#endif

#ifndef __DEBUG_H
#define __DEBUG_H

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/*#define __DEBUG_PRINT_*/

#ifdef __DEBUG_PRINT_
#define debug(format, ...) do { \
	fprintf(stdout, "[%s]%s:%s+%d:" format "\n", _prg, __func__, __FILE__, __LINE__,  ## __VA_ARGS__); \
	fprintf(_dlog_fp, "[%s]%s:%s+%d:" format "\n", _prg, __func__, __FILE__, __LINE__,  ## __VA_ARGS__); \
} while(0)
#else
#define debug(format, ...) NULL
#endif

void __attribute__((__no_instrument_function__))show_prg_info(pid_t pid);
void __attribute__((__no_instrument_function__))print_stacktrace();
void __attribute__((__no_instrument_function__))print_stack();
void __attribute__((__no_instrument_function__))get_filename_by_fd(int fd, char *buf, int sz);
void __attribute__((__no_instrument_function__))debug_on();
void __attribute__((__no_instrument_function__))debug_off();


#endif

#ifdef __cplusplus
}
#endif
