// SPDX-FileCopyrightText: 2009 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_COMPILER_H
#define _URCU_COMPILER_H

/*
 * Compiler definitions.
 */

#include <stddef.h>	/* for offsetof */

#if defined __cplusplus
# include <type_traits>	/* for std::remove_cv */
#endif

#include <urcu/config.h>

#define caa_likely(x)	__builtin_expect(!!(x), 1)
#define caa_unlikely(x)	__builtin_expect(!!(x), 0)

#ifdef CONFIG_RCU_USE_ATOMIC_BUILTINS
#  define cmm_barrier() __atomic_signal_fence(__ATOMIC_SEQ_CST)
#else
#  define cmm_barrier() __asm__ __volatile__ ("" : : : "memory")
#endif

/*
 * Instruct the compiler to perform only a single access to a variable
 * (prohibits merging and refetching). The compiler is also forbidden to reorder
 * successive instances of CMM_ACCESS_ONCE(), but only when the compiler is aware of
 * particular ordering. Compiler ordering can be ensured, for example, by
 * putting two CMM_ACCESS_ONCE() in separate C statements.
 *
 * This macro does absolutely -nothing- to prevent the CPU from reordering,
 * merging, or refetching absolutely anything at any time.  Its main intended
 * use is to mediate communication between process-level code and irq/NMI
 * handlers, all running on the same CPU.
 */
#define CMM_ACCESS_ONCE(x)	(*(__volatile__  __typeof__(x) *)&(x))

/*
 * If the toolchain support the C11 memory model, define the private macro
 * _CMM_TOOLCHAIN_SUPPORT_C11_MM.
 */
#if ((defined (__cplusplus) && __cplusplus >= 201103L) ||		\
	(defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L))
# define _CMM_TOOLCHAIN_SUPPORT_C11_MM
#elif defined(CONFIG_RCU_USE_ATOMIC_BUILTINS)
#  error "URCU was configured to use atomic builtins, but this toolchain does not support them."
#endif

/* Make the optimizer believe the variable can be manipulated arbitrarily. */
#define _CMM_OPTIMIZER_HIDE_VAR(var)		\
	__asm__ ("" : "+r" (var))

#ifndef caa_max
#define caa_max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef caa_min
#define caa_min(a,b) ((a)<(b)?(a):(b))
#endif

#if defined(__SIZEOF_LONG__)
#define CAA_BITS_PER_LONG	(__SIZEOF_LONG__ * 8)
#elif defined(_LP64)
#define CAA_BITS_PER_LONG	64
#else
#define CAA_BITS_PER_LONG	32
#endif

/*
 * caa_container_of - Get the address of an object containing a field.
 *
 * @ptr: pointer to the field.
 * @type: type of the object.
 * @member: name of the field within the object.
 */
#define caa_container_of(ptr, type, member)				\
	__extension__							\
	({								\
		const __typeof__(((type *) NULL)->member) * __ptr = (ptr); \
		(type *)((char *)__ptr - offsetof(type, member));	\
	})

/*
 * caa_container_of_check_null - Get the address of an object containing a field.
 *
 * @ptr: pointer to the field.
 * @type: type of the object.
 * @member: name of the field within the object.
 *
 * Return the address of the object containing the field. Return NULL if
 * @ptr is NULL.
 */
#define caa_container_of_check_null(ptr, type, member)			\
	__extension__							\
	({								\
		const __typeof__(((type *) NULL)->member) * __ptr = (ptr); \
		(__ptr) ? (type *)((char *)__ptr - offsetof(type, member)) : NULL; \
	})

#define CAA_BUILD_BUG_ON_ZERO(cond) (sizeof(struct { int:-!!(cond); }))
#define CAA_BUILD_BUG_ON(cond) ((void)CAA_BUILD_BUG_ON_ZERO(cond))

/*
 * __rcu is an annotation that documents RCU pointer accesses that need
 * to be protected by a read-side critical section. Eventually, a static
 * checker will be able to use this annotation to detect incorrect RCU
 * usage.
 */
#define __rcu

#ifdef __cplusplus
#define URCU_FORCE_CAST(_type, arg)	(reinterpret_cast<typename std::remove_cv<_type>::type>(arg))
#else
#define URCU_FORCE_CAST(type, arg)	((type) (arg))
#endif

#define caa_is_signed_type(type)	((type) -1 < (type) 0)

/*
 * Cast to unsigned long, sign-extending if @v is signed.
 * Note: casting to a larger type or to same type size keeps the sign of
 * the expression being cast (see C99 6.3.1.3).
 */
#define caa_cast_long_keep_sign(v)	((unsigned long) (v))

#if defined (__GNUC__) \
	&& ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)	\
		|| __GNUC__ >= 5)
#define CDS_DEPRECATED(msg)	\
	__attribute__((deprecated(msg)))
#else
#define CDS_DEPRECATED(msg)	\
	__attribute__((deprecated))
#endif

#define CAA_ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

/*
 * URCU_GCC_VERSION is used to blacklist specific GCC versions with known
 * bugs, clang also defines these macros to an equivalent GCC version it
 * claims to support, so exclude it.
 */
#if defined(__GNUC__) && !defined(__clang__)
# define URCU_GCC_VERSION	(__GNUC__ * 10000 \
				+ __GNUC_MINOR__ * 100 \
				+ __GNUC_PATCHLEVEL__)
#endif

#ifdef __cplusplus
#define caa_unqual_scalar_typeof(x)					\
	std::remove_cv<std::remove_reference<decltype(x)>::type>::type
#else
#define caa_scalar_type_to_expr(type)					\
	unsigned type: (unsigned type)0,				\
	signed type: (signed type)0

/*
 * Use C11 _Generic to express unqualified type from expression. This removes
 * volatile qualifier from expression type.
 */
#define caa_unqual_scalar_typeof(x)					\
	__typeof__(							\
		_Generic((x),						\
			char: (char)0,					\
			caa_scalar_type_to_expr(char),			\
			caa_scalar_type_to_expr(short),		\
			caa_scalar_type_to_expr(int),			\
			caa_scalar_type_to_expr(long),			\
			caa_scalar_type_to_expr(long long),		\
			default: (x)					\
		)							\
	)
#endif

/*
 * Allow user to manually define CMM_SANITIZE_THREAD if their toolchain is not
 * supported by this check.
 */
#ifndef CMM_SANITIZE_THREAD
# if defined(__GNUC__) && defined(__SANITIZE_THREAD__)
#  define CMM_SANITIZE_THREAD
# elif defined(__clang__) && defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#   define CMM_SANITIZE_THREAD
#  endif
# endif
#endif	/* !CMM_SANITIZE_THREAD */

/*
 * Helper to add the volatile qualifier to a pointer.
 *
 * This is used to enforce volatile accesses even when using atomic
 * builtins. Indeed, C11 atomic operations all work on volatile memory
 * accesses, but this is not documented for compiler atomic builtins.
 *
 * This could prevent certain classes of optimization from the compiler such
 * as load/store fusing.
 */
#if defined __cplusplus
template <typename T>
volatile T cmm_cast_volatile(T t)
{
    return static_cast<volatile T>(t);
}
#else
#  define cmm_cast_volatile(ptr)			\
	__extension__					\
	({						\
		(volatile __typeof__(ptr))(ptr);	\
	})
#endif

/*
 * Compile time assertion.
 * - predicate: boolean expression to evaluate,
 * - msg: string to print to the user on failure when `static_assert()` is
 *   supported,
 * - c_identifier_msg: message to be included in the typedef to emulate a
 *   static assertion. This parameter must be a valid C identifier as it will
 *   be used as a typedef name.
 */
#ifdef __cplusplus
#define urcu_static_assert(predicate, msg, c_identifier_msg)  \
	static_assert(predicate, msg)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define urcu_static_assert(predicate, msg, c_identifier_msg)  \
	_Static_assert(predicate, msg)
#else
/*
 * Evaluates the predicate and emit a compilation error on failure.
 *
 * If the predicate evaluates to true, this macro emits a function
 * prototype with an argument type which is an array of size 0.
 *
 * If the predicate evaluates to false, this macro emits a function
 * prototype with an argument type which is an array of negative size
 * which is invalid in C and forces a compiler error. The
 * c_identifier_msg parameter is used as the argument identifier so it
 * is printed to the user when the error is reported.
 */
#define urcu_static_assert(predicate, msg, c_identifier_msg)  \
	void urcu_static_assert_proto(char c_identifier_msg[2*!!(predicate)-1])
#endif

#endif /* _URCU_COMPILER_H */
