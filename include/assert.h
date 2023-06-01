#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#include <stdio.h>
int _assert(void);
#define assert(e) ((void)((e)|| fprintf(stderr, "assert at %s +%d : %s\n", __FILE__, __LINE__, #e) && _assert()))
#endif
