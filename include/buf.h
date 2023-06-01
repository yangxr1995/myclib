#ifndef BUF_H

#define BUF_H

#include "str.h"

typedef struct buf_s buf_t;
struct buf_s {
	char *data;
	char *pos;
	char *end;
	size_t size;
};

#define buf_set(_buf, _ptr, _size) do {	\
	char *__ptr = _ptr;	\
	int __size = _size;	\
	buf_t *__buf = _buf;	\
	__buf->data = __ptr;	\
	__buf->pos = __ptr;	\
	__buf->end = __ptr + __size;\
	__buf->size = __size;	\
} while (0)

#define buf_get_str(buf, str) do { \
	buf_t *_buf = buf; \
	str_t *_str = str; \
	_str->data = _buf->data; \
	_str->len = _buf->pos - _buf->data; \
} while (0)

#endif /* end of include guard: BUF_H */
