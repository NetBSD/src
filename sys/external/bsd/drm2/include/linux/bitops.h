/*	$NetBSD: bitops.h,v 1.1.2.3 2013/09/08 15:37:34 riastradh Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/cdefs.h>

#include <machine/limits.h>

#include <lib/libkern/libkern.h>

static inline unsigned int
hweight16(uint16_t n)
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

#endif  /* _LINUX_BITOPS_H_ */
