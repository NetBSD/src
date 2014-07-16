/*	$NetBSD: atomic.h,v 1.6 2014/07/16 20:59:58 riastradh Exp $	*/

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

#ifndef _LINUX_ATOMIC_H_
#define _LINUX_ATOMIC_H_

#include <sys/atomic.h>

#include <machine/limits.h>

struct atomic {
	union {
		volatile int au_int;
		volatile unsigned int au_uint;
	} a_u;
};

#define	ATOMIC_INIT(i)	{ .a_u = { .au_int = (i) } }

typedef struct atomic atomic_t;

static inline int
atomic_read(atomic_t *atomic)
{
	return atomic->a_u.au_int;
}

static inline void
atomic_set(atomic_t *atomic, int value)
{
	atomic->a_u.au_int = value;
}

static inline void
atomic_add(int addend, atomic_t *atomic)
{
	atomic_add_int(&atomic->a_u.au_uint, addend);
}

static inline void
atomic_sub(int subtrahend, atomic_t *atomic)
{
	atomic_add_int(&atomic->a_u.au_uint, -subtrahend);
}

static inline int
atomic_add_return(int addend, atomic_t *atomic)
{
	return (int)atomic_add_int_nv(&atomic->a_u.au_uint, addend);
}

static inline void
atomic_inc(atomic_t *atomic)
{
	atomic_inc_uint(&atomic->a_u.au_uint);
}

static inline void
atomic_dec(atomic_t *atomic)
{
	atomic_dec_uint(&atomic->a_u.au_uint);
}

static inline int
atomic_inc_return(atomic_t *atomic)
{
	return (int)atomic_inc_uint_nv(&atomic->a_u.au_uint);
}

static inline int
atomic_dec_return(atomic_t *atomic)
{
	return (int)atomic_dec_uint_nv(&atomic->a_u.au_uint);
}

static inline int
atomic_dec_and_test(atomic_t *atomic)
{
	return (0 == (int)atomic_dec_uint_nv(&atomic->a_u.au_uint));
}

static inline void
atomic_set_mask(unsigned long mask, atomic_t *atomic)
{
	atomic_or_uint(&atomic->a_u.au_uint, mask);
}

static inline void
atomic_clear_mask(unsigned long mask, atomic_t *atomic)
{
	atomic_and_uint(&atomic->a_u.au_uint, ~mask);
}

static inline int
atomic_add_unless(atomic_t *atomic, int addend, int zero)
{
	int value;

	do {
		value = atomic->a_u.au_int;
		if (value == zero)
			return 0;
	} while (atomic_cas_uint(&atomic->a_u.au_uint, value, (value + addend))
	    != value);

	return 1;
}

static inline int
atomic_inc_not_zero(atomic_t *atomic)
{
	return atomic_add_unless(atomic, 1, 0);
}

static inline int
atomic_xchg(atomic_t *atomic, int new)
{
	return (int)atomic_swap_uint(&atomic->a_u.au_uint, (unsigned)new);
}

static inline int
atomic_cmpxchg(atomic_t *atomic, int old, int new)
{
	return (int)atomic_cas_uint(&atomic->a_u.au_uint, (unsigned)old,
	    (unsigned)new);
}

struct atomic64 {
	volatile uint64_t	a_v;
};

typedef struct atomic64 atomic64_t;

static inline uint64_t
atomic64_read(const struct atomic64 *a)
{
	return a->a_v;
}

static inline void
atomic64_set(struct atomic64 *a, uint64_t v)
{
	a->a_v = v;
}

static inline void
atomic64_add(long long d, struct atomic64 *a)
{
	atomic_add_64(&a->a_v, d);
}

static inline void
atomic64_sub(long long d, struct atomic64 *a)
{
	atomic_add_64(&a->a_v, -d);
}

static inline uint64_t
atomic64_xchg(struct atomic64 *a, uint64_t v)
{
	return atomic_swap_64(&a->a_v, v);
}

static inline void
set_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);

	atomic_or_ulong(&ptr[bit / units], (1UL << (bit % units)));
}

static inline void
clear_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);

	atomic_and_ulong(&ptr[bit / units], ~(1UL << (bit % units)));
}

static inline void
change_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	do v = *p; while (atomic_cas_ulong(p, v, (v ^ mask)) != v);
}

static inline unsigned long
test_and_set_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	do v = *p; while (atomic_cas_ulong(p, v, (v | mask)) != v);

	return (v & mask);
}

static inline unsigned long
test_and_clear_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	do v = *p; while (atomic_cas_ulong(p, v, (v & ~mask)) != v);

	return (v & mask);
}

static inline unsigned long
test_and_change_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	do v = *p; while (atomic_cas_ulong(p, v, (v ^ mask)) != v);

	return (v & mask);
}

#if defined(MULTIPROCESSOR) && !defined(__HAVE_ATOMIC_AS_MEMBAR)
/*
 * XXX These memory barriers are doubtless overkill, but I am having
 * trouble understanding the intent and use of the Linux atomic membar
 * API.  I think that for reference counting purposes, the sequences
 * should be insn/inc/enter and exit/dec/insn, but the use of the
 * before/after memory barriers is not consistent throughout Linux.
 */
#  define	smp_mb__before_atomic_inc()	membar_sync()
#  define	smp_mb__after_atomic_inc()	membar_sync()
#  define	smp_mb__before_atomic_dec()	membar_sync()
#  define	smp_mb__after_atomic_dec()	membar_sync()
#else
#  define	smp_mb__before_atomic_inc()	__insn_barrier()
#  define	smp_mb__after_atomic_inc()	__insn_barrier()
#  define	smp_mb__before_atomic_dec()	__insn_barrier()
#  define	smp_mb__after_atomic_dec()	__insn_barrier()
#endif

#endif  /* _LINUX_ATOMIC_H_ */
