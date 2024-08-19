#ifndef __TLV_H_
#define __TLV_H_

#include <arpa/inet.h>
#include <alloca.h>
#include <string.h>

#include "assert.h"
#include "logger.h"
#include "mm_pool.h"
#include "type.h"

#define PAYLOAD_GET_STR(pfrom, to, len) do{ \
	if (len == 0) { \
		to = NULL; \
		break; \
	}\
	to = (char *)mpool_alloc(mp, len + 1); \
	to[len] = 0; \
	strncpy(to, (char *)pfrom, len); \
	pfrom += len; \
} while (0)

#define PAYLOAD_SET(ptr, val) do { \
	int _offset; \
	int _type_sz = sizeof(val); \
	void *_p = &(val); \
	switch (_type_sz) { \
		case 1: \
			*((u8 *)ptr) = *(u8 *)_p; \
			_offset = 1; \
			break; \
		case 2: \
			*(u16 *)(ptr) = htons(*(u16 *)_p); \
			_offset = 2; \
			break; \
		case 4: \
			*(u32 *)(ptr) = htonl(*(u32 *)_p); \
			_offset = 4; \
			break; \
		default: \
			memcpy(ptr, _p, _type_sz); \
			_offset = _type_sz; \
	} \
	(ptr) += _offset; \
} while (0)

#define PAYLOAD_SET_STR(p, val, len) do{ \
	if (len == 0 || val == NULL) \
		break; \
	memcpy(p, val, len); \
	p += len; \
}while (0)

#define PAYLOAD_GET(pfrom, to) do { \
	int _offset = 0; \
	int _type_sz = sizeof(to); \
	char *_p = (char *)&(to); \
	switch (_type_sz) { \
		case 1: \
			*(u8 *)_p = *(u8 *)pfrom; \
			_offset = 1; \
			break; \
		case 2: \
			*(u16 *)_p = ntohs(*(u16 *)pfrom); \
			_offset = 2; \
			break; \
		case 4: \
			*(u32 *)_p = ntohl(*(u32 *)pfrom); \
			_offset = 4; \
			break; \
		default: \
			memcpy(_p, pfrom, _type_sz); \
			_offset = _type_sz; \
	} \
	pfrom += _offset; \
} while (0)


typedef struct tlv_s tlv_t;
typedef struct tlv_fn_tb_s tlv_fn_tb_t;
typedef struct tlv_type_desc_s tlv_type_desc_t;

struct tlv_s {
	tlv_fn_tb_t *fn_tb;	
    unsigned short id;
    unsigned short len;
    void *val;
}__attribute__((packed));

struct tlv_fn_tb_s {
	tlv_t *(*init)(tlv_t *tlv, void *val, int len, mpool_t *mp);
	char *(*pl_set)(tlv_t *tlv, char *p);
	char *(*pl_get)(tlv_t *tlv, char *p, mpool_t *mp);
	void (*print)(tlv_t *tlv);
	int (*exec)(tlv_t *tlv, void *arg);
};

struct tlv_type_desc_s {
	unsigned short id;	
	tlv_fn_tb_t *fn_tb;
};

inline static tlv_type_desc_t *
tlv_type_desc_map(tlv_type_desc_t **arr, int sz, tlv_type_desc_t *desc) {
    tlv_type_desc_t *tmp;
    assert(desc->id < sz);
    tmp = arr[desc->id];
    arr[desc->id] = desc;
    return tmp;
}

inline static tlv_type_desc_t *
tlv_type_desc_new(unsigned short id, tlv_fn_tb_t *fn_tb, int (*exec)(tlv_t *, void *))
{
    tlv_type_desc_t *desc = (tlv_type_desc_t *)malloc(sizeof(*desc));
    desc->id = id;
    desc->fn_tb = (tlv_fn_tb_t *)malloc(sizeof(*fn_tb));
    memcpy(desc->fn_tb, fn_tb, sizeof(*fn_tb));
    desc->fn_tb->exec = exec;

    return desc;
}

inline static char *
tlv_pl_get(tlv_t **ptlv, char *p, tlv_type_desc_t **arr, mpool_t *mp) {
	unsigned short id;
	unsigned short len;
    tlv_type_desc_t *desc;
    tlv_t *tlv;

	PAYLOAD_GET(p, id);
	PAYLOAD_GET(p, len);
    log_debug("id[%d] len[%d]", id, len);

    if ((desc = arr[id]) == NULL) {
        return NULL;
    }
    tlv = (tlv_t *)mpool_alloc(mp, sizeof(*tlv));
    tlv->id = id;
    tlv->len = len;
    tlv->fn_tb = desc->fn_tb;
    tlv->val = NULL;
    p = tlv->fn_tb->pl_get(tlv, p, mp);
    *ptlv = tlv;

    return p;
}

inline static tlv_t *
tlv_new(unsigned short id, void *val, int val_len, tlv_type_desc_t **arr, int sz,mpool_t *mp) {
	tlv_t *tlv;
    assert(id < sz);
    tlv = (tlv_t *)mpool_alloc(mp, sizeof(*tlv));
    tlv->id = id;
    tlv->len = val_len;
    tlv->fn_tb = arr[id]->fn_tb;
    tlv->fn_tb->init(tlv, val, val_len, mp);
	return tlv;
}

inline static char *
tlv_pl_set(tlv_t *tlv, char *p) {
    PAYLOAD_SET(p, tlv->id);
    PAYLOAD_SET(p, tlv->len);
	return tlv->fn_tb->pl_set(tlv, p);
}

inline static char *
tlv_set(char *p, unsigned short id, void *val, int val_len, tlv_type_desc_t **arr, int sz,mpool_t *mp) {
    return tlv_pl_set(tlv_new(id, val, val_len, arr, sz, mp), p);
}

typedef struct tlv_str_s tlv_str_t;
struct tlv_str_s {
	tlv_fn_tb_t *fn_tb;	
	unsigned short id;
	unsigned short len;
	char *val;
}__attribute__((packed));

typedef struct tlv_byte_s tlv_byte_t;
struct tlv_byte_s {
	tlv_fn_tb_t *fn_tb;	
	unsigned short id;
	unsigned short len;
	unsigned char val;
}__attribute__((packed));

typedef struct tlv_word_s tlv_word_t;
struct tlv_word_s {
	tlv_fn_tb_t *fn_tb;	
	unsigned short id;
	unsigned short len;
	unsigned short val;
}__attribute__((packed));

typedef struct tlv_dword_s tlv_dword_t;
struct tlv_dword_s {
	tlv_fn_tb_t *fn_tb;	
	unsigned short id;
	unsigned short len;
	unsigned int val;
}__attribute__((packed));

typedef struct tlv_null_s tlv_null_t;
struct tlv_null_s {
	tlv_fn_tb_t *fn_tb;	
	unsigned short id;
	unsigned short len;
}__attribute__((packed));

// 内存块
typedef struct tlv_buf_s tlv_buf_t;
struct tlv_buf_s {
    tlv_fn_tb_t *fn_tb;
    unsigned short id;
    unsigned short len; // 内存块大小
    char *val;
}__attribute__((packed));

inline static char *
tlv_buf_pl_set(tlv_t *tlv, char *p) {
	tlv_buf_t *buf = (tlv_buf_t *)tlv;
    PAYLOAD_SET_STR(p, buf->val, buf->len);
	return p;
}

inline static char *
tlv_null_pl_set(tlv_t *tlv, char *p) {
	tlv_null_t *null = (tlv_null_t *)tlv;
	return p;
}

inline static char *
tlv_word_pl_set(tlv_t *tlv, char *p) {
	tlv_word_t *word = (tlv_word_t *)tlv;
	PAYLOAD_SET(p, word->val);
	return p;
}

inline static char *
tlv_dword_pl_set(tlv_t *tlv, char *p) {
	tlv_dword_t *dword = (tlv_dword_t *)tlv;	
	PAYLOAD_SET(p, dword->val);
	return p;
}

inline static char *
tlv_str_pl_set(tlv_t *tlv, char *p) {
	tlv_str_t *tlv_str = (tlv_str_t *)tlv;
	PAYLOAD_SET_STR(p, tlv_str->val, tlv_str->len);
	return p;
}

inline static char *
tlv_byte_pl_set(tlv_t *tlv, char *p) {
	tlv_byte_t *tlv_byte = (tlv_byte_t *)tlv;
	PAYLOAD_SET(p, tlv_byte->val);
	return p;
}

inline static void
tlv_buf_print(tlv_t *tlv) {
	tlv_buf_t *buf = (tlv_buf_t *)tlv;	
	log_info("id[0x%X]len[%d]\n", buf->id, buf->len);
}

inline static void
tlv_null_print(tlv_t *tlv) {
	tlv_null_t *null = (tlv_null_t *)tlv;	
	log_info("id[0x%X]len[%d]\n", null->id, null->len);
}

inline static void
tlv_dword_print(tlv_t *tlv) {
	tlv_dword_t *dword = (tlv_dword_t *)tlv;	
	log_info("id[0x%X]len[%d]val[0x%X]\n", dword->id, dword->len, dword->val);
}

inline static void
tlv_word_print(tlv_t *tlv) {
	tlv_word_t *word = (tlv_word_t *)tlv;	
	log_info("id[0x%X]len[%d]val[0x%X]\n", word->id, word->len, word->val);
}

inline static void
tlv_str_print(tlv_t *tlv) {
	tlv_str_t *str = (tlv_str_t *)tlv;	
	log_info("id[0x%X]len[%d]val[%s]\n", str->id, str->len, str->val);
}

inline static void
tlv_byte_print(tlv_t *tlv) {
	tlv_byte_t *byte = (tlv_byte_t *)tlv;	
	log_info("id[0x%X]len[%d]val[0x%X]\n", byte->id, byte->len, byte->val);
}

// #define tlv_len(tlv) ((tlv)->len + sizeof((tlv)->id) + sizeof((tlv)->len))

inline static tlv_t *tlv_buf_init   (tlv_t *tlv, void *val, int len, mpool_t *mp);
inline static tlv_t *tlv_word_init  (tlv_t *tlv, void *val, int len, mpool_t *mp);
inline static tlv_t *tlv_byte_init  (tlv_t *tlv, void *val, int len, mpool_t *mp);
inline static tlv_t *tlv_dword_init (tlv_t *tlv, void *val, int len, mpool_t *mp);
inline static tlv_t *tlv_str_init   (tlv_t *tlv, void *val, int len, mpool_t *mp);
inline static tlv_t *tlv_null_init  (tlv_t *tlv, void *val, int len, mpool_t *mp);

inline static char *
tlv_null_pl_get(tlv_t *tlv, char *p, mpool_t *mp) {
	return p;
}

inline static char *
tlv_buf_pl_get(tlv_t *tlv, char *p, mpool_t *mp) {
    tlv_buf_t *buf = (tlv_buf_t *)tlv;
    buf->val = (char *)mpool_alloc(mp, buf->len);
    memcpy(buf->val, p, buf->len);
    p += buf->len;
	return p;
}

inline static char *
tlv_word_pl_get(tlv_t *tlv, char *p, mpool_t *mp) {
    tlv_word_t *word = (tlv_word_t *)tlv;
    PAYLOAD_GET(p, word->val);
	return p;
}

inline static char *
tlv_dword_pl_get(tlv_t *tlv, char *p, mpool_t *mp) {
    tlv_dword_t *dword = (tlv_dword_t *)tlv;
    PAYLOAD_GET(p, dword->val);
	return p;
}

inline static char *
tlv_str_pl_get(tlv_t *tlv, char *p, mpool_t *mp) {
    tlv_str_t *str = (tlv_str_t *)tlv;
    str->val = (char *)mpool_alloc(mp, str->len + 1);
    strncpy(str->val, p, str->len);
    str->val[str->len] = 0;
    p += str->len;
	return p;
}

inline static char *
tlv_byte_pl_get(tlv_t *tlv, char *p, mpool_t *mp) {
    tlv_byte_t *byte = (tlv_byte_t *)tlv;
    byte->val = *p;
    ++p;
	return p;
}

// TODO 
static tlv_fn_tb_t tlv_buf_fn_tb = {
	.init = tlv_buf_init,
	.pl_set = tlv_buf_pl_set,
	.pl_get = tlv_buf_pl_get,
	.print = tlv_buf_print,
};

static tlv_fn_tb_t tlv_null_fn_tb = {
	.init = tlv_null_init,
	.pl_set = tlv_null_pl_set,
	.pl_get = tlv_null_pl_get,
	.print = tlv_null_print,
};

static tlv_fn_tb_t tlv_word_fn_tb = {
	.init = tlv_word_init,
	.pl_set = tlv_word_pl_set,
	.pl_get = tlv_word_pl_get,
	.print = tlv_word_print,
};

static tlv_fn_tb_t tlv_dword_fn_tb = {
	.init = tlv_dword_init,
	.pl_set = tlv_dword_pl_set,
	.pl_get = tlv_dword_pl_get,
	.print = tlv_dword_print,
};

static tlv_fn_tb_t tlv_str_fn_tb = {
	.init = tlv_str_init,
	.pl_set = tlv_str_pl_set,
	.pl_get = tlv_str_pl_get,
	.print = tlv_str_print,
};

static tlv_fn_tb_t tlv_byte_fn_tb = {
	.init = tlv_byte_init,
	.pl_set = tlv_byte_pl_set,
	.pl_get = tlv_byte_pl_get,
	.print = tlv_byte_print,
};

inline static tlv_t *
tlv_buf_init(tlv_t *tlv, void *val, int len, mpool_t *mp) {
	tlv_buf_t *buf = (tlv_buf_t *)tlv;
    buf->val = (char *)mpool_alloc(mp, len);
    memcpy(buf->val, val, len);
	buf->fn_tb = &tlv_buf_fn_tb;
    buf->len = len;
	return (tlv_t *)buf;
}

inline static tlv_t *
tlv_null_init(tlv_t *tlv, void *val, int len, mpool_t *mp) {
	tlv_null_t *null = (tlv_null_t *)tlv;
	null->fn_tb = &tlv_null_fn_tb;
	null->len = 0;
	return (tlv_t *)null;
}

inline static tlv_t *
tlv_word_init(tlv_t *tlv, void *val, int len, mpool_t *mp) {
	tlv_word_t *word = (tlv_word_t *)tlv;
	word->fn_tb = &tlv_word_fn_tb;
	word->len = 2;
	word->val = *(unsigned short *)val;
	return (tlv_t *)word;
}

inline static tlv_t *
tlv_byte_init(tlv_t *tlv, void *val, int len, mpool_t *mp) {
	tlv_byte_t *byte = (tlv_byte_t *)tlv;
	byte->fn_tb = &tlv_byte_fn_tb;
	byte->len = 1;
	byte->val = *(unsigned char *)val;
	return (tlv_t *)byte;
}

inline static tlv_t *
tlv_dword_init(tlv_t *tlv, void *val, int len, mpool_t *mp) {
	tlv_dword_t *dword = (tlv_dword_t *)tlv;
	dword->fn_tb = &tlv_dword_fn_tb;
	dword->len = 4;
	dword->val = *(unsigned int *)val;
	return (tlv_t *)dword;
}

inline static tlv_t *
tlv_str_init(tlv_t *tlv, void *val, int len, mpool_t *mp) {
	tlv_str_t *str = (tlv_str_t *)tlv;
	str->len = strlen((char *)val);
	str->val = (char *)mpool_alloc(mp, str->len + 1);
    str->val[str->len] = 0;
	str->fn_tb = &tlv_str_fn_tb;
    memcpy(str->val, val, str->len);
	return (tlv_t *)str;
}

inline static
void tlv_print(tlv_t *tlv) {
	tlv->fn_tb->print(tlv);
}

inline static int
tlv_exec(tlv_t *tlv, void *arg) {
    if (tlv->fn_tb->exec)
        return tlv->fn_tb->exec(tlv, arg);
	return 0;
}

#endif
