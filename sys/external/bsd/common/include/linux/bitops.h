/*	$NetBSD: bitops.h,v 1.5 2018/08/27 07:08:37 riastradh Exp $	*/

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

/*
 * XXX Don't define BITS_PER_LONG as sizeof(unsigned long)*CHAR_BIT
 * because that won't work in preprocessor conditionals, where it often
 * turns up.
 */

#define	BITS_TO_LONGS(n)						\
	roundup2((n), (sizeof(unsigned long) * CHAR_BIT))

#define	BIT(n)	((uintmax_t)1 << (n))
#define	GENMASK(h,l)	__BITS(h,l)

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
find_next_zero_bit(const unsigned long *ptr, unsigned long nbits,
    unsigned long startbit)
{
	const size_t bpl = (CHAR_BIT * sizeof(*ptr));
	const unsigned long *p = ptr + startbit/bpl;
	unsigned long result = rounddown(startbit, bpl);
	uint64_t word;

	/*
	 * We use ffs64 because NetBSD doesn't have a handy ffsl that
	 * works on unsigned long.  This is a waste on 32-bit systems
	 * but I'd rather not maintain multiple copies of this -- the
	 * first version had enough bugs already.
	 */

	/* Do we need to examine a partial starting word?  */
	if (startbit % bpl) {
		/* Are any of the first startbit%bpl bits zero?  */
		if (~(*p | (~0UL << (startbit % bpl)))) {
			/* Invert the bits and convert to 64 bits.  */
			word = ~(uint64_t)*p;

			/* Clear the low startbit%bpl bits.  */
			word &= ~(~0UL << (startbit % bpl));

			/* Find the first set bit in this word. */
			result += ffs64(word);

			/* Clamp down to at most nbits.  */
			return MIN(result, nbits);
		}
	}

	/* Find the first word with zeros in it.  */
	for (; bpl < nbits; p++, result += bpl) {
		if (~*p)
			break;
	}

	/* Invert the bits and convert to 64 bits for ffs64.  */
	word = ~(uint64_t)*p;

	/* Find the first set bit in this word.  */
	result += ffs64(word);

	/* Clamp down to at most nbits.  */
	return MIN(result, nbits);
}

static inline unsigned long
find_first_zero_bit(const unsigned long *ptr, unsigned long nbits)
{
	return find_next_zero_bit(ptr, nbits, 0);
}

static inline unsigned
hweight8(unsigned w)
{

	return popcount(w & 0xff);
}

#endif  /* _LINUX_BITOPS_H_ */
