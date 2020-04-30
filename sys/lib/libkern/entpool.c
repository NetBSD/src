/*	$NetBSD: entpool.c,v 1.1 2020/04/30 03:28:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

/*
 * Entropy pool (`reseedable pseudorandom number generator') based on a
 * sponge duplex, following the design described and analyzed in
 *
 *	Guido Bertoni, Joan Daemen, Michaël Peeters, and Gilles Van
 *	Assche, `Sponge-Based Pseudo-Random Number Generators', in
 *	Stefan Mangard and François-Xavier Standaert, eds.,
 *	Cryptographic Hardware and Embedded Systems—CHES 2010, Springer
 *	LNCS 6225, pp. 33–47.
 *	https://link.springer.com/chapter/10.1007/978-3-642-15031-9_3
 *	https://keccak.team/files/SpongePRNG.pdf
 *
 *	Guido Bertoni, Joan Daemen, Michaël Peeters, and Gilles Van
 *	Assche, `Duplexing the Sponge: Single-Pass Authenticated
 *	Encryption and Other Applications', in Ali Miri and Serge
 *	Vaudenay, eds., Selected Areas in Cryptography—SAC 2011,
 *	Springer LNCS 7118, pp. 320–337.
 *	https://link.springer.com/chapter/10.1007/978-3-642-28496-0_19
 *	https://keccak.team/files/SpongeDuplex.pdf
 *
 * We make the following tweaks that don't affect security:
 *
 *	- Samples are length-delimited 7-bit variable-length encoding.
 *	  The encoding is still injective, so the security theorems
 *	  continue to apply.
 *
 *	- Output is not buffered -- callers should draw 32 bytes and
 *	  expand with a stream cipher.  In effect, every output draws
 *	  the full rate, and we just discard whatever the caller didn't
 *	  ask for; the impact is only on performance, not security.
 *
 * On top of the underlying sponge state, an entropy pool maintains an
 * integer i in [0, RATE-1] indicating where to write the next byte in
 * the input buffer.  Zeroing an entropy pool initializes it.
 */

#if defined(_KERNEL) || defined(_STANDALONE)
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: entpool.c,v 1.1 2020/04/30 03:28:19 riastradh Exp $");
#endif

#include "entpool.h"
#include ENTPOOL_HEADER

#if defined(_KERNEL) || defined(_STANDALONE)
#include <sys/types.h>
#include <lib/libkern/libkern.h>
#define	ASSERT		KASSERT
#else
#include <sys/cdefs.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define	ASSERT		assert
#define	CTASSERT	__CTASSERT
#endif

#define	secret	/* must not use in variable-time operations; should zero */
#define	arraycount(A)	(sizeof(A)/sizeof((A)[0]))
#define	MIN(X,Y)	((X) < (Y) ? (X) : (Y))

#define	RATE		ENTPOOL_RATE

/*
 * stir(P)
 *
 *	Internal subroutine to apply the sponge permutation to the
 *	state in P.  Resets P->i to 0 to indicate that the input buffer
 *	is empty.
 */
static void
stir(struct entpool *P)
{
	size_t i;

	/*
	 * Switch to the permutation's byte order, if necessary, apply
	 * permutation, and then switch back.  This way we can data in
	 * and out byte by byte, but get the same answers out of test
	 * vectors.
	 */
	for (i = 0; i < arraycount(P->s.w); i++)
		P->s.w[i] = ENTPOOL_WTOH(P->s.w[i]);
	ENTPOOL_PERMUTE(P->s.w);
	for (i = 0; i < arraycount(P->s.w); i++)
		P->s.w[i] = ENTPOOL_HTOW(P->s.w[i]);

	/* Reset the input buffer.  */
	P->i = 0;
}

/*
 * entpool_enter(P, buf, len)
 *
 *	Enter len bytes from buf into the entropy pool P, stirring as
 *	needed.  Corresponds to P.feed in the paper.
 */
void
entpool_enter(struct entpool *P, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t n = len, n1 = n;

	/* Sanity-check P->i.  */
	ASSERT(P->i <= RATE-1);

	/* Encode the length, stirring as needed.  */
	while (n1) {
		if (P->i == RATE-1)
			stir(P);
		ASSERT(P->i < RATE-1);
		P->s.u8[P->i++] ^= (n1 >= 0x80 ? 0x80 : 0) | (n1 & 0x7f);
		n1 >>= 7;
	}

	/* Enter the sample, stirring as needed.  */
	while (n --> 0) {
		if (P->i == RATE-1)
			stir(P);
		ASSERT(P->i < RATE-1);
		P->s.u8[P->i++] ^= *p++;
	}

	/* If we filled the input buffer exactly, stir once more.  */
	if (P->i == RATE-1)
		stir(P);
	ASSERT(P->i < RATE-1);
}

/*
 * entpool_enter_nostir(P, buf, len)
 *
 *	Enter as many bytes as possible, up to len, from buf into the
 *	entropy pool P.  Roughly corresponds to P.feed in the paper,
 *	but we stop if we would have run the permutation.
 *
 *	Return true if the sample was consumed in its entirety, or true
 *	if the sample was truncated so the caller should arrange to
 *	call entpool_stir when it is next convenient to do so.
 *
 *	This function is cheap -- it only xors the input into the
 *	state, and never calls the underlying permutation, but it may
 *	truncate samples.
 */
bool
entpool_enter_nostir(struct entpool *P, const void *buf, size_t len)
{
	const uint8_t *p = buf;
	size_t n0, n;

	/* Sanity-check P->i.  */
	ASSERT(P->i <= RATE-1);

	/* If the input buffer is full, fail.  */
	if (P->i == RATE-1)
		return false;
	ASSERT(P->i < RATE-1);

	/*
	 * Truncate the sample and enter it with 1-byte length encoding
	 * -- don't bother with variable-length encoding, not worth the
	 * trouble.
	 */
	n = n0 = MIN(127, MIN(len, RATE-1 - P->i - 1));
	P->s.u8[P->i++] ^= n;
	while (n --> 0)
		P->s.u8[P->i++] ^= *p++;

	/* Can't guarantee anything better than 0 <= i <= RATE-1.  */
	ASSERT(P->i <= RATE-1);

	/* Return true if all done, false if truncated and in need of stir.  */
	return (n0 == len);
}

/*
 * entpool_stir(P)
 *
 *	Stir the entropy pool after entpool_enter_nostir fails.  If it
 *	has already been stirred already, this has no effect.
 */
void
entpool_stir(struct entpool *P)
{

	/* Sanity-check P->i.  */
	ASSERT(P->i <= RATE-1);

	/* If the input buffer is full, stir.  */
	if (P->i == RATE-1)
		stir(P);
	ASSERT(P->i < RATE-1);
}

/*
 * entpool_extract(P, buf, len)
 *
 *	Extract len bytes from the entropy pool P into buf.
 *	Corresponds to iterating P.fetch/P.forget in the paper.
 *	(Feeding the output back in -- as P.forget does -- is the same
 *	as zeroing what we just read out.)
 */
void
entpool_extract(struct entpool *P, secret void *buf, size_t len)
{
	uint8_t *p = buf;
	size_t n = len;

	/* Sanity-check P->i.  */
	ASSERT(P->i <= RATE-1);

	/* If input buffer is not empty, stir.  */
	if (P->i != 0)
		stir(P);
	ASSERT(P->i == 0);

	/*
	 * Copy out and zero (RATE-1)-sized chunks at a time, stirring
	 * with a bit set to distinguish this from inputs.
	 */
	while (n >= RATE-1) {
		memcpy(p, P->s.u8, RATE-1);
		memset(P->s.u8, 0, RATE-1);
		P->s.u8[RATE-1] ^= 0x80;
		stir(P);
		p += RATE-1;
		n -= RATE-1;
	}

	/*
	 * If there's anything left, copy out a partial rate's worth
	 * and zero the entire rate's worth, stirring with a bit set to
	 * distinguish this from inputs.
	 */
	if (n) {
		ASSERT(n < RATE-1);
		memcpy(p, P->s.u8, n);		/* Copy part of it.  */
		memset(P->s.u8, 0, RATE-1);	/* Zero all of it. */
		P->s.u8[RATE-1] ^= 0x80;
		stir(P);
	}
}

/*
 * Known-answer tests
 */

#if ENTPOOL_SMALL

#define	KATLEN	15

/* Gimli */
static const uint8_t known_answers[][KATLEN] = {
	[0] = {
		0x69,0xb8,0x49,0x0d,0x39,0xfb,0x42,0x61,
		0xf7,0x66,0xdf,0x04,0xb6,0xed,0x11,
	},
	[1] = {
		0x74,0x15,0x16,0x49,0x31,0x07,0x77,0xa1,
		0x3b,0x4d,0x78,0xc6,0x5d,0xef,0x87,
	},
	[2] = {
		0xae,0xfd,0x7d,0xc4,0x3b,0xce,0x09,0x25,
		0xbf,0x60,0x21,0x6e,0x3c,0x3a,0x84,
	},
	[3] = {
		0xae,0xfd,0x7d,0xc4,0x3b,0xce,0x09,0x25,
		0xbf,0x60,0x21,0x6e,0x3c,0x3a,0x84,
	},
	[4] = {
		0x69,0xb8,0x49,0x0d,0x39,0xfb,0x42,0x61,
		0xf7,0x66,0xdf,0x04,0xb6,0xed,0x11,
	},
	[5] = {
		0xa9,0x3c,0x3c,0xac,0x5f,0x6d,0x80,0xdc,
		0x33,0x0c,0xb2,0xe3,0xdd,0x55,0x31,
	},
	[6] = {
		0x2e,0x69,0x1a,0x2a,0x2d,0x09,0xd4,0x5e,
		0x49,0xcc,0x8c,0xb2,0x0b,0xcc,0x42,
	},
	[7] = {
		0xae,0xfd,0x7d,0xc4,0x3b,0xce,0x09,0x25,
		0xbf,0x60,0x21,0x6e,0x3c,0x3a,0x84,
	},
	[8] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	},
	[9] = {
		0x69,0xb8,0x49,0x0d,0x39,0xfb,0x42,0x61,
		0xf7,0x66,0xdf,0x04,0xb6,0xed,0x11,
	},
	[10] = {
		0x2e,0x69,0x1a,0x2a,0x2d,0x09,0xd4,0x5e,
		0x49,0xcc,0x8c,0xb2,0x0b,0xcc,0x42,
	},
	[11] = {
		0x6f,0xfd,0xd2,0x29,0x78,0x46,0xc0,0x7d,
		0xc7,0xf2,0x0a,0x2b,0x72,0xd6,0xc6,
	},
	[12] = {
		0x86,0xf0,0xc1,0xf9,0x95,0x0f,0xc9,0x12,
		0xde,0x38,0x39,0x10,0x1f,0x8c,0xc4,
	},
};

#else  /* !ENTPOOL_SMALL */

#define	KATLEN	16

/* Keccak-p[1600, 24] */
static const uint8_t known_answers[][KATLEN] = {
	[0] = {
		0x3b,0x20,0xf0,0xe9,0xce,0x94,0x48,0x07,
		0x97,0xb6,0x16,0xb5,0xb5,0x05,0x1a,0xce,
	},
	[1] = {
		0x57,0x49,0x6e,0x28,0x7f,0xaa,0xee,0x6c,
		0xa8,0xb0,0xf5,0x0b,0x87,0xae,0xd6,0xd6,
	},
	[2] = {
		0x51,0x72,0x0f,0x59,0x54,0xe1,0xaf,0xa8,
		0x16,0x67,0xfa,0x3f,0x8a,0x19,0x52,0x50,
	},
	[3] = {
		0x51,0x72,0x0f,0x59,0x54,0xe1,0xaf,0xa8,
		0x16,0x67,0xfa,0x3f,0x8a,0x19,0x52,0x50,
	},
	[4] = {
		0x3b,0x20,0xf0,0xe9,0xce,0x94,0x48,0x07,
		0x97,0xb6,0x16,0xb5,0xb5,0x05,0x1a,0xce,
	},
	[5] = {
		0x95,0x23,0x77,0xe4,0x84,0xeb,0xaa,0x2e,
		0x6a,0x99,0xc2,0x52,0x06,0x6d,0xdf,0xea,
	},
	[6] = {
		0x8c,0xdd,0x1b,0xaf,0x0e,0xf6,0xe9,0x1d,
		0x51,0x33,0x68,0x38,0x8d,0xad,0x55,0x84,
	},
	[7] = {
		0x51,0x72,0x0f,0x59,0x54,0xe1,0xaf,0xa8,
		0x16,0x67,0xfa,0x3f,0x8a,0x19,0x52,0x50,
	},
	[8] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	},
	[9] = {
		0x3b,0x20,0xf0,0xe9,0xce,0x94,0x48,0x07,
		0x97,0xb6,0x16,0xb5,0xb5,0x05,0x1a,0xce,
	},
	[10] = {
		0x8c,0xdd,0x1b,0xaf,0x0e,0xf6,0xe9,0x1d,
		0x51,0x33,0x68,0x38,0x8d,0xad,0x55,0x84,
	},
	[11] = {
		0xf6,0xc1,0x14,0xbb,0x13,0x0a,0xaf,0xed,
		0xca,0x0b,0x35,0x2c,0xf1,0x2b,0x1a,0x85,
	},
	[12] = {
		0xf9,0x4b,0x05,0xd1,0x8b,0xcd,0xb3,0xd0,
		0x77,0x27,0xfe,0x46,0xf9,0x33,0xb2,0xa2,
	},
};

#endif

#define	KAT_BEGIN(P, n)	memset(P, 0, sizeof(*(P)))
#define	KAT_ERROR()	return -1
#define	KAT_END(P, n)	do						      \
{									      \
	uint8_t KAT_ACTUAL[KATLEN];					      \
	entpool_extract(P, KAT_ACTUAL, KATLEN);				      \
	if (memcmp(KAT_ACTUAL, known_answers[n], KATLEN))		      \
		return -1;						      \
} while (0)

int
entpool_selftest(void)
{
	struct entpool pool, *P = &pool;
	uint8_t sample[1] = {0xff};
	uint8_t scratch[RATE];
	const uint8_t zero[RATE] = {0};

	/* Test entpool_enter with empty buffer.  */
	KAT_BEGIN(P, 0);
	entpool_stir(P);	/* noop */
	entpool_enter(P, sample, 1);
	entpool_stir(P);	/* noop */
	KAT_END(P, 0);

	/* Test entpool_enter with partial buffer.  */
	KAT_BEGIN(P, 1);
	entpool_stir(P);	/* noop */
#if ENTPOOL_SMALL
	entpool_enter(P, zero, RATE-3);
#else
	entpool_enter(P, zero, RATE-4);
#endif
	entpool_stir(P);	/* noop */
	entpool_enter(P, sample, 1);
	entpool_stir(P);	/* noop */
	KAT_END(P, 1);

	/* Test entpool_enter with full buffer.  */
	KAT_BEGIN(P, 2);
	entpool_stir(P);	/* noop */
#if ENTPOOL_SMALL
	if (!entpool_enter_nostir(P, zero, RATE-2))
		KAT_ERROR();
#else
	if (!entpool_enter_nostir(P, zero, 127))
		KAT_ERROR();
	if (!entpool_enter_nostir(P, zero, RATE-2 - 127 - 1))
		KAT_ERROR();
#endif
	entpool_enter(P, sample, 1);
	entpool_stir(P);	/* noop */
	KAT_END(P, 2);

	/* Test entpool_enter with full buffer after stir.  */
	KAT_BEGIN(P, 3);
	entpool_stir(P);	/* noop */
#if ENTPOOL_SMALL
	if (!entpool_enter_nostir(P, zero, RATE-2))
		KAT_ERROR();
#else
	CTASSERT(127 <= RATE-2);
	if (!entpool_enter_nostir(P, zero, 127))
		KAT_ERROR();
	if (!entpool_enter_nostir(P, zero, RATE-2 - 127 - 1))
		KAT_ERROR();
#endif
	entpool_stir(P);
	entpool_enter(P, sample, 1);
	entpool_stir(P);	/* noop */
	KAT_END(P, 3);

	/* Test entpool_enter_nostir with empty buffer.  */
	KAT_BEGIN(P, 4);
	entpool_stir(P);	/* noop */
	if (!entpool_enter_nostir(P, sample, 1))
		KAT_ERROR();
	entpool_stir(P);	/* noop */
	KAT_END(P, 4);

	/* Test entpool_enter_nostir with partial buffer.  */
	KAT_BEGIN(P, 5);
	entpool_stir(P);	/* noop */
#if ENTPOOL_SMALL
	entpool_enter(P, zero, RATE-3);
#else
	entpool_enter(P, zero, RATE-4);
#endif
	entpool_stir(P);	/* noop */
	if (entpool_enter_nostir(P, sample, 1))
		KAT_ERROR();
	entpool_stir(P);
	KAT_END(P, 5);

	/* Test entpool_enter_nostir with full buffer.  */
	KAT_BEGIN(P, 6);
	entpool_stir(P);	/* noop */
#if ENTPOOL_SMALL
	if (!entpool_enter_nostir(P, zero, RATE-2))
		KAT_ERROR();
#else
	CTASSERT(127 <= RATE-2);
	if (!entpool_enter_nostir(P, zero, 127))
		KAT_ERROR();
	if (!entpool_enter_nostir(P, zero, RATE-2 - 127 - 1))
		KAT_ERROR();
#endif
	if (entpool_enter_nostir(P, sample, 1))
		KAT_ERROR();
	entpool_stir(P);
	KAT_END(P, 6);

	/* Test entpool_enter_nostir with full buffer after stir.  */
	KAT_BEGIN(P, 7);
	entpool_stir(P);	/* noop */
#if ENTPOOL_SMALL
	if (!entpool_enter_nostir(P, zero, RATE-2))
		KAT_ERROR();
#else
	CTASSERT(127 <= RATE-2);
	if (!entpool_enter_nostir(P, zero, 127))
		KAT_ERROR();
	if (!entpool_enter_nostir(P, zero, RATE-2 - 127 - 1))
		KAT_ERROR();
#endif
	entpool_stir(P);
	if (!entpool_enter_nostir(P, sample, 1))
		KAT_ERROR();
	entpool_stir(P);	/* noop */
	KAT_END(P, 7);

	/* Test entpool_extract with empty input buffer.  */
	KAT_BEGIN(P, 8);
	entpool_stir(P);	/* noop */
	KAT_END(P, 8);

	/* Test entpool_extract with nonempty input buffer.  */
	KAT_BEGIN(P, 9);
	entpool_stir(P);	/* noop */
	entpool_enter(P, sample, 1);
	entpool_stir(P);	/* noop */
	KAT_END(P, 9);

	/* Test entpool_extract with full input buffer.  */
	KAT_BEGIN(P, 10);
	entpool_stir(P);	/* noop */
#if ENTPOOL_SMALL
	if (!entpool_enter_nostir(P, zero, RATE-2))
		KAT_ERROR();
#else
	CTASSERT(127 <= RATE-2);
	if (!entpool_enter_nostir(P, zero, 127))
		KAT_ERROR();
	if (!entpool_enter_nostir(P, zero, RATE-2 - 127 - 1))
		KAT_ERROR();
#endif
	KAT_END(P, 10);

	/* Test entpool_extract with iterated output.  */
	KAT_BEGIN(P, 11);
	entpool_stir(P);	/* noop */
	entpool_extract(P, scratch, RATE-1 + 1);
	entpool_stir(P);	/* noop */
	KAT_END(P, 11);

	/* Test extract, enter, extract.  */
	KAT_BEGIN(P, 12);
	entpool_stir(P);	/* noop */
	entpool_extract(P, scratch, 1);
	entpool_stir(P);	/* noop */
	entpool_enter(P, sample, 1);
	entpool_stir(P);	/* noop */
	KAT_END(P, 12);

	return 0;
}

#if ENTPOOL_TEST
int
main(void)
{
	return entpool_selftest();
}
#endif

/*
 * Known-answer test generation
 *
 *	This generates the known-answer test vectors from explicitly
 *	specified duplex inputs that correspond to what entpool_enter
 *	&c. induce, to confirm the encoding of inputs works as
 *	intended.
 */

#if ENTPOOL_GENKAT

#include <stdio.h>

struct event {
	enum { IN, OUT, STOP } t;
	uint8_t b[RATE-1];
};

/* Cases correspond to entpool_selftest above.  */
static const struct event *const cases[] = {
	[0] = (const struct event[]) {
		{IN, {1, 0xff}},
		{STOP, {0}},
	},
	[1] = (const struct event[]) {
#if ENTPOOL_SMALL
		{IN, {RATE-3, [RATE-2] = 1}},
#else
		{IN, {0x80|((RATE-4)&0x7f), (RATE-4)>>7, [RATE-2] = 1}},
#endif
		{IN, {0xff}},
		{STOP, {0}},
	},
	[2] = (const struct event[]) {
#if ENTPOOL_SMALL
		{IN, {RATE-2}},
#else
		{IN, {127, [128] = RATE-2 - 127 - 1}},
#endif
		{IN, {1, 0xff}},
		{STOP, {0}},
	},
	[3] = (const struct event[]) {
#if ENTPOOL_SMALL
		{IN, {RATE-2}},
#else
		{IN, {127, [128] = RATE-2 - 127 - 1}},
#endif
		{IN, {1, 0xff}},
		{STOP, {0}},
	},
	[4] = (const struct event[]) {
		{IN, {1, 0xff}},
		{STOP, {0}},
	},

	[5] = (const struct event[]) {
#if ENTPOOL_SMALL
		{IN, {RATE-3, [RATE-2] = 0 /* truncated length */}},
#else
		{IN, {0x80|((RATE-4)&0x7f), (RATE-4)>>7,
		      [RATE-2] = 0 /* truncated length */}},
#endif
		{STOP, {0}},
	},
	[6] = (const struct event[]) {
#if ENTPOOL_SMALL
		{IN, {RATE-2}},
#else
		{IN, {127, [128] = RATE-2 - 127 - 1}},
#endif
		{STOP, {0}},
	},
	[7] = (const struct event[]) {
#if ENTPOOL_SMALL
		{IN, {RATE-2}},
#else
		{IN, {127, [128] = RATE-2 - 127 - 1}},
#endif
		{IN, {1, 0xff}},
		{STOP, {0}},
	},
	[8] = (const struct event[]) {
		{STOP, {0}},
	},
	[9] = (const struct event[]) {
		{IN, {1, 0xff}},
		{STOP, {0}},
	},
	[10] = (const struct event[]) {
#if ENTPOOL_SMALL
		{IN, {RATE-2}},
#else
		{IN, {127, [128] = RATE-2 - 127 - 1}},
#endif
		{STOP, {0}},
	},
	[11] = (const struct event[]) {
		{OUT, {0}},
		{OUT, {0}},
		{STOP, {0}},
	},
	[12] = (const struct event[]) {
		{OUT, {0}},
		{IN, {1, 0xff}},
		{STOP, {0}},
	},
};

static void
compute(uint8_t output[KATLEN], const struct event *events)
{
	union {
		uint8_t b[ENTPOOL_SIZE];
		ENTPOOL_WORD w[ENTPOOL_SIZE/sizeof(ENTPOOL_WORD)];
	} u;
	unsigned i, j, k;

	memset(&u.b, 0, sizeof u.b);
	for (i = 0;; i++) {
		if (events[i].t == STOP)
			break;
		for (j = 0; j < sizeof(events[i].b); j++)
			u.b[j] ^= events[i].b[j];
		if (events[i].t == OUT) {
			memset(u.b, 0, RATE-1);
			u.b[RATE-1] ^= 0x80;
		}

		for (k = 0; k < arraycount(u.w); k++)
			u.w[k] = ENTPOOL_WTOH(u.w[k]);
		ENTPOOL_PERMUTE(u.w);
		for (k = 0; k < arraycount(u.w); k++)
			u.w[k] = ENTPOOL_HTOW(u.w[k]);
	}

	for (j = 0; j < KATLEN; j++)
		output[j] = u.b[j];
}

int
main(void)
{
	uint8_t output[KATLEN];
	unsigned i, j;

	printf("static const uint8_t known_answers[][KATLEN] = {\n");
	for (i = 0; i < arraycount(cases); i++) {
		printf("\t[%u] = {\n", i);
		compute(output, cases[i]);
		for (j = 0; j < KATLEN; j++) {
			if (j % 8 == 0)
				printf("\t\t");
			printf("0x%02hhx,", output[j]);
			if (j % 8 == 7)
				printf("\n");
		}
		if ((KATLEN % 8) != 0)
			printf("\n");
		printf("\t},\n");
	}
	printf("};\n");

	fflush(stdout);
	return ferror(stdout);
}

#endif
