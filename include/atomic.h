#ifndef __ATOMIC_H
#define __ATOMIC_H

/*
 * std::atomic_compare_exchange 参数必须是 _Atomic
 * 所以使用gcc内置cas
 */

#define cmpxchg_strong(ptr, o, n) \
({ \
	typeof(*ptr) _____actual = (o); \
	\
	__atomic_compare_exchange_n((ptr), (void *)&_____actual, (n), 0, \
			__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
})


#define atomic_cmp_set(ptr, v, new) __sync_bool_compare_and_swap(ptr, v, new)

#define cmpxchg_weak(ptr, o, n) \
({ \
	typeof(*ptr) _____actual = (o); \
	\
	__atomic_compare_exchange_n((ptr), (void *)&_____actual, (n), 1, \
			__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
})

#define xchg(ptr, v) __atomic_exchange_n((ptr), (v), __ATOMIC_SEQ_CST)

#endif
