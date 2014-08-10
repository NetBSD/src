/*	$NetBSD: bitops.h,v 1.2.2.1 2014/08/10 06:55:39 tls Exp $	*/

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

static inline unsigned long
__ffs64(uint64_t x)
{
	return ffs64(x);
}

static inline unsigned int
hweight16(uint16_t n)
{
	return popcount32(n);
}

static inline unsigned int
hweight32(uint16_t n)
{
	return popcount32(n);
}

/*
 * XXX Don't define BITS_PER_LONG as sizeof(unsigned long)*CHAR_BIT
 * because that won't work in preprocessor conditionals, where it often
 * turns up.
 */

#define	BITS_TO_LONGS(n)						\
	roundup2((n), (sizeof(unsigned long) * CHAR_BIT))

#define	BIT(n)	((uintmax_t)1 << (n))

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
find_first_zero_bit(const unsigned long *ptr, unsigned long nbits)
{
	const size_t bpl = (CHAR_BIT * sizeof(*ptr));
	const unsigned long *p;
	unsigned long result = 0;

	for (p = ptr; bpl < nbits; nbits -= bpl, p++, result += bpl) {
		if (~*p)
			break;
	}

	result += ffs(~*p | (~0UL << MIN(nbits, bpl)));
	return result;
}

#endif  /* _LINUX_BITOPS_H_ */
