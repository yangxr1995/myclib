#ifndef __MM_POOL_H
#define __MM_POOL_H

#include <stdlib.h>

#define MM_BLOCK	(512)

typedef struct mpool_free_func_s mpool_free_func_t;
struct mpool_free_func_s {
	mpool_free_func_t *next;
	void (*free_func)(void *data);
	void *data;
};

typedef struct mpool mpool_t;

/*
 * prev：指向内存池
 * avail: 指向内存池的起始地址
 * limit: 指向内存池的结束地址
 */
struct mpool {
	struct mpool *prev;	
	char *avail;
	char *limit;
	mpool_free_func_t *free_list;
};

/*
 * 描述：
 * 	创建一个内存池
 */
extern struct mpool *mpool_new(void);

extern void mpool_init(mpool_t *pool);

/*
 * 描述：
 * 	释放内存池，和内存池本身
 */
extern void mpool_destroy(struct mpool **mpool);

/*
 * 描述：
 * 	在 mpool 内存池上分配nbytes大小空间
 */
extern void *mpool_alloc(struct mpool *mpool, size_t nbytes);
/*
 * 描述：
 * 	释放mpool内存池所有内存
 */
extern void mpool_free(struct mpool *mpool);
/*
 * 描述：
 * 	清空mpool内存池中内存的占用，但不释放内存
 */
extern void mpool_clear(struct mpool *mpool);

extern void mpool_debug(void);

extern mpool_free_func_t *mpool_free_func_alloc(mpool_t *pool);
extern void mpool_do_free_list(struct mpool *mpool);

#endif
