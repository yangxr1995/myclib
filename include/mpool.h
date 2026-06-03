#ifndef __mpool_H__
#define __mpool_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#define MM_POOL_ALIGNMENT   16
#define MM_ALIGN_PTR(p, a)  \
    (void *)(((uintptr_t)(p) + ((a) - 1)) & ~((uintptr_t)(a) - 1))

#define MM_MAX_ALLOC_FROM_POOL  (4095)

#define MM_DEFAULT_BLOCK_SIZE   (16 * 1024)

typedef struct mpool_free_func_s  mpool_free_func_t;
typedef struct mpool_block_s      mpool_block_t;
typedef struct mpool_large_s      mpool_large_t;
typedef struct mpool              mpool_t;

/* Lightweight block header for pool block extensions */
struct mpool_block_s {
    char           *avail;
    char           *end;
    mpool_block_t  *next;
};

/* Large allocation tracking node */
struct mpool_large_s {
    mpool_large_t  *next;
    void           *alloc;
};

/*
 * Cleanup callback — executed inside the pool spinlock.
 *
 * WARNING: free_func MUST NOT call any pool function (mpool_alloc,
 * mpool_reset, mpool_pfree, …) or a deadlock will occur.
 */
struct mpool_free_func_s {
    mpool_free_func_t  *next;
    void             (*free_func)(void *data);
    void              *data;
};

/* Main pool structure (head only) */
struct mpool {
    mpool_block_t      *blocks;      /* block list head        */
    mpool_block_t      *current;     /* current allocation blk */
    size_t              max;         /* small/large threshold  */
    mpool_large_t      *large;       /* large alloc list       */
    mpool_free_func_t  *free_list;   /* cleanup callbacks      */
    pthread_spinlock_t  locker;      /* only head has a lock   */
};

extern mpool_t *mpool_new(void);
extern void     mpool_destroy(mpool_t **mpool);
extern void    *mpool_alloc(mpool_t *mpool, size_t nbytes);
extern void    *mpool_nalloc(mpool_t *mpool, size_t nbytes);
extern void     mpool_reset(mpool_t *mpool);
extern int      mpool_pfree(mpool_t *mpool, void *p);

extern mpool_free_func_t *mpool_cleanup_add(mpool_t *pool, size_t size);
extern void               mpool_run_cleanup(mpool_t *mpool);

/* Convenience: allocate zeroed memory */
inline static void *mpool_calloc(mpool_t *mp, size_t nbytes)
{
    if (nbytes == 0) {
        nbytes = 1;
    }
    void *p = mpool_alloc(mp, nbytes);
    if (p) {
        memset(p, 0, nbytes);
    }
    return p;
}

/* Convenience: strdup from pool */
inline static char *mpool_strdup(const char *ptr, mpool_t *mp)
{
    size_t len = strlen(ptr);
    char  *data = (char *)mpool_alloc(mp, len + 1);
    if (data) {
        memcpy(data, ptr, len);
        data[len] = '\0';
    }
    return data;
}

#ifdef __cplusplus
}
#endif

#endif /* __mpool_H__ */
