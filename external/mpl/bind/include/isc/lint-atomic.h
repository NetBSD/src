/*	 $NetBSD: lint-atomic.h,v 1.1 2025/01/26 16:31:40 christos Exp $	*/

/*
 * Definitions for clang's atomic builtins
 */

#ifdef __clang__

#define __c11_atomic_init(a, b)	(*(a) = (b))

#define __c11_atomic_load(a, m) (*(a))
#define __c11_atomic_store(a, b, m) (*(a) = (b))

#define __c11_atomic_fetch_add(a, b, m) (*(a) += (b))
#define __c11_atomic_fetch_sub(a, b, m) (*(a) -= (b))
#define __c11_atomic_fetch_or(a, b, m) (*(a) |= (b))
#define __c11_atomic_fetch_and(a, b, m) (*(a) &= (b))

#define __c11_atomic_exchange(a, b, m) (*(a) = (b))

#define __c11_atomic_compare_exchange_strong(a, b, e, d, m) \
    ((*(a) == (e)) ? (*(b) = (d)) : 0)
#define __c11_atomic_compare_exchange_weak(a, b, e, d, m) \
    ((*(a) == (e)) ? (*(b) = (d)) : 0)

#endif
