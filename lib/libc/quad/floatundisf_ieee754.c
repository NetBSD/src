/*	$NetBSD: floatundisf_ieee754.c,v 1.2.4.2 2014/05/22 11:36:53 yamt Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: floatundisf_ieee754.c,v 1.2.4.2 2014/05/22 11:36:53 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(SOFTFLOAT) || defined(__ARM_EABI__)
#include "softfloat/softfloat-for-gcc.h"
#endif

#include <machine/ieee.h>
#include "quad.h"

float __floatundisf(u_quad_t);

/*
 * Convert unsigned quad to float.
 */
float
__floatundisf(u_quad_t x)
{
	union ieee_single_u ux = { .sngu_f = 0.0 };

	if (x == 0)
		return 0.0;
	if (x == 1)
		return 1.0;

#if defined(_LP64) || defined(__mips_n32)
	u_int l = __builtin_clzll(x);
	x <<= (l + 1);	/* clear implicit bit */

	ux.sngu_frac = x >> (64 - SNG_FRACBITS);
#else
	union uu u = { .uq = x };
	uint32_t frac;
	u_int l;
	if (u.ul[H] == 0) {
		l = __builtin_clz(u.ul[L]);
		frac = u.ul[L] << (l + 1);	/* clear implicit bit */
		l += 32;
	} else {
		l = __builtin_clz(u.ul[H]);
		frac = u.ul[H] << (l + 1);	/* clear implicit bit */
		frac |= u.ul[L] >> (32 - (l + 1));
	}

	ux.sngu_frac = frac >> (32 - SNG_FRACBITS);
#endif
	ux.sngu_exp = SNG_EXP_BIAS + 63 - l;

	return ux.sngu_f;
}
