/*	$NetBSD: chacha_ref.c,v 1.1 2020/07/25 22:46:34 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * ChaCha pseudorandom function family and stream cipher portable C
 * implementation.  Derived from the specification,
 *
 *	Daniel J. Bernstein, `ChaCha, a variant of Salsa20', Workshop
 *	Record of the State of the Art in Stream Ciphers -- SASC 2008.
 *	https://cr.yp.to/papers.html#chacha
 *
 * which in turn builds on the specification of Salsa20 available at
 * <https://cr.yp.to/snuffle.html>.  The particular parametrization of
 * the stream cipher, with a 32-bit block counter and 96-bit nonce, is
 * described in
 *
 *	Y. Nir and A. Langley, `ChaCha20 and Poly1305 for IETF
 *	Protocols', IETF RFC 8439, June 2018.
 *	https://tools.ietf.org/html/rfc8439
 */

#include "chacha_ref.h"

static uint32_t
rol32(uint32_t u, unsigned c)
{

	return (u << c) | (u >> (32 - c));
}

#define	CHACHA_QUARTERROUND(a, b, c, d) do				      \
{									      \
	(a) += (b); (d) ^= (a); (d) = rol32((d), 16);			      \
	(c) += (d); (b) ^= (c); (b) = rol32((b), 12);			      \
	(a) += (b); (d) ^= (a); (d) = rol32((d),  8);			      \
	(c) += (d); (b) ^= (c); (b) = rol32((b),  7);			      \
} while (/*CONSTCOND*/0)

const uint8_t chacha_const32[16] = "expand 32-byte k";

static void
chacha_core_ref(uint8_t out[restrict static 64], const uint8_t in[static 16],
    const uint8_t k[static 32], const uint8_t c[static 16], unsigned nr)
{
	uint32_t x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15;
	uint32_t y0,y1,y2,y3,y4,y5,y6,y7,y8,y9,y10,y11,y12,y13,y14,y15;

	x0 = y0 = le32dec(c + 0);
	x1 = y1 = le32dec(c + 4);
	x2 = y2 = le32dec(c + 8);
	x3 = y3 = le32dec(c + 12);
	x4 = y4 = le32dec(k + 0);
	x5 = y5 = le32dec(k + 4);
	x6 = y6 = le32dec(k + 8);
	x7 = y7 = le32dec(k + 12);
	x8 = y8 = le32dec(k + 16);
	x9 = y9 = le32dec(k + 20);
	x10 = y10 = le32dec(k + 24);
	x11 = y11 = le32dec(k + 28);
	x12 = y12 = le32dec(in + 0);
	x13 = y13 = le32dec(in + 4);
	x14 = y14 = le32dec(in + 8);
	x15 = y15 = le32dec(in + 12);

	for (; nr > 0; nr -= 2) {
		CHACHA_QUARTERROUND( y0, y4, y8,y12);
		CHACHA_QUARTERROUND( y1, y5, y9,y13);
		CHACHA_QUARTERROUND( y2, y6,y10,y14);
		CHACHA_QUARTERROUND( y3, y7,y11,y15);
		CHACHA_QUARTERROUND( y0, y5,y10,y15);
		CHACHA_QUARTERROUND( y1, y6,y11,y12);
		CHACHA_QUARTERROUND( y2, y7, y8,y13);
		CHACHA_QUARTERROUND( y3, y4, y9,y14);
	}

	le32enc(out + 0, x0 + y0);
	le32enc(out + 4, x1 + y1);
	le32enc(out + 8, x2 + y2);
	le32enc(out + 12, x3 + y3);
	le32enc(out + 16, x4 + y4);
	le32enc(out + 20, x5 + y5);
	le32enc(out + 24, x6 + y6);
	le32enc(out + 28, x7 + y7);
	le32enc(out + 32, x8 + y8);
	le32enc(out + 36, x9 + y9);
	le32enc(out + 40, x10 + y10);
	le32enc(out + 44, x11 + y11);
	le32enc(out + 48, x12 + y12);
	le32enc(out + 52, x13 + y13);
	le32enc(out + 56, x14 + y14);
	le32enc(out + 60, x15 + y15);
}

/* ChaCha stream cipher (IETF style, 96-bit nonce and 32-bit block counter) */

static void
chacha_stream_ref(uint8_t *restrict s, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t k[static 32],
    unsigned nr)
{
	const uint8_t *c = chacha_const32;
	uint32_t x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15;
	uint32_t y0,y1,y2,y3,y4,y5,y6,y7,y8,y9,y10,y11,y12,y13,y14,y15;
	unsigned i;

	x0 = le32dec(c + 0);
	x1 = le32dec(c + 4);
	x2 = le32dec(c + 8);
	x3 = le32dec(c + 12);
	x4 = le32dec(k + 0);
	x5 = le32dec(k + 4);
	x6 = le32dec(k + 8);
	x7 = le32dec(k + 12);
	x8 = le32dec(k + 16);
	x9 = le32dec(k + 20);
	x10 = le32dec(k + 24);
	x11 = le32dec(k + 28);
	/* x12 = blkno */
	x13 = le32dec(nonce + 0);
	x14 = le32dec(nonce + 4);
	x15 = le32dec(nonce + 8);

	for (; nbytes >= 64; nbytes -= 64, s += 64, blkno++) {
		y0 = x0;
		y1 = x1;
		y2 = x2;
		y3 = x3;
		y4 = x4;
		y5 = x5;
		y6 = x6;
		y7 = x7;
		y8 = x8;
		y9 = x9;
		y10 = x10;
		y11 = x11;
		y12 = x12 = blkno;
		y13 = x13;
		y14 = x14;
		y15 = x15;
		for (i = nr; i > 0; i -= 2) {
			CHACHA_QUARTERROUND( y0, y4, y8,y12);
			CHACHA_QUARTERROUND( y1, y5, y9,y13);
			CHACHA_QUARTERROUND( y2, y6,y10,y14);
			CHACHA_QUARTERROUND( y3, y7,y11,y15);
			CHACHA_QUARTERROUND( y0, y5,y10,y15);
			CHACHA_QUARTERROUND( y1, y6,y11,y12);
			CHACHA_QUARTERROUND( y2, y7, y8,y13);
			CHACHA_QUARTERROUND( y3, y4, y9,y14);
		}
		le32enc(s + 0, x0 + y0);
		le32enc(s + 4, x1 + y1);
		le32enc(s + 8, x2 + y2);
		le32enc(s + 12, x3 + y3);
		le32enc(s + 16, x4 + y4);
		le32enc(s + 20, x5 + y5);
		le32enc(s + 24, x6 + y6);
		le32enc(s + 28, x7 + y7);
		le32enc(s + 32, x8 + y8);
		le32enc(s + 36, x9 + y9);
		le32enc(s + 40, x10 + y10);
		le32enc(s + 44, x11 + y11);
		le32enc(s + 48, x12 + y12);
		le32enc(s + 52, x13 + y13);
		le32enc(s + 56, x14 + y14);
		le32enc(s + 60, x15 + y15);
	}

	if (nbytes) {
		uint8_t buf[64];

		y0 = x0;
		y1 = x1;
		y2 = x2;
		y3 = x3;
		y4 = x4;
		y5 = x5;
		y6 = x6;
		y7 = x7;
		y8 = x8;
		y9 = x9;
		y10 = x10;
		y11 = x11;
		y12 = x12 = blkno;
		y13 = x13;
		y14 = x14;
		y15 = x15;
		for (i = nr; i > 0; i -= 2) {
			CHACHA_QUARTERROUND( y0, y4, y8,y12);
			CHACHA_QUARTERROUND( y1, y5, y9,y13);
			CHACHA_QUARTERROUND( y2, y6,y10,y14);
			CHACHA_QUARTERROUND( y3, y7,y11,y15);
			CHACHA_QUARTERROUND( y0, y5,y10,y15);
			CHACHA_QUARTERROUND( y1, y6,y11,y12);
			CHACHA_QUARTERROUND( y2, y7, y8,y13);
			CHACHA_QUARTERROUND( y3, y4, y9,y14);
		}
		le32enc(buf + 0, x0 + y0);
		le32enc(buf + 4, x1 + y1);
		le32enc(buf + 8, x2 + y2);
		le32enc(buf + 12, x3 + y3);
		le32enc(buf + 16, x4 + y4);
		le32enc(buf + 20, x5 + y5);
		le32enc(buf + 24, x6 + y6);
		le32enc(buf + 28, x7 + y7);
		le32enc(buf + 32, x8 + y8);
		le32enc(buf + 36, x9 + y9);
		le32enc(buf + 40, x10 + y10);
		le32enc(buf + 44, x11 + y11);
		le32enc(buf + 48, x12 + y12);
		le32enc(buf + 52, x13 + y13);
		le32enc(buf + 56, x14 + y14);
		le32enc(buf + 60, x15 + y15);
		memcpy(s, buf, nbytes);
	}
}

static void
chacha_stream_xor_ref(uint8_t *s, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t k[static 32],
    unsigned nr)
{
	const uint8_t *c = chacha_const32;
	uint32_t x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,x14,x15;
	uint32_t y0,y1,y2,y3,y4,y5,y6,y7,y8,y9,y10,y11,y12,y13,y14,y15;
	unsigned i;

	x0 = le32dec(c + 0);
	x1 = le32dec(c + 4);
	x2 = le32dec(c + 8);
	x3 = le32dec(c + 12);
	x4 = le32dec(k + 0);
	x5 = le32dec(k + 4);
	x6 = le32dec(k + 8);
	x7 = le32dec(k + 12);
	x8 = le32dec(k + 16);
	x9 = le32dec(k + 20);
	x10 = le32dec(k + 24);
	x11 = le32dec(k + 28);
	/* x12 = blkno */
	x13 = le32dec(nonce + 0);
	x14 = le32dec(nonce + 4);
	x15 = le32dec(nonce + 8);

	for (; nbytes >= 64; nbytes -= 64, s += 64, p += 64, blkno++) {
		y0 = x0;
		y1 = x1;
		y2 = x2;
		y3 = x3;
		y4 = x4;
		y5 = x5;
		y6 = x6;
		y7 = x7;
		y8 = x8;
		y9 = x9;
		y10 = x10;
		y11 = x11;
		y12 = x12 = blkno;
		y13 = x13;
		y14 = x14;
		y15 = x15;
		for (i = nr; i > 0; i -= 2) {
			CHACHA_QUARTERROUND( y0, y4, y8,y12);
			CHACHA_QUARTERROUND( y1, y5, y9,y13);
			CHACHA_QUARTERROUND( y2, y6,y10,y14);
			CHACHA_QUARTERROUND( y3, y7,y11,y15);
			CHACHA_QUARTERROUND( y0, y5,y10,y15);
			CHACHA_QUARTERROUND( y1, y6,y11,y12);
			CHACHA_QUARTERROUND( y2, y7, y8,y13);
			CHACHA_QUARTERROUND( y3, y4, y9,y14);
		}
		le32enc(s + 0, (x0 + y0) ^ le32dec(p + 0));
		le32enc(s + 4, (x1 + y1) ^ le32dec(p + 4));
		le32enc(s + 8, (x2 + y2) ^ le32dec(p + 8));
		le32enc(s + 12, (x3 + y3) ^ le32dec(p + 12));
		le32enc(s + 16, (x4 + y4) ^ le32dec(p + 16));
		le32enc(s + 20, (x5 + y5) ^ le32dec(p + 20));
		le32enc(s + 24, (x6 + y6) ^ le32dec(p + 24));
		le32enc(s + 28, (x7 + y7) ^ le32dec(p + 28));
		le32enc(s + 32, (x8 + y8) ^ le32dec(p + 32));
		le32enc(s + 36, (x9 + y9) ^ le32dec(p + 36));
		le32enc(s + 40, (x10 + y10) ^ le32dec(p + 40));
		le32enc(s + 44, (x11 + y11) ^ le32dec(p + 44));
		le32enc(s + 48, (x12 + y12) ^ le32dec(p + 48));
		le32enc(s + 52, (x13 + y13) ^ le32dec(p + 52));
		le32enc(s + 56, (x14 + y14) ^ le32dec(p + 56));
		le32enc(s + 60, (x15 + y15) ^ le32dec(p + 60));
	}

	if (nbytes) {
		uint8_t buf[64];

		y0 = x0;
		y1 = x1;
		y2 = x2;
		y3 = x3;
		y4 = x4;
		y5 = x5;
		y6 = x6;
		y7 = x7;
		y8 = x8;
		y9 = x9;
		y10 = x10;
		y11 = x11;
		y12 = x12 = blkno;
		y13 = x13;
		y14 = x14;
		y15 = x15;
		for (i = nr; i > 0; i -= 2) {
			CHACHA_QUARTERROUND( y0, y4, y8,y12);
			CHACHA_QUARTERROUND( y1, y5, y9,y13);
			CHACHA_QUARTERROUND( y2, y6,y10,y14);
			CHACHA_QUARTERROUND( y3, y7,y11,y15);
			CHACHA_QUARTERROUND( y0, y5,y10,y15);
			CHACHA_QUARTERROUND( y1, y6,y11,y12);
			CHACHA_QUARTERROUND( y2, y7, y8,y13);
			CHACHA_QUARTERROUND( y3, y4, y9,y14);
		}
		le32enc(buf + 0, x0 + y0);
		le32enc(buf + 4, x1 + y1);
		le32enc(buf + 8, x2 + y2);
		le32enc(buf + 12, x3 + y3);
		le32enc(buf + 16, x4 + y4);
		le32enc(buf + 20, x5 + y5);
		le32enc(buf + 24, x6 + y6);
		le32enc(buf + 28, x7 + y7);
		le32enc(buf + 32, x8 + y8);
		le32enc(buf + 36, x9 + y9);
		le32enc(buf + 40, x10 + y10);
		le32enc(buf + 44, x11 + y11);
		le32enc(buf + 48, x12 + y12);
		le32enc(buf + 52, x13 + y13);
		le32enc(buf + 56, x14 + y14);
		le32enc(buf + 60, x15 + y15);
		for (i = 0; i < nbytes - nbytes%4; i += 4)
			le32enc(s + i, le32dec(p + i) ^ le32dec(buf + i));
		for (; i < nbytes; i++)
			s[i] = p[i] ^ buf[i];
	}
}

/* HChaCha */

static void
hchacha_ref(uint8_t out[restrict static 32], const uint8_t in[static 16],
    const uint8_t k[static 32], const uint8_t c[static 16], unsigned nr)
{
	uint32_t y0,y1,y2,y3,y4,y5,y6,y7,y8,y9,y10,y11,y12,y13,y14,y15;

	y0 = le32dec(c + 0);
	y1 = le32dec(c + 4);
	y2 = le32dec(c + 8);
	y3 = le32dec(c + 12);
	y4 = le32dec(k + 0);
	y5 = le32dec(k + 4);
	y6 = le32dec(k + 8);
	y7 = le32dec(k + 12);
	y8 = le32dec(k + 16);
	y9 = le32dec(k + 20);
	y10 = le32dec(k + 24);
	y11 = le32dec(k + 28);
	y12 = le32dec(in + 0);
	y13 = le32dec(in + 4);
	y14 = le32dec(in + 8);
	y15 = le32dec(in + 12);

	for (; nr > 0; nr -= 2) {
		CHACHA_QUARTERROUND( y0, y4, y8,y12);
		CHACHA_QUARTERROUND( y1, y5, y9,y13);
		CHACHA_QUARTERROUND( y2, y6,y10,y14);
		CHACHA_QUARTERROUND( y3, y7,y11,y15);
		CHACHA_QUARTERROUND( y0, y5,y10,y15);
		CHACHA_QUARTERROUND( y1, y6,y11,y12);
		CHACHA_QUARTERROUND( y2, y7, y8,y13);
		CHACHA_QUARTERROUND( y3, y4, y9,y14);
	}

	le32enc(out + 0, y0);
	le32enc(out + 4, y1);
	le32enc(out + 8, y2);
	le32enc(out + 12, y3);
	le32enc(out + 16, y12);
	le32enc(out + 20, y13);
	le32enc(out + 24, y14);
	le32enc(out + 28, y15);
}

/* XChaCha stream cipher */

/* https://tools.ietf.org/html/draft-irtf-cfrg-xchacha-03 */

static void
xchacha_stream_ref(uint8_t *restrict s, size_t nbytes, uint32_t blkno,
    const uint8_t nonce[static 24], const uint8_t k[static 32], unsigned nr)
{
	uint8_t subkey[32];
	uint8_t subnonce[12];

	hchacha_ref(subkey, nonce/*[0:16)*/, k, chacha_const32, nr);
	memset(subnonce, 0, 4);
	memcpy(subnonce + 4, nonce + 16, 8);
	chacha_stream_ref(s, nbytes, blkno, subnonce, subkey, nr);
}

static void
xchacha_stream_xor_ref(uint8_t *restrict c, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t k[static 32],
    unsigned nr)
{
	uint8_t subkey[32];
	uint8_t subnonce[12];

	hchacha_ref(subkey, nonce/*[0:16)*/, k, chacha_const32, nr);
	memset(subnonce, 0, 4);
	memcpy(subnonce + 4, nonce + 16, 8);
	chacha_stream_xor_ref(c, p, nbytes, blkno, subnonce, subkey, nr);
}

static int
chacha_probe_ref(void)
{

	/* The reference implementation is always available.  */
	return 0;
}

const struct chacha_impl chacha_ref_impl = {
	.ci_name = "Portable C ChaCha",
	.ci_probe = chacha_probe_ref,
	.ci_chacha_core = chacha_core_ref,
	.ci_hchacha = hchacha_ref,
	.ci_chacha_stream = chacha_stream_ref,
	.ci_chacha_stream_xor = chacha_stream_xor_ref,
	.ci_xchacha_stream = xchacha_stream_ref,
	.ci_xchacha_stream_xor = xchacha_stream_xor_ref,
};
