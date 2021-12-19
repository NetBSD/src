/*	$NetBSD: kernel.h,v 1.44 2021/12/19 11:35:36 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_KERNEL_H_
#define _LINUX_KERNEL_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#include <asm/byteorder.h>
#include <asm/div64.h>

#include <linux/bitops.h>
#include <linux/compiler.h>
#include <linux/log2.h>
#include <linux/printk.h>
#include <linux/slab.h>

#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define	S16_MAX	INT16_MAX
#define	S32_MAX	INT32_MAX
#define	S64_MAX	INT64_MAX

#define	oops_in_progress	(panicstr != NULL)

#define	IS_BUILTIN(option)	(1) /* Probably... */
#define	IS_ENABLED(option)	(option)
#define	IS_REACHABLE(option)	(option)

#define	might_sleep	ASSERT_SLEEPABLE
#define	might_sleep_if(C) do						      \
{									      \
	if (C)								      \
		might_sleep();						      \
} while (0)

#define	DEFINE_STATIC_KEY_FALSE(N)	bool N __unused = false

/*
 * XXX Linux kludge to work around GCC uninitialized variable warning.
 * Linux does `x = x', which is bollocks.
 */
#define	uninitialized_var(x)	x = 0

#define	typecheck(T, X)	({(1 + 0*sizeof((T *)0 - &(X)));})

/* XXX These will multiply evaluate their arguments.  */
#define	min(X, Y)	MIN(X, Y)
#define	max(X, Y)	MAX(X, Y)

#define	max_t(T, X, Y)	MAX((T)(X), (T)(Y))
#define	min_t(T, X, Y)	MIN((T)(X), (T)(Y))

#define	clamp_t(T, X, MIN, MAX)	min_t(T, max_t(T, X, MIN), MAX)
#define	clamp(X, MN, MX)	MIN(MAX(X, MN), MX)
#define	clamp_val(X, MIN, MAX)	clamp_t(typeof(X), X, MIN, MAX)

#define	min3(X, Y, Z)	MIN(X, MIN(Y, Z))
#define	max3(X, Y, Z)	MAX(X, MAX(Y, Z))

/*
 * Rounding to nearest.
 */
#define	DIV_ROUND_CLOSEST(N, D)						\
	((0 < (N)) ? (((N) + ((D) / 2)) / (D))				\
	    : (((N) - ((D) / 2)) / (D)))

#define	DIV_ROUND_CLOSEST_ULL(N, D)	(((N) + (D)/2)/(D))

/*
 * Rounding to what may or may not be powers of two.
 */
#define	DIV_ROUND_UP(X, N)	(((X) + (N) - 1) / (N))
#define	DIV_ROUND_UP_ULL(X, N)	DIV_ROUND_UP((unsigned long long)(X), (N))

#define	DIV_ROUND_DOWN_ULL(X,N)	((unsigned long long)(X) / (N))

/*
 * Rounding to powers of two -- carefully avoiding multiple evaluation
 * of arguments and pitfalls with C integer arithmetic rules.
 */
#define	round_up(X, N)		((((X) - 1) | ((N) - 1)) + 1)
#define	round_down(X, N)	((X) & ~(uintmax_t)((N) - 1))

#define	IS_ALIGNED(X, N)	(((X) & ((N) - 1)) == 0)

#define	ALIGN_DOWN(X, N)	round_down(X, N)

/*
 * These select 32-bit halves of what may be 32- or 64-bit quantities,
 * for which straight 32-bit shifts may be undefined behaviour (and do
 * the wrong thing on most machines: return the input unshifted by
 * ignoring the upper bits of the shift count).
 */
#define	upper_32_bits(X)	((uint32_t) (((X) >> 16) >> 16))
#define	lower_32_bits(X)	((uint32_t) ((X) & 0xffffffffUL))

#define	ARRAY_SIZE(ARRAY)	__arraycount(ARRAY)

#define	__is_constexpr(x)	__builtin_constant_p(x)

#define	swap(X, Y)	do						\
{									\
	/* XXX Kludge for type-safety.  */				\
	if (&(X) != &(Y)) {						\
		CTASSERT(sizeof(X) == sizeof(Y));			\
		/* XXX Can't do this much better without typeof.  */	\
		char __swap_tmp[sizeof(X)];				\
		(void)memcpy(__swap_tmp, &(X), sizeof(X));		\
		(void)memcpy(&(X), &(Y), sizeof(X));			\
		(void)memcpy(&(Y), __swap_tmp, sizeof(X));		\
	}								\
} while (0)

static __inline int64_t
abs64(int64_t x)
{
	return (x < 0? (-x) : x);
}

static __inline uintmax_t
mult_frac(uintmax_t x, uintmax_t multiplier, uintmax_t divisor)
{
	uintmax_t q = (x / divisor);
	uintmax_t r = (x % divisor);

	return ((q * multiplier) + ((r * multiplier) / divisor));
}

static int panic_timeout __unused = 0;

static __inline int __printflike(3, 0)
vscnprintf(char *buf, size_t size, const char *fmt, va_list va)
{
	int ret;

	ret = vsnprintf(buf, size, fmt, va);
	if (__predict_false(ret < 0))
		return ret;
	if (__predict_false(size == 0))
		return 0;
	if (__predict_false(size <= (size_t)ret))
		return (size - 1);

	return ret;
}

static __inline int __printflike(3, 4)
scnprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list va;
	int ret;

	va_start(va, fmt);
	ret = vscnprintf(buf, size, fmt, va);
	va_end(va);

	return ret;
}

static __inline int
kstrtol(const char *s, unsigned base, long *vp)
{
	long long v;

	v = strtoll(s, NULL, base);
	if (v < LONG_MIN || LONG_MAX < v)
		return -ERANGE;
	*vp = v;
	return 0;
}

static inline long
simple_strtol(const char *s, char **endp, unsigned base)
{
	long v;

	*endp = NULL;		/* paranoia */
	v = strtoll(s, endp, base);
	if (v < LONG_MIN || LONG_MAX < v)
		return 0;
	return v;
}

static __inline char * __printflike(2, 0)
kvasprintf(gfp_t gfp, const char *fmt, va_list va)
{
	va_list tva;
	char *str;
	int len, len1 __diagused;

	va_copy(tva, va);
	len = vsnprintf(NULL, 0, fmt, tva);
	va_end(tva);
	str = kmalloc(len + 1, gfp);
	if (str == NULL)
		return NULL;
	len1 = vsnprintf(str, len + 1, fmt, va);
	KASSERT(len1 == len);

	return str;
}

static __inline char * __printflike(2, 3)
kasprintf(gfp_t gfp, const char *fmt, ...)
{
	va_list va;
	char *str;

	va_start(va, fmt);
	str = kvasprintf(gfp, fmt, va);
	va_end(va);

	return str;
}

static inline void __user *
u64_to_user_ptr(uint64_t addr)
{

	return (void __user *)(uintptr_t)addr;
}

#define	TAINT_MACHINE_CHECK	0
#define	TAINT_WARN		1

#define	LOCKDEP_STILL_OK	0

static inline void
add_taint(unsigned taint, int lockdep)
{
}

#define	DFEINE_STATIC_KEY_FALSE(FLAG)					      \
	bool FLAG = false

static inline bool
static_branch_likely(const bool *flagp)
{
	return __predict_true(*flagp);
}

#endif  /* _LINUX_KERNEL_H_ */
