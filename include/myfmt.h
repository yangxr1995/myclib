#ifndef MYFMT_H

#define MYFMT_H

#include <stdarg.h>
#include <stdio.h>

typedef void (*fmt_t)(int code, va_list *app,
	int put(int c, void *cl), void *cl);

extern void fmt_fmt(int put(int c, void *cl), void *cl, const char *fmt, ...);
extern void fmt_fprint(FILE *stream, const char *fmt, ...);
extern void fmt_print(char *fmt, ...);

#endif /* end of include guard: MYFMT_H */
