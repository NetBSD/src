/*	$NetBSD: random.h,v 1.4 2021/12/19 11:49:12 riastradh Exp $	*/

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

#ifndef	_LINUX_RANDOM_H_
#define	_LINUX_RANDOM_H_

#include <sys/cprng.h>

static inline int
get_random_int(void)
{
	int v;

	cprng_strong(kern_cprng, &v, sizeof v, 0);

	return v;
}

static inline long
get_random_long(void)
{
	long v;

	cprng_strong(kern_cprng, &v, sizeof v, 0);

	return v;
}

static inline uint32_t
get_random_u32(void)
{
	uint32_t v;

	cprng_strong(kern_cprng, &v, sizeof v, 0);

	return v;
}

static inline uint32_t
prandom_u32_max(uint32_t bound)
{
	uint32_t v, min;

	min = (-bound) % bound;
	do {
		v = get_random_u32();
	} while (v < min);

	return v % bound;
}

#endif	/* _LINUX_RANDOM_H_ */
