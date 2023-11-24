#ifndef __ATOMIC_H
#define __ATOMIC_H

#define atomic_set(ptr, v) do { \
	__sync_lock_test_and_set(ptr, v); \
} while (0)

#define atomic_zero(ptr) do { \
	__sync_lock_release(ptr);  \
} while (0)

#define atomic_cmp(ptr, v) __sync_bool_compare_and_swap(ptr, v, v)

#define atomic_cmp_set(ptr, v, new) __sync_bool_compare_and_swap(ptr, v, new)

#define memory_barrier() __sync_synchronize()

#define atomic_add_and_fetch(ptr, v) __sync_add_and_fetch(ptr, v)

#define atomic_sub_and_fetch(ptr, v) __sync_sub_and_fetch(ptr, v)

#define atomic_fetch_and_sub(ptr, v) __sync_fetch_and_sub(ptr, v)

#endif
