/*	$NetBSD: bitmap.h,v 1.6 2018/08/27 14:50:52 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_BITMAP_H_
#define _LINUX_BITMAP_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>

/*
 * bitmap_zero(bitmap, nbits)
 *
 *	Zero a bitmap that was allocated to have nbits bits.  Yes, this
 *	zeros bits past nbits.
 */
static inline void
bitmap_zero(unsigned long *bitmap, size_t nbits)
{
	const size_t bpl = NBBY * sizeof(*bitmap);

	memset(bitmap, 0, howmany(nbits, bpl) * sizeof(*bitmap));
}

/*
 * bitmap_empty(bitmap, nbits)
 *
 *	Return true if all bits at 0, 1, 2, ..., nbits-2, nbits-1 are
 *	0, or false if any of them is 1.
 */
static inline bool
bitmap_empty(const unsigned long *bitmap, size_t nbits)
{
	const size_t bpl = NBBY * sizeof(*bitmap);

	for (; nbits >= bpl; nbits -= bpl) {
		if (*bitmap++)
			return false;
	}

	if (nbits) {
		if (*bitmap & ~(~0UL << nbits))
			return false;
	}

	return true;
}

/*
 * bitmap_weight(bitmap, nbits)
 *
 *	Compute the number of 1 bits at 0, 1, 2, ..., nbits-2, nbits-1.
 */
static inline int
bitmap_weight(const unsigned long *bitmap, size_t nbits)
{
	const size_t bpl = NBBY * sizeof(*bitmap);
	int weight = 0;

	for (; nbits >= bpl; nbits -= bpl)
		weight += popcountl(*bitmap++);
	if (nbits)
		weight += popcountl(*bitmap & ~(~0UL << nbits));

	return weight;
}

/*
 * bitmap_set(bitmap, startbit, nbits)
 *
 *	Set bits at startbit, startbit+1, ..., startbit+nbits-2,
 *	startbit+nbits-1 to 1.
 */
static inline void
bitmap_set(unsigned long *bitmap, size_t startbit, size_t nbits)
{
	const size_t bpl = NBBY * sizeof(*bitmap);
	unsigned long *p = bitmap + startbit/bpl;
	unsigned long mask;
	unsigned sz;

	for (sz = bpl - (startbit%bpl), mask = ~0UL << (startbit%bpl);
	     nbits >= sz;
	     nbits -= sz, sz = bpl, mask = ~0UL)
		*p++ |= mask;

	if (nbits)
		*p |= mask & ~(~0UL << (nbits + bpl - sz));
}

/*
 * bitmap_clear(bitmap, startbit, nbits)
 *
 *	Clear bits at startbit, startbit+1, ..., startbit+nbits-2,
 *	startbit+nbits-1, replacing them by 0.
 */
static inline void
bitmap_clear(unsigned long *bitmap, size_t startbit, size_t nbits)
{
	const size_t bpl = NBBY * sizeof(*bitmap);
	unsigned long *p = bitmap + startbit/bpl;
	unsigned long mask;
	unsigned sz;

	for (sz = bpl - (startbit%bpl), mask = ~(~0UL << (startbit%bpl));
	     nbits >= sz;
	     nbits -= sz, sz = bpl, mask = 0UL)
		*p++ &= mask;

	if (nbits)
		*p &= mask | (~0UL << (nbits + bpl - sz));
}

/*
 * bitmap_and(dst, src1, src2, nbits)
 *
 *	Set dst to be the bitwise AND of src1 and src2, all bitmaps
 *	allocated to have nbits bits.  Yes, this modifies bits past
 *	nbits.  Any pair of {dst, src1, src2} may be aliases.
 */
static inline void
bitmap_and(unsigned long *dst, const unsigned long *src1,
    const unsigned long *src2, size_t nbits)
{
	const size_t bpl = NBBY * sizeof(unsigned long);
	size_t n = howmany(nbits, bpl);

	while (n --> 0)
		*dst++ = *src1++ & *src2++;
}

/*
 * bitmap_or(dst, src1, src2, nbits)
 *
 *	Set dst to be the bitwise inclusive-OR of src1 and src2, all
 *	bitmaps allocated to have nbits bits.  Yes, this modifies bits
 *	past nbits.  Any pair of {dst, src1, src2} may be aliases.
 */
static inline void
bitmap_or(unsigned long *dst, const unsigned long *src1,
    const unsigned long *src2, size_t nbits)
{
	const size_t bpl = NBBY * sizeof(unsigned long);
	size_t n = howmany(nbits, bpl);

	while (n --> 0)
		*dst++ = *src1++ | *src2++;
}

#endif  /* _LINUX_BITMAP_H_ */
