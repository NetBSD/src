/*	$NetBSD: ccrand_var.h,v 1.1.2.1 2014/07/17 14:03:33 tls Exp $ */

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
 * ccrand_var.h
 *
 * Internal declarations for the chacha pseudo-random number generator.
 */
#ifndef	__CCRAND_VAR_H__
#define	__CCRAND_VAR_H__

/*
 * Always include the inlines...
 */
#define	_CCRAND_NEED_INLINES		/* nothing */
#include <crypto/ccrand/ccrand.h>

/*
 * These two macros are shared between __ccrand_gen16() and
 * ccrand_reseed().  Chosen to match the reference implementation.
 */
#define ROTATE(v, c)	(((v) << (c)) | ((v) >> (32 - (c))))
#define PLUS(v, w)	((v) + (w))

/*
 * Save some typing.
 */
#define	ll(x)	((uint64_t)(x))
#define	ss(x)	((uint32_t)(x))

/*
 * The ChaCha family apparently has 8, 12 and 20 round variants.
 * You can set the following to 8, 12 or 20 to compile the one you
 * want.  Right now we use ChaCha20 since almost all secure uses
 * of the keystream generator use this, but for generic PRNG
 * use it seems like 12 or 8 might produce numbers which are as
 * random.
 */
#define	CHACHA_R	12

/*
 * The 32 word context array is divided into two 16-word parts.  The
 * first 16 words buffer up to 15 generated but not yet used random
 * values, with the first of the 16 words being an index to the first
 * unused value.  That is, we have:
 *
 * word 0	- index to first unused random word, range 1 to 16 (16
 *		  indicates the buffer is empty).  A value outside that
 *		  range is taken to mean the PRNG has not yet been seeded.
 *
 * words 1-15	- pre-generated random words.  The cipher produces 16
 *		  words at a time, but since we only generate new values
 *		  when we are going to use at least one of them 15 is
 *		  sufficient to hold the left-overs.
 *
 * The second 16 words holds the cipher's context and is organized the
 * way the chacha implementation does it.  All 16 words are hashed to
 * generate a new block, with the counter being incremented afterwards.
 * We have
 *
 * words 0-3	- constant values.  I think these may serve the function
 *		  of avoiding letting the user set the context to a weak
 *		  state (all-zeros would produce 16 0-valued words, for
 *		  example).  ISAAC should have done this...
 *
 * words 4-11	- holds the "key".  It is filled in by data given to the
 *		  seed function, with the data being repeated if we don't
 *		  get enough to fill the 8 words (plus the iv).  It is set
 *		  to internal constants if the seed function is called
 *		  with no data (which isn't secure but should be
 *		  random-looking)
 *
 * words 12-13	- a 64-bit counter.  It is set to zero when the cipher
 *		  is (re)seeded and is incremented after a block
 *		  is generated.
 *
 * words 14-15	- the "iv".  There is no distinction between "iv" and
 *		  "key" in the hash code, this is an arbitrary distinction
 *		  made by chacha's API.
 *
 * The _seed() and _reseed() functions mimic the chacha _keysetup() and
 * _ivsetup() functions when called with the appropriate number of words
 * of seed.  Calling the _seed() function with 4 words sets a "128-bit key",
 * while a call with 6 words uses the last 2 to fill in the iv.  Similarly,
 * a call with 8 words sets a "256-bit key", while the additional 2 words
 * of a 10 word seed are used for the iv.  If the _reseed() function is called
 * with exactly 2 words it replaces the iv with them (if called with more it
 * uses the extra to alter the key).  When called with other amounts of seed,
 * however, it does what it can to make best use of that but won't be
 * compatible with the reference implementation.
 *
 * Here are some #defines.
 */
#define	N	16		/* number of words per block */

#define	V_BUF	0		/* index to random word buffer */
#define	V_INDEX	0		/* index of index word */

#define	V_CTX	16		/* index to cipher context */
#define	V_CONST	0		/* index to constant in cipher context */
#define	V_KEY	4		/* index to "key" */
#define	V_CNT	12		/* index to low order counter word */
#define	V_CNTHI	13
#define	V_IV	14		/* index to iv */
#define	V_IV2	15

#endif	/* __CCRAND_VAR_H__ */
