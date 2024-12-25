#include <string.h>
#include <limits.h>

#include "fmt.h"
//#include "except.h"
#include "assert.h"
#include "str.h"

typedef struct fmt_buf_s fmt_buf_t;
struct fmt_buf_s {
	char *buf;
	char *cur;
	char *end;	
};

static void fmt_put(const char *str, int len, int put(int c, void *cl), void *cl);

static void cvt_c(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_d(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_f(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_o(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_p(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_s(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_u(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_x(int code, va_list *app, int put(int c, void *cl), void *cl);
static void cvt_v(int code, va_list *app, int put(int c, void *cl), void *cl);

static fmt_t cvt[256] = {
 /*   0-  7 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*   8- 15 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  16- 23 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  24- 31 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  32- 39 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  40- 47 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  48- 55 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  56- 63 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  64- 71 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  72- 79 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  80- 87 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  88- 95 */ 0,     0, 0,     0,     0,     0,     0,     0,
 /*  96-103 */ 0,     0, 0, cvt_c, cvt_d, cvt_f, cvt_f, cvt_f,
 /* 104-111 */ 0,     0, 0,     0,     0,     0,     0, cvt_o,
 /* 112-119 */ cvt_p, 0, 0, cvt_s,     0, cvt_u, cvt_v,     0,
 /* 120-127 */ cvt_x, 0, 0,     0,     0,     0,     0,     0
};


void 
fmt_vfmt(int put(int c, void *cl), void *cl, const char *fmt, va_list *ap) 
{
	assert(put);	
	assert(fmt);
	
	while (*fmt) {
		// If the form is not '%n' or is '%%'
		// direct output
		if (*fmt != '%' || *++fmt == '%') {
			put((unsigned char)*fmt++, cl);	
		}
		else {
		// If the form is '%n'
		// formatted output
			unsigned char c, flags[256];
			c = *fmt++;
			if (cvt[c] == NULL) {
				printf("c : %c\n", c);
			}
			assert(cvt[c]);
			(*cvt[c])(c, ap, put, cl);
		}
	}
}

void 
fmt_fmt(int put(int c, void *cl), void *cl, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fmt_vfmt(put, cl, fmt, &ap);
	va_end(ap);
}


static void cvt_c(int code, va_list *app, int put(int c, void *cl), void *cl)
{
	put((unsigned char )va_arg(*app, int), cl);
}
static void cvt_d(int code, va_list *app, int put(int c, void *cl), void *cl)
{
	int val = va_arg(*app, int);
	int m;
	char *p, buf[256];
	if (val == INT_MIN) {
		m = INT_MAX + 1U;
	}
	else if (val < 0){
		m = -val;
	}
	else {
		m = val;
	}
	p = buf + sizeof(buf);
	do {
		*--p = m%10 + '0';
	} while ((m /= 10) > 0);
	if (val < 0) {
		*--p = '-';
	}
	fmt_put(p, buf + sizeof(buf) - p, put, cl);
}
static void cvt_f(int code, va_list *app, int put(int c, void *cl), void *cl)
{
}
static void cvt_o(int code, va_list *app, int put(int c, void *cl), void *cl)
{

}
static void 
cvt_p(int code, va_list *app, int put(int c, void *cl), void *cl)
{
	unsigned long m = (unsigned long)va_arg(*app, void *);
	char buf[43];
	char *p = buf + sizeof buf;
	do {
		*--p = "0123456789abcdef"[m & 0xf];
	} while ((m >>= 4) != 0);
	*--p = 'x';
	*--p = '0';
	fmt_put(p, (buf + sizeof buf) - p, put, cl);
}
static void 
cvt_s(int code, va_list *app, int put(int c, void *cl), void *cl)
{
	char *str = va_arg(*app, char *);
	assert(str);
	fmt_put(str, strlen(str), put, cl);
}
static void 
cvt_u(int code, va_list *app, int put(int c, void *cl), void *cl)
{
	unsigned m = va_arg(*app, unsigned);
	char buf[43];
	char *p = buf + sizeof buf;
	do
		*--p = m%10 + '0';
	while ((m /= 10) > 0);
	fmt_put(p, (buf + sizeof buf) - p, put, cl);
}
static void 
cvt_x(int code, va_list *app, int put(int c, void *cl), void *cl)
{
	unsigned m = (unsigned long)va_arg(*app, unsigned);
	char buf[43];
	char *p = (char *)buf + sizeof buf;
	do {
		*--p = "0123456789abcdef"[m&0xf];
	} while ((m >>= 4) != 0);
	fmt_put(p, (buf + sizeof buf) - p, put, cl);
}

static int 
outc(int c, void *cl)
{
	FILE *f = cl;
	return putc(c, f);
}

void 
fmt_fprint(FILE *stream, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fmt_vfmt(outc, stream, fmt, &ap);
	va_end(ap);
}
void 
fmt_print(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fmt_vfmt(outc, stdout, fmt, &ap);
	va_end(ap);
}

static void 
fmt_put(const char *str, int len, int put(int c, void *cl), void *cl)
{
	int i, _len;
	fmt_buf_t *fmt_buf;

	fmt_buf = (fmt_buf_t *)cl;

	for (i = 0; i < len; i++) {
	 _len = fmt_buf->cur - fmt_buf->buf;
		put(str[i], cl);
	}
}

static void 
cvt_v(int code, va_list *app, int put(int c, void *cl), void *cl)
{
	str_t *str;
	str = va_arg(*app, str_t *);
	fmt_put(str->data, str->len, put, cl);
}

static int 
snput(int c, void *cl)
{
	fmt_buf_t *fmt_buf;

	fmt_buf = (fmt_buf_t *)cl;
	if (fmt_buf->cur < fmt_buf->end)
		*fmt_buf->cur++ = c;
	else
		return -1;

	return 0;
}

int 
fmt_snprint(char *str, int size, const char *fmt, ...)
{
	va_list ap;
	void *cl;

	fmt_buf_t fmt_buf;
	fmt_buf.buf = str;
	fmt_buf.cur = str;
	fmt_buf.end = str + size;
	cl = &fmt_buf;

	memset(str, 0x0, size);
	va_start(ap, fmt);
	fmt_vfmt(snput, cl, fmt, &ap);
	va_end(ap);

	return fmt_buf.cur - fmt_buf.buf;
}

int 
fmt_vsnprint(char *str, int size, const char *fmt, va_list *app)
{
	void *cl;

	fmt_buf_t fmt_buf;
	fmt_buf.buf = str;
	fmt_buf.cur = str;
	fmt_buf.end = str + size;
	cl = &fmt_buf;

	memset(str, 0x0, size);
	fmt_vfmt(snput, cl, fmt, app);

	return fmt_buf.cur - fmt_buf.buf;

}
