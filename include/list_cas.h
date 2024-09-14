#ifndef __LIST_CAS_H_
#define __LIST_CAS_H_

#include "atomic.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct list_head {
	struct list_head *next, *prev;
    _Atomic bool is_del;
} list_head_t;

#define LIST_HEAD_INITIALIZER(name) { &(name), &(name) }
#define list_head_def(name) \
	list_head_t name = LIST_HEAD_INITIALIZER(name)

#define list_head_init(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline bool __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
    new->next = next;
    new->prev = prev;
    if (cmpxchg_weak(&prev->next, next, new)) {
        next->prev = new;
        return true;
    }
    return false;
}

static inline void list_add_head(struct list_head *new, struct list_head *head)
{
	while (!__list_add(new, head, head->next))
        NULL;
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    while (!__list_add(new, head->prev, head))
        NULL;
}

static inline bool __list_del(struct list_head *node, 
        struct list_head * prev, 
        struct list_head * next)
{
    if (cmpxchg_weak(&prev->next, node, next)) {
        next->prev = prev;
        return true;
    }
    return false;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry, entry->prev, entry->next);
}

#define container_of(ptr, type, member)                         \
    __extension__({                                             \
        const __typeof__(((type *) 0)->member) *__mptr = (ptr); \
        (type *) ((char *) __mptr - offsetof(type, member));    \
    })

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
		pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); \
		pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))



#endif
