/* $NetBSD: ieee.h,v 1.10 2014/01/29 01:34:44 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

/*
 * ieee.h defines the machine-dependent layout of the machine's IEEE
 * floating point.  It does *not* define (yet?) any of the rounding
 * mode bits, exceptions, and so forth.
 */

#ifndef _ARM_IEEE_H_
#define _ARM_IEEE_H_

#include <sys/ieee754.h>

#if 0
#define	SNG_QUIETNAN	(1 << 22)
#define	DBL_QUIETNAN	(1 << 19)
#endif

#ifdef __ARM_PCS_AAPCS64

/*
 * The AArch64 architecture defines the following IEEE 754 compliant
 * 128-bit extended-precision format.
 */

#define	EXT_EXPBITS	15
#define EXT_FRACHBITS	48
#define	EXT_FRACLBITS	64
#define	EXT_FRACBITS	(EXT_FRACLBITS + EXT_FRACHBITS)

#define	EXT_TO_ARRAY32(u, a) do {				\
	(a)[0] = (uint32_t)((u).extu_ext.ext_fracl >>  0);	\
	(a)[1] = (uint32_t)((u).extu_ext.ext_fracl >> 32);	\
	(a)[2] = (uint32_t)((u).extu_ext.ext_frach >>  0);	\
	(a)[3] = (uint32_t)((u).extu_ext.ext_frach >> 32);	\
} while(/*CONSTCOND*/0)

struct ieee_ext {
#ifdef __AARCH64EB__
	uint64_t	ext_sign:1;
	uint64_t	ext_exp:EXT_EXPBITS;
	uint64_t	ext_frach:EXT_FRACHBITS;
	uint64_t	ext_fracl;
#else
	uint64_t	ext_fracl;
	uint64_t	ext_frach:EXT_FRACHBITS;
	uint64_t	ext_exp:EXT_EXPBITS;
	uint64_t	ext_sign:1;
#endif
};

/*
 * Floats whose exponent is in [1..INFNAN) (of whatever type) are
 * `normal'.  Floats whose exponent is INFNAN are either Inf or NaN.
 * Floats whose exponent is zero are either zero (iff all fraction
 * bits are zero) or subnormal values.
 *
 * A NaN is a `signalling NaN' if its QUIETNAN bit is clear in its
 * high fraction; if the bit is set, it is a `quiet NaN'.
 */
#define	EXT_EXP_INFNAN	0x7fff
#define	EXT_EXP_INF	0x7fff
#define	EXT_EXP_NAN	0x7fff

#if 0
#define	EXT_QUIETNAN	(1 << 15)
#endif

/*
 * Exponent biases.
 */
#define	EXT_EXP_BIAS	16383

/*
 * Convenience data structures.
 */
union ieee_ext_u {
	long double		extu_ld;
	struct ieee_ext		extu_ext;
};

#define extu_exp	extu_ext.ext_exp
#define extu_sign	extu_ext.ext_sign
#define extu_fracl	extu_ext.ext_fracl
#define extu_frach	extu_ext.ext_frach

#define LDBL_IMPLICIT_NBIT	1	/* our NBIT is implicit */

#endif /* __ARM_PCS_AAPCS64 */

#endif /* !_ARM_IEEE_H_ */
