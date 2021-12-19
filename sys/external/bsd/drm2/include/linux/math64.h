/*	$NetBSD: math64.h,v 1.12 2021/12/19 11:48:34 riastradh Exp $	*/

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

#ifndef _LINUX_MATH64_H_
#define _LINUX_MATH64_H_

#include <sys/types.h>

#include <asm/div64.h>

#include <linux/types.h>

static inline int64_t
div64_u64(int64_t dividend, uint64_t divisor)
{
	return dividend / divisor;
}

static inline int64_t
div_u64(int64_t dividend, uint32_t divisor)
{
	return dividend / divisor;
}

static inline int64_t
div64_s64(int64_t dividend, int64_t divisor)
{
	return dividend / divisor;
}

static inline uint64_t
DIV64_U64_ROUND_UP(uint64_t dividend, uint64_t divisor)
{
	return (dividend + (divisor - 1))/divisor;
}

static inline int64_t
div_s64(int64_t dividend, int32_t divisor)
{
	return dividend / divisor;
}

static inline uint32_t
div_u64_rem(uint64_t dividend, uint32_t divisor, uint32_t *rem)
{
	*rem = dividend % divisor;
	return dividend / divisor;
}

static inline uint64_t
div64_u64_rem(uint64_t dividend, uint64_t divisor, uint64_t *rem)
{
	*rem = dividend % divisor;
	return dividend / divisor;
}

static inline uint64_t
mul_u32_u32(uint32_t a, uint32_t b)
{
	return (uint64_t)a * (uint64_t)b;
}

static inline uint64_t
mul_u64_u32_div(uint64_t a, uint32_t b, uint32_t div)
{
	/* XXX implement to account for overflow */
	return (a * b) / div;
}

/* return floor((a*b) / 2^c) */
static inline uint64_t
mul_u64_u32_shr(uint64_t a, uint32_t b, unsigned c)
{
	/* 2^32 a_hi + a_lo := a */
	uint64_t a_hi = a >> 32;
	uint64_t a_lo = a & 0xffffffffU;

	if (c >= 32) {
		/* (a*b) / 2^c = (a_hi b + a_lo b / 2^32) / 2^{c - 32} */
		return ((a_hi * b) + ((a_lo * b) >> 32)) >> (c - 32);
	} else {
		/* (a*b) / 2^c = 2^{32 - c} a_hi b + a_lo b / 2^c */
		return ((a_hi * b) << (32 - c)) + ((a_lo * b) >> c);
	}
}

#endif  /* _LINUX_MATH64_H_ */
