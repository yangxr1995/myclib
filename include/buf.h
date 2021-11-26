#ifndef BUF_H

#define BUF_H

typedef struct buf_s buf_t;
struct buf_s {
	char *data;
	size_t size;
};

#define buf_set(_buf, _ptr, _size) do {	\
	char *__ptr = _ptr;	\
	int __size = _size;	\
	buf_t *__buf = _buf;	\
	__buf->data = __ptr;	\
	__buf->size = __size;	\
} while (0)



#endif /* end of include guard: BUF_H */
