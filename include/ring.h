#ifndef __RING_H__
#define __RING_H__

#include <stdlib.h>
#include <string.h>

typedef struct ring_s ring_t;

struct ring_s {
    unsigned int cap;
    unsigned int idx_begin;
    unsigned int idx_end;
    unsigned int  size;
    unsigned int  nelts;
    void *elts;
};

inline static ring_t *
ring_new(unsigned int cap, unsigned int size) {
    ring_t *r = (ring_t *)malloc(sizeof(*r) + cap * size);
    r->cap = cap;
    r->idx_end = 0;
    r->idx_begin = 0;
    r->size = size;
    r->nelts = 0;
    r->elts = (char *)r + sizeof(*r);

    return r;
}

inline static void
ring_delete(ring_t *r) {
    free(r);
}

inline static void
ring_expand(ring_t **pr) {
    ring_t *r = *pr;
    ring_t *tmp = (ring_t *)malloc(sizeof(*r) + r->cap * 2 * r->size);

    tmp->size = r->size;
    tmp->cap = r->cap * 2;
    tmp->nelts = r->nelts;
    tmp->elts = (char *)tmp + sizeof(*r);

    if (r->idx_begin < r->idx_end) {
        memcpy((char *)tmp->elts + r->idx_begin * r->size, 
                (char *)r->elts + r->idx_begin * r->size, 
                (r->idx_end - r->idx_begin) * r->size);
        tmp->idx_begin = r->idx_begin;
        tmp->idx_end = r->idx_end;
    }
    else {
        memcpy((char *)tmp->elts + r->idx_begin * r->size, 
                (char *)r->elts + r->idx_begin * r->size, 
                (r->cap - r->idx_begin) * r->size);

        memcpy((char *)tmp->elts + r->cap * r->size, 
                (char *)r->elts,
                r->idx_end * r->size);

        tmp->idx_begin = r->idx_begin;
        tmp->idx_end = r->cap + r->idx_end;
    }

    free(r);
    *pr = tmp;
}

inline static void * 
ring_push(ring_t **pr)
{
    ring_t *r = *pr;
    void *ret;

    if (r->nelts == r->cap) {
        ring_expand(&r);
        *pr = r;
    }

    ret = (char *)r->elts + r->size * r->idx_end;

    ++r->idx_end;
    if (r->idx_end >= r->cap)
        r->idx_end = 0;

    ++r->nelts;

    return ret;
}

inline static ring_t * 
ring_push_h(ring_t *r, void *v)
{
    if (r->nelts == r->cap) {
        ring_expand(&r);
    }

    memcpy((char *)r->elts + r->size * r->idx_end, v, r->size);

    ++r->idx_end;
    if (r->idx_end >= r->cap)
        r->idx_end = 0;

    ++r->nelts;

    return r;
}

inline static void
ring_tighten(ring_t *r, unsigned int step)
{
    if (r->nelts < step)
        r->nelts = 0;
    else
        r->nelts -= step;

    if (r->nelts == 0) {
        r->idx_begin = r->idx_end;
    }
    else {
        r->idx_begin = r->idx_begin + step;
        if (r->idx_begin > r->cap - 1)
            r->idx_begin = r->idx_begin - r->cap;
    }
}

#define ring_for_each(r, pos) \
	for ( int i = 0 || ( pos = ((typeof(pos))((r)->elts))  + (r)->idx_begin );  \
            i != (r)->nelts + 1; \
            ++i &&  (pos = pos + 1 ==   (typeof(pos))((r)->elts) + (r)->cap ?  (typeof(pos))((r)->elts) : pos + 1  ) )

#define ring_for_each_idx(r, pos, offset) \
	for ( offset = 0 , ( pos = ((typeof(pos))((r)->elts))  + (r)->idx_begin );  \
            offset != (r)->nelts; \
            ++offset &&  (pos = pos + 1 ==   (typeof(pos))((r)->elts) + (r)->cap ?  (typeof(pos))((r)->elts) : pos + 1  ) )


/*for (int i = 0 && (  pos =  (typeof(pos)(r)->elts) + r->idx_begin   ); \*/
//        ++i && ( pos = (pos + 1 ==  typeof(pos)(r)->elts +r->cap ?  typeof(pos)(r)-elts : pos + 1)  )

/*for (int i = 0 &&  (pos = (typeof(pos))((r)->elts) + r->idx_begin);  \*/
/*        i < r->nelts; \*/
/*        ++i && (pos = pos + 1 == (typeof(pos))((r)->elts) + r->cap ? (typeof(pos))((r)->elts) : pos + 1) )*/
/**/

#endif
