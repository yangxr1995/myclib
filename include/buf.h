#ifndef __BUF_H

#define __BUF_H

#include <stdlib.h>
#include <string.h>

// 
//             ┌────────────┬───────────┬───────────────┐
//             │            │           │               │
//             └────────────┴───────────┴───────────────┘
//             ▲            ▲           ▲                ▲
//             │            │           │                │
//             head        pos         tail              end
//          空间头        解析游标    有效数据尾         空间尾
// 
//          已解析范围 : [head, pos)
//          待解析范围 : [pos, tail)
//          剩余空间   : [tail, end)
//          总数据     : [head, tail)
//          总空间     : [head, end)
// 

typedef struct buf_s buf_t;
struct buf_s {
	char *end;
	char *head;
	char *tail;
	char *pos;
};

inline static void buf_cache_push(buf_t *b, unsigned int n)
{
    b->tail += n;
}

inline static void buf_data_push(buf_t *b, unsigned int n)
{
    b->pos += n;
}

inline static unsigned int buf_buf_len(buf_t *b)
{
    return b->end - b->head;
}

inline static unsigned int buf_cache_len(buf_t *b)
{
    return b->tail - b->head;
}

inline static unsigned int buf_room_len(buf_t *b)
{
    return b->end - b->tail;
}

inline static unsigned int buf_data_len(buf_t *b)
{
    return b->pos - b->head;
}

inline static unsigned int buf_raw_data_len(buf_t *b)
{
    return b->tail - b->pos;
}

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
	int sz = buf_raw_data_len(b);
	memmove(b->head, b->pos, sz);
	b->pos = b->head;
	b->tail = b->pos + sz;
}

#endif
