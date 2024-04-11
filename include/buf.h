#ifndef __BUF_H

#define __BUF_H

#include <stdlib.h>
#include <string.h>

typedef struct buf_s buf_t;
struct buf_s {
	char *end;
	char *head;
	char *tail;
	char *pos;
};

inline static void buf_free(buf_t *b)
{
	free(b->head);
}

inline static void buf_free_new(buf_t *b)
{
	free(b);
}

inline static void buf_clear(buf_t *b)
{
	b->pos = b->tail = b->head;
}

inline static buf_t *buf_new(unsigned int sz)
{
	buf_t *b;

	if ((b = (buf_t *)malloc(sz + sizeof(*b))) == NULL)
		return NULL;
	b->head = (char *)b + sizeof(*b);
	b->end = b->head + sz;
	b->pos = b->head;
	b->tail = b->head;

	return b;
}

inline static int buf_init(buf_t *b, unsigned int sz)
{
	char *p;

	if ((p = (char *)malloc(sz)) == NULL)
		return -1;

	b->head = p;
	b->end = p + sz;
	b->pos = b->head;
	b->tail = b->head;

	return 0;
}

inline static void buf_move(buf_t *b)
{
	int sz = b->tail - b->pos;
	memmove(b->head, b->pos, sz);
	b->pos = b->head;
	b->tail = b->pos + sz;
}

#endif
