/*	$NetBSD: ccrand.h,v 1.1.2.1 2014/07/17 14:03:33 tls Exp $ */

/*
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dennis Ferguson.
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
 * ccrand.h
 *
 * Definitions for the chacha-based pseudo-random number generator
 */
#ifndef	__CCRAND_H__
#define	__CCRAND_H__
#include <sys/types.h>
#include <sys/null.h>

/*
 * Context structure.  Just 32 words.  The first 16 buffer previously
 * generated but unused values, the last 16 are our key state.
 */
typedef struct __ccrand_t {
	uint32_t v[32];
} ccrand_t;


/*
 * Declarations of functions which are always external
 */
void ccrand_copy_state(ccrand_t * __restrict, const ccrand_t * __restrict);
void ccrand_seed(ccrand_t *, const uint32_t *, unsigned int);
void ccrand_reseed(ccrand_t *, const uint32_t *, unsigned int);
void ccrand_seed32(ccrand_t *, uint32_t);
void ccrand_seed64(ccrand_t *, uint64_t);
void ccrand_bytes(ccrand_t * __restrict, void * __restrict, size_t);
void ccrand_words(ccrand_t *, uint32_t *, unsigned int);
uint64_t ccrand_use(ccrand_t *);

uint32_t __ccrand_gen16(uint32_t *, uint32_t *);

/*
 * __ccrand_getword_inline()
 *
 * Internal function to get a 32 bit random word.  It
 * doesn't check whether the cipher has been seeded.
 */
static inline uint32_t
__ccrand_getword_inline(ccrand_t *x)
{
	uint32_t r;

	if (x->v[0] == 16) {
		r = __ccrand_gen16(&x->v[0], &x->v[16]);
	} else {
		r = x->v[x->v[0]++];
	}

	return (r);
}


/*
 * __ccrand32_inline()
 *
 * Return a 32 bit random value.
 */
static inline uint32_t
__ccrand32_inline(ccrand_t *x)
{

	if ((x->v[0] - 1) >= 16) {
		ccrand_seed(x, 0, 0);
	}

	return (__ccrand_getword_inline(x));
}


/*
 * __ccrand64_inline()
 *
 * Return a 64 bit random value.
 */
static inline uint64_t
__ccrand64_inline(ccrand_t *x)
{
	uint32_t r0, r1;

	if ((x->v[0] - 1) >= 16) {
		ccrand_seed(x, 0, 0);
	}

	switch (x->v[0]) {
	case 16:
		r0 = __ccrand_gen16(&x->v[0], &x->v[16]);
		r1 = x->v[x->v[0]++];
		break;

	case 15:
		r0 = x->v[15];
		r1 = __ccrand_gen16(&x->v[0], &x->v[16]);
		break;

	default:
		r0 = x->v[x->v[0]++];
		r1 = x->v[x->v[0]++];
		break;
	}

	return (((uint64_t) r1 << 32) | (uint64_t) r0);
}


/*
 * __ccrand2_inline()
 *
 * Return a 32 bit value betweeen 0 and (2^n - 1), inclusive
 */
static inline uint32_t
__ccrand2_inline(ccrand_t *x, unsigned int n)
{
	uint32_t r;

	if ((n - 1) > 31) {
		r = 0;
	} else {
		r = __ccrand32_inline(x);
		r &= 0xffffffff >> (32 - n);
	}

	return (r);
}


/*
 * __ccrandn_inline()
 *
 * Return a 32 bit value between 0 and n-1, inclusive.  The results
 * will be a wee bit biased when n is not a power of 2, with the worst
 * case being values just under 2048 where the bias will approach
 * 2^-21.
 */
static inline uint32_t
__ccrandn_inline(ccrand_t *x, uint32_t n)
{
	uint64_t rl;

	rl = (uint64_t) n * __ccrand32_inline(x);
	if (n > 2048) {
		rl += ((uint64_t) n * __ccrand_getword_inline(x)) >> 32;
	}
	return ((uint32_t) (rl >> 32));
}

/*
 * Now the remaining declarations.  Define them as the inline versions
 * if he wants that, or as externals otherwise.
 */
#ifdef CCRAND_INLINE
#define	ccrand32(x)	__ccrand32_inline((x))
#define	ccrand64(x)	__ccrand64_inline((x))
#define	ccrand2(x, n)	__ccrand2_inline((x), (n))
#define	ccrandn(x, n)	__ccrandn_inline((x), (n))
#else	/* CCRAND_INLINE */
uint32_t ccrand32(ccrand_t *);
uint64_t ccrand64(ccrand_t *);
uint32_t ccrand2(ccrand_t *, unsigned int);
uint32_t ccrandn(ccrand_t *, uint32_t);
#endif	/* CCRAND_INLINE */

#endif	/* __CCRAND_H__ */

