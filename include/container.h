/*
 * Soft:        Keepalived is a failover program for the LVS project
 *              <www.linuxvirtualserver.org>. It monitor & manipulate
 *              a loadbalanced server pool using multi-layer checks.
 *
 * Part:        container.h include file.
 *
 * Author:      Alexandre Cassen, <acassen@linux-vs.org>
 *
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *              See the GNU General Public License for more details.
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Copyright (C) 2019-2019 Alexandre Cassen, <acassen@gmail.com>
 */

#ifndef _CONTAINER_H
#define _CONTAINER_H

#include "align.h"

#if defined _HAVE_FUNCTION_ATTRIBUTE_ERROR_ && (!defined _HAVE_WARNING_NESTED_EXTERNS_ || defined _HAVE_DIAGNOSTIC_PUSH_POP_PRAGMAS_)

/* Copied from linux kernel 5.15 source include/linux/{build_bug,compiler_types,compiler_attributes}.h */

/*
 * 解释下 __attribute__((error(msg))) 的作用。
 * 当有个函数使用设置上面的属性，如
 * void func() __attribute__((error("Result of error")));
 * 则当编译器编译到这个函数的调用时，编译报错，并打印error()的参数
 * 如
 * main() {
 *   func();
 * }
 * 则报错：error on func ... : Result of error
 * 所以这个特性可以实现编译时断言。
 * 此外，由于是编译时使用，所以 func不需要有定义。
 *
 *
 */

#define __compiletime_error(message) __attribute__((error(message)))

# define __compiletime_assert(condition, msg, prefix, suffix)		\
	do {								\
		extern void prefix ## suffix(void) __compiletime_error(msg); \
		if (!(condition))					\
			prefix ## suffix();				\
	} while (0)


#define _compiletime_assert(condition, msg, prefix, suffix) \
	__compiletime_assert(condition, msg, prefix, suffix)

#define compiletime_assert(condition, msg) \
	_compiletime_assert(condition, msg, __compiletime_assert_, __COUNTER__)

/*
 * 注意这里对cond进行反转
 */
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)

/*
 * __builtin_types_compatible_p(type1, type2) 是 gcc的内置函数，用于检查两个类型是否一样。
 */
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

#else
#define BUILD_BUG_ON_MSG(conf, msg)	do {} while (0)
#endif


/* Copied from linux kernel 5.15 source include/linux/{kernel.h,stddef.h} */

#ifndef offsetof
# define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/*
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */

#ifndef container_of
# define container_of(ptr, type, member) ({				\
	BUILD_BUG_ON_MSG(!__same_type(*(ptr), ((type *)0)->member) &&	\
			!__same_type(*(ptr), void),			\
			"pointer type mismatch in container_of()");	\
	typeof( ((type *)0)->member ) *__mptr = (ptr); 			\
	PTR_CAST(type, (char *)__mptr - offsetof(type,member) );})
#endif

#ifndef container_of_const
# define container_of_const(ptr, type, member) ({			\
	BUILD_BUG_ON_MSG(!__same_type(*(ptr), ((type *)0)->member) &&	\
			!__same_type(*(ptr), void),			\
			"pointer type mismatch in container_of()");	\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);  		\
	PTR_CAST_CONST(type, (const char *)__mptr - offsetof(type,member) );})
#endif

#endif
