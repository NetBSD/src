/*	$NetBSD: bitops.h,v 1.16.4.1 2024/10/04 11:40:50 martin Exp $	*/

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

#ifndef _LINUX_BITOPS_H_
#define _LINUX_BITOPS_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bitops.h>

#include <machine/limits.h>

#include <lib/libkern/libkern.h>

#include <asm/barrier.h>

#include <linux/bits.h>

/*
 * Linux __ffs/__ffs64 is zero-based; zero input is undefined.  Our
 * ffs/ffs64 is one-based; zero input yields zero.
 */
static inline unsigned long
__ffs(unsigned long x)
{

	KASSERT(x != 0);
	return ffs64(x) - 1;
}

static inline unsigned long
__ffs64(uint64_t x)
{

	KASSERT(x != 0);
	return ffs64(x) - 1;
}

/*
 * Linux fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32, so it matches
 * our fls semantics.
 */
static inline int
fls(int x)
{
	return fls32(x);
}

static inline unsigned int
hweight8(uint8_t w)
{
	return popcount(w & 0xff);
}

static inline unsigned int
hweight16(uint16_t n)
{
	return popcount32(n);
}

static inline unsigned int
hweight32(uint32_t n)
{
	return popcount32(n);
}

static inline unsigned int
hweight64(uint64_t n)
{
	return popcount64(n);
}

static inline int64_t
sign_extend64(uint64_t x, unsigned n)
{
	return (int64_t)(x << (63 - n)) >> (63 - n);
}

#define	BITS_TO_LONGS(n)						\
	howmany((n), (sizeof(unsigned long) * CHAR_BIT))

#define	BITS_PER_TYPE(type)	(sizeof(type) * NBBY)
#define	BITS_PER_BYTE		NBBY
#define	BITS_PER_LONG		(__SIZEOF_LONG__ * CHAR_BIT)

#define	BIT(n)		((unsigned long)__BIT(n))
#define	BIT_ULL(n)	((unsigned long long)__BIT(n))
#define	GENMASK(h,l)	((unsigned long)__BITS(h,l))
#define	GENMASK_ULL(h,l)((unsigned long long)__BITS(h,l))

static inline int
test_bit(unsigned int n, const volatile unsigned long *p)
{
	const unsigned units = (sizeof(unsigned long) * CHAR_BIT);

	return ((p[n / units] & (1UL << (n % units))) != 0);
}

static inline void
__set_bit(unsigned int n, volatile unsigned long *p)
{
	const unsigned units = (sizeof(unsigned long) * CHAR_BIT);

	p[n / units] |= (1UL << (n % units));
}

static inline void
__clear_bit(unsigned int n, volatile unsigned long *p)
{
	const unsigned units = (sizeof(unsigned long) * CHAR_BIT);

	p[n / units] &= ~(1UL << (n % units));
}

static inline void
__change_bit(unsigned int n, volatile unsigned long *p)
{
	const unsigned units = (sizeof(unsigned long) * CHAR_BIT);

	p[n / units] ^= (1UL << (n % units));
}

static inline unsigned long
__test_and_set_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	v = *p;
	*p |= mask;

	return ((v & mask) != 0);
}

static inline unsigned long
__test_and_clear_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	v = *p;
	*p &= ~mask;

	return ((v & mask) != 0);
}

static inline unsigned long
__test_and_change_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	v = *p;
	*p ^= mask;

	return ((v & mask) != 0);
}

static inline unsigned long
__find_next_bit(const unsigned long *ptr, unsigned long nbits,
    unsigned long startbit, unsigned long toggle)
{
	const size_t bpl = (CHAR_BIT * sizeof(*ptr));
	const unsigned long *p = ptr + startbit/bpl;
	size_t n = howmany(nbits, bpl);
	unsigned long result;
	uint64_t word;

	/*
	 * We use ffs64 because NetBSD doesn't have a handy ffsl that
	 * works on unsigned long.  This is a waste on 32-bit systems
	 * but I'd rather not maintain multiple copies of this -- the
	 * first version had enough bugs already.
	 */

	/* Do we need to examine a partial starting word?  */
	if (startbit % bpl) {
		/* Toggle the bits and convert to 64 bits for ffs64.  */
		word = *p ^ toggle;

		/* Clear the low startbit%bpl bits.  */
		word &= (~0UL << (startbit % bpl));

		/* Are any of these bits set now?  */
		if (word)
			goto out;

		/* Move past it.  */
		p++;
		n--;
	}

	/* Find the first word with any bits set.  */
	for (; n --> 0; p++) {
		/* Toggle the bits and convert to 64 bits for ffs64. */
		word = *p ^ toggle;

		/* Are any of these bits set now?  */
		if (word)
			goto out;
	}

	/* Nada.  */
	return nbits;

out:
	/* Count how many words we've skipped.  */
	result = bpl*(p - ptr);

	/* Find the first set bit in this word, zero-based.  */
	result += ffs64(word) - 1;

	/* We may have overshot, so clamp down to at most nbits.  */
	return MIN(result, nbits);
}

static inline unsigned long
find_next_bit(const unsigned long *ptr, unsigned long nbits,
    unsigned long startbit)
{
	return __find_next_bit(ptr, nbits, startbit, 0);
}

static inline unsigned long
find_first_bit(const unsigned long *ptr, unsigned long nbits)
{
	return find_next_bit(ptr, nbits, 0);
}

static inline unsigned long
find_next_zero_bit(const unsigned long *ptr, unsigned long nbits,
    unsigned long startbit)
{
	return __find_next_bit(ptr, nbits, startbit, ~0UL);
}

static inline unsigned long
find_first_zero_bit(const unsigned long *ptr, unsigned long nbits)
{
	return find_next_zero_bit(ptr, nbits, 0);
}

#define	for_each_set_bit(BIT, PTR, NBITS)				      \
	for ((BIT) = find_first_bit((PTR), (NBITS));			      \
	     (BIT) < (NBITS);						      \
	     (BIT) = find_next_bit((PTR), (NBITS), (BIT) + 1))

#define	for_each_clear_bit(BIT, PTR, NBITS)				      \
	for ((BIT) = find_first_zero_bit((PTR), (NBITS));		      \
	     (BIT) < (NBITS);						      \
	     (BIT) = find_next_zero_bit((PTR), (NBITS), (BIT) + 1))

static inline void
set_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);

	/* no memory barrier */
	atomic_or_ulong(&ptr[bit / units], (1UL << (bit % units)));
}

static inline void
clear_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);

	/* no memory barrier */
	atomic_and_ulong(&ptr[bit / units], ~(1UL << (bit % units)));
}

static inline void
clear_bit_unlock(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);

	/* store-release */
	smp_mb__before_atomic();
	atomic_and_ulong(&ptr[bit / units], ~(1UL << (bit % units)));
}

static inline void
change_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	/* no memory barrier */
	do v = *p; while (atomic_cas_ulong(p, v, (v ^ mask)) != v);
}

static inline int
test_and_set_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	smp_mb__before_atomic();
	do v = *p; while (atomic_cas_ulong(p, v, (v | mask)) != v);
	smp_mb__after_atomic();

	return ((v & mask) != 0);
}

static inline int
test_and_clear_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	smp_mb__before_atomic();
	do v = *p; while (atomic_cas_ulong(p, v, (v & ~mask)) != v);
	smp_mb__after_atomic();

	return ((v & mask) != 0);
}

static inline int
test_and_change_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	smp_mb__before_atomic();
	do v = *p; while (atomic_cas_ulong(p, v, (v ^ mask)) != v);
	smp_mb__after_atomic();

	return ((v & mask) != 0);
}

#endif  /* _LINUX_BITOPS_H_ */
