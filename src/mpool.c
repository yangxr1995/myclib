#include "mpool.h"

#include <stdio.h>
#include <assert.h>

#define USE_SPINLOCK   0

static void  run_cleanup_unlocked(mpool_t *pool);
static void *alloc_small(mpool_t *pool, size_t size, int align);
static void *alloc_block(mpool_t *pool, size_t size, int align);
static void *alloc_large(mpool_t *pool, size_t size);

mpool_t *
mpool_new(void)
{
    mpool_t *pool;
    size_t   block_size;

    pool = malloc(sizeof(mpool_t));
    if (pool == NULL) {
        return NULL;
    }

    block_size = MM_DEFAULT_BLOCK_SIZE;

    {
        void *blk;
        if (posix_memalign(&blk, MM_POOL_ALIGNMENT, block_size) != 0) {
            free(pool);
            return NULL;
        }
        pool->blocks = (mpool_block_t *)blk;
    }

    pool->blocks->avail  = (char *)pool->blocks + sizeof(mpool_block_t);
    pool->blocks->avail  = MM_ALIGN_PTR(pool->blocks->avail, MM_POOL_ALIGNMENT);
    pool->blocks->end    = (char *)pool->blocks + block_size;
    pool->blocks->next   = NULL;

    {
        size_t remaining = block_size - sizeof(mpool_block_t);
        pool->max = (remaining < MM_MAX_ALLOC_FROM_POOL)
                    ? remaining : MM_MAX_ALLOC_FROM_POOL;
    }

    pool->current   = pool->blocks;
    pool->large     = NULL;
    pool->free_list = NULL;

#if USE_SPINLOCK
    pthread_spin_init(&pool->locker, 0);
#endif

    return pool;
}

void
mpool_destroy(mpool_t **mpool)
{
    mpool_t         *pool;
    mpool_block_t   *blk, *next_blk;
    mpool_large_t   *lg;

    assert(mpool);
    if (*mpool == NULL) {
        return;
    }
    pool = *mpool;

#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif

    run_cleanup_unlocked(pool);

    for (lg = pool->large; lg; lg = lg->next) {
        if (lg->alloc) {
            free(lg->alloc);
        }
    }

    for (blk = pool->blocks; blk; blk = next_blk) {
        next_blk = blk->next;
        free(blk);
    }

#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
    pthread_spin_destroy(&pool->locker);
#endif

    free(pool);
    *mpool = NULL;
}

void
mpool_reset(mpool_t *pool)
{
    mpool_block_t *blk;
    mpool_large_t *lg;

    assert(pool);

#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif

    run_cleanup_unlocked(pool);

    for (lg = pool->large; lg; lg = lg->next) {
        if (lg->alloc) {
            free(lg->alloc);
            lg->alloc = NULL;
        }
    }
    pool->large = NULL;

    for (blk = pool->blocks; blk; blk = blk->next) {
        blk->avail  = (char *)blk + sizeof(mpool_block_t);
        blk->avail  = MM_ALIGN_PTR(blk->avail, MM_POOL_ALIGNMENT);
    }

    pool->current = pool->blocks;

#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
#endif
}

void *
mpool_alloc(mpool_t *pool, size_t nbytes)
{
    void *ret;

    assert(pool);

    if (nbytes == 0) {
        nbytes = 1;
    }

    assert(nbytes > 0);

#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif

    if (nbytes <= pool->max) {
        ret = alloc_small(pool, nbytes, 1);
    } else {
        ret = alloc_large(pool, nbytes);
    }

#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
#endif

    return ret;
}

void *
mpool_nalloc(mpool_t *pool, size_t nbytes)
{
    void *ret;

    assert(pool);

    if (nbytes == 0) {
        nbytes = 1;
    }

    assert(nbytes > 0);

#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif

    if (nbytes <= pool->max) {
        ret = alloc_small(pool, nbytes, 0);
    } else {
        ret = alloc_large(pool, nbytes);
    }

#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
#endif

    return ret;
}

/* Free a single large allocation. Returns 0 on success, -1 if not found. */
int
mpool_pfree(mpool_t *pool, void *p)
{
    mpool_large_t **prev;

    assert(pool);

#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif

    for (prev = &pool->large; *prev; prev = &(*prev)->next) {
        if (p == (*prev)->alloc) {
            mpool_large_t *lg = *prev;
            *prev = lg->next;
            free(lg->alloc);

#if USE_SPINLOCK
            pthread_spin_unlock(&pool->locker);
#endif
            return 0;
        }
    }

#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
#endif

    return -1;
}

mpool_free_func_t *
mpool_cleanup_add(mpool_t *pool, size_t size)
{
    mpool_free_func_t *c;
    void              *data;

    assert(pool);

#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif

    /* Allocate data first: if this fails, no pool memory is wasted.
     * With a bump allocator we cannot reclaim individual allocations,
     * so ordering matters — data (potentially large) before the small
     * header node minimises waste on partial failure. */
    if (size > 0) {
        data = alloc_small(pool, size, 1);
        if (data == NULL) {
#if USE_SPINLOCK
            pthread_spin_unlock(&pool->locker);
#endif
            return NULL;
        }
    } else {
        data = NULL;
    }

    c = alloc_small(pool, sizeof(*c), 1);
    if (c == NULL) {
        /* data bytes are stranded in the pool block; unavoidable with
         * a bump allocator — reclaimed on pool reset/destroy. */
#if USE_SPINLOCK
        pthread_spin_unlock(&pool->locker);
#endif
        return NULL;
    }

    c->next      = pool->free_list;
    c->free_func = NULL;
    c->data      = data;

    pool->free_list = c;

#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
#endif

    return c;
}

void
mpool_run_cleanup(mpool_t *pool)
{
    assert(pool);

#if USE_SPINLOCK
    pthread_spin_lock(&pool->locker);
#endif

    run_cleanup_unlocked(pool);

#if USE_SPINLOCK
    pthread_spin_unlock(&pool->locker);
#endif
}

/* ================================================================
 * internal
 * ================================================================ */

static void
run_cleanup_unlocked(mpool_t *pool)
{
    mpool_free_func_t *c;

    for (c = pool->free_list; c; c = c->next) {
        if (c->free_func) {
            c->free_func(c->data);
        }
    }
    pool->free_list = NULL;
}

static void *
alloc_small(mpool_t *pool, size_t size, int align)
{
    char           *m;
    mpool_block_t  *blk;

    blk = pool->current;

    do {
        m = blk->avail;

        if (align) {
            m = MM_ALIGN_PTR(m, MM_POOL_ALIGNMENT);
        }

        if ((size_t)(blk->end - m) >= size) {
            blk->avail = m + size;
            return m;
        }

        blk = blk->next;
    } while (blk);

    return alloc_block(pool, size, align);
}

static void *
alloc_block(mpool_t *pool, size_t size, int align)
{
    char           *m;
    void           *mem;
    size_t          psize;
    mpool_block_t  *new_blk, *p;

    psize = (size_t)(pool->blocks->end - (char *)pool->blocks);

    if (posix_memalign(&mem, MM_POOL_ALIGNMENT, psize) != 0) {
        return NULL;
    }
    m = (char *)mem;

    new_blk = (mpool_block_t *)m;
    new_blk->end  = m + psize;
    new_blk->next = NULL;

    m += sizeof(mpool_block_t);

    if (align) {
        m = MM_ALIGN_PTR(m, MM_POOL_ALIGNMENT);
    }

    new_blk->avail = m + size;

    for (p = pool->current; p->next; p = p->next)
        ;

    p->next = new_blk;
    pool->current = new_blk;

    return m;
}

static void *
alloc_large(mpool_t *pool, size_t size)
{
    void          *p;
    mpool_large_t *lg;

    p = malloc(size);
    if (p == NULL) {
        return NULL;
    }

    lg = alloc_small(pool, sizeof(mpool_large_t), 1);
    if (lg == NULL) {
        free(p);
        return NULL;
    }

    lg->alloc   = p;
    lg->next    = pool->large;
    pool->large = lg;

    return p;
}
