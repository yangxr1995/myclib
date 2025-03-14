#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TRACE_H_
#define __TRACE_H_

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


#endif

#ifdef __cplusplus
}
#endif
