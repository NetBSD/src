/*	$NetBSD: chacha_neon.c,v 1.1 2020/07/25 22:51:57 riastradh Exp $	*/

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

#include <sys/types.h>
#include <sys/endian.h>

#include "arm_neon.h"
#include "chacha_neon.h"

static inline uint32x4_t
vrolq_n_u32(uint32x4_t x, uint8_t n)
{

	return vshlq_n_u32(x, n) | vshrq_n_u32(x, 32 - n);
}

static inline uint32x4_t
vhtole_u32(uint32x4_t x)
{
#if _BYTE_ORDER == _LITTLE_ENDIAN
	return x;
#elif _BYTE_ORDER == _BIG_ENDIAN
	return vrev32q_u8(x);
#endif
}

static inline uint32x4_t
vletoh_u32(uint32x4_t x)
{
#if _BYTE_ORDER == _LITTLE_ENDIAN
	return x;
#elif _BYTE_ORDER == _BIG_ENDIAN
	return vrev32q_u8(x);
#endif
}

static inline void
chacha_permute(uint32x4_t *p0, uint32x4_t *p1, uint32x4_t *p2, uint32x4_t *p3,
    unsigned nr)
{
	uint32x4_t r0, r1, r2, r3;
	uint32x4_t c0, c1, c2, c3;

	r0 = *p0;
	r1 = *p1;
	r2 = *p2;
	r3 = *p3;

	for (; nr > 0; nr -= 2) {
		r0 = vaddq_u32(r0, r1); r3 ^= r0; r3 = vrolq_n_u32(r3, 16);
		r2 = vaddq_u32(r2, r3); r1 ^= r2; r1 = vrolq_n_u32(r1, 12);
		r0 = vaddq_u32(r0, r1); r3 ^= r0; r3 = vrolq_n_u32(r3, 8);
		r2 = vaddq_u32(r2, r3); r1 ^= r2; r1 = vrolq_n_u32(r1, 7);

		c0 = r0;
		c1 = vextq_u32(r1, r1, 1);
		c2 = vextq_u32(r2, r2, 2);
		c3 = vextq_u32(r3, r3, 3);

		c0 = vaddq_u32(c0, c1); c3 ^= c0; c3 = vrolq_n_u32(c3, 16);
		c2 = vaddq_u32(c2, c3); c1 ^= c2; c1 = vrolq_n_u32(c1, 12);
		c0 = vaddq_u32(c0, c1); c3 ^= c0; c3 = vrolq_n_u32(c3, 8);
		c2 = vaddq_u32(c2, c3); c1 ^= c2; c1 = vrolq_n_u32(c1, 7);

		r0 = c0;
		r1 = vextq_u32(c1, c1, 3);
		r2 = vextq_u32(c2, c2, 2);
		r3 = vextq_u32(c3, c3, 1);
	}

	*p0 = r0;
	*p1 = r1;
	*p2 = r2;
	*p3 = r3;
}

void
chacha_core_neon(uint8_t out[restrict static 64],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{
	uint32x4_t in0, in1, in2, in3;
	uint32x4_t r0, r1, r2, r3;

	r0 = in0 = vletoh_u32(vld1q_u32((const uint32_t *)c));
	r1 = in1 = vletoh_u32(vld1q_u32((const uint32_t *)k));
	r2 = in2 = vletoh_u32(vld1q_u32((const uint32_t *)k + 4));
	r3 = in3 = vletoh_u32(vld1q_u32((const uint32_t *)in));

	chacha_permute(&r0, &r1, &r2, &r3, nr);

	vst1q_u32((uint32_t *)out + 0, vhtole_u32(vaddq_u32(r0, in0)));
	vst1q_u32((uint32_t *)out + 4, vhtole_u32(vaddq_u32(r1, in1)));
	vst1q_u32((uint32_t *)out + 8, vhtole_u32(vaddq_u32(r2, in2)));
	vst1q_u32((uint32_t *)out + 12, vhtole_u32(vaddq_u32(r3, in3)));
}

void
hchacha_neon(uint8_t out[restrict static 32],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{
	uint32x4_t r0, r1, r2, r3;

	r0 = vletoh_u32(vld1q_u32((const uint32_t *)c));
	r1 = vletoh_u32(vld1q_u32((const uint32_t *)k));
	r2 = vletoh_u32(vld1q_u32((const uint32_t *)k + 4));
	r3 = vletoh_u32(vld1q_u32((const uint32_t *)in));

	chacha_permute(&r0, &r1, &r2, &r3, nr);

	vst1q_u32((uint32_t *)out + 0, r0);
	vst1q_u32((uint32_t *)out + 4, r3);
}

void
chacha_stream_neon(uint8_t *restrict s, size_t n,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t k[static 32],
    unsigned nr)
{

	for (; n >= 256; s += 256, n -= 256, blkno += 4)
		chacha_stream256_neon(s, blkno, nonce, k, chacha_const32, nr);

	if (n) {
		const uint32x4_t blkno_inc = {1,0,0,0};
		uint32x4_t in0, in1, in2, in3;
		uint32x4_t r0, r1, r2, r3;

		in0 = vletoh_u32(vld1q_u32((const uint32_t *)chacha_const32));
		in1 = vletoh_u32(vld1q_u32((const uint32_t *)k));
		in2 = vletoh_u32(vld1q_u32((const uint32_t *)k + 4));
		in3 = (uint32x4_t) {
			blkno,
			le32dec(nonce),
			le32dec(nonce + 4),
			le32dec(nonce + 8)
		};

		for (; n >= 64; s += 64, n -= 64) {
			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = vhtole_u32(vaddq_u32(r0, in0));
			r1 = vhtole_u32(vaddq_u32(r1, in1));
			r2 = vhtole_u32(vaddq_u32(r2, in2));
			r3 = vhtole_u32(vaddq_u32(r3, in3));
			vst1q_u32((uint32_t *)s + 4*0, r0);
			vst1q_u32((uint32_t *)s + 4*1, r1);
			vst1q_u32((uint32_t *)s + 4*2, r2);
			vst1q_u32((uint32_t *)s + 4*3, r3);
			in3 = vaddq_u32(in3, blkno_inc);
		}

		if (n) {
			uint8_t buf[64];

			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = vhtole_u32(vaddq_u32(r0, in0));
			r1 = vhtole_u32(vaddq_u32(r1, in1));
			r2 = vhtole_u32(vaddq_u32(r2, in2));
			r3 = vhtole_u32(vaddq_u32(r3, in3));
			vst1q_u32((uint32_t *)buf + 4*0, r0);
			vst1q_u32((uint32_t *)buf + 4*1, r1);
			vst1q_u32((uint32_t *)buf + 4*2, r2);
			vst1q_u32((uint32_t *)buf + 4*3, r3);

			memcpy(s, buf, n);
		}
	}
}

void
chacha_stream_xor_neon(uint8_t *s, const uint8_t *p, size_t n,
    uint32_t blkno,
    const uint8_t nonce[static 12],
    const uint8_t k[static 32],
    unsigned nr)
{

	for (; n >= 256; s += 256, p += 256, n -= 256, blkno += 4)
		chacha_stream_xor256_neon(s, p, blkno, nonce, k,
		    chacha_const32, nr);

	if (n) {
		const uint32x4_t blkno_inc = {1,0,0,0};
		uint32x4_t in0, in1, in2, in3;
		uint32x4_t r0, r1, r2, r3;

		in0 = vletoh_u32(vld1q_u32((const uint32_t *)chacha_const32));
		in1 = vletoh_u32(vld1q_u32((const uint32_t *)k));
		in2 = vletoh_u32(vld1q_u32((const uint32_t *)k + 4));
		in3 = (uint32x4_t) {
			blkno,
			le32dec(nonce),
			le32dec(nonce + 4),
			le32dec(nonce + 8)
		};

		for (; n >= 64; s += 64, p += 64, n -= 64) {
			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = vhtole_u32(vaddq_u32(r0, in0));
			r1 = vhtole_u32(vaddq_u32(r1, in1));
			r2 = vhtole_u32(vaddq_u32(r2, in2));
			r3 = vhtole_u32(vaddq_u32(r3, in3));
			r0 ^= vld1q_u32((const uint32_t *)p + 4*0);
			r1 ^= vld1q_u32((const uint32_t *)p + 4*1);
			r2 ^= vld1q_u32((const uint32_t *)p + 4*2);
			r3 ^= vld1q_u32((const uint32_t *)p + 4*3);
			vst1q_u32((uint32_t *)s + 4*0, r0);
			vst1q_u32((uint32_t *)s + 4*1, r1);
			vst1q_u32((uint32_t *)s + 4*2, r2);
			vst1q_u32((uint32_t *)s + 4*3, r3);
			in3 = vaddq_u32(in3, blkno_inc);
		}

		if (n) {
			uint8_t buf[64];
			unsigned i;

			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = vhtole_u32(vaddq_u32(r0, in0));
			r1 = vhtole_u32(vaddq_u32(r1, in1));
			r2 = vhtole_u32(vaddq_u32(r2, in2));
			r3 = vhtole_u32(vaddq_u32(r3, in3));
			vst1q_u32((uint32_t *)buf + 4*0, r0);
			vst1q_u32((uint32_t *)buf + 4*1, r1);
			vst1q_u32((uint32_t *)buf + 4*2, r2);
			vst1q_u32((uint32_t *)buf + 4*3, r3);

			for (i = 0; i < n - n%4; i += 4)
				le32enc(s + i,
				    le32dec(p + i) ^ le32dec(buf + i));
			for (; i < n; i++)
				s[i] = p[i] ^ buf[i];
		}
	}
}

void
xchacha_stream_neon(uint8_t *restrict s, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t k[static 32],
    unsigned nr)
{
	uint8_t subkey[32];
	uint8_t subnonce[12];

	hchacha_neon(subkey, nonce/*[0:16)*/, k, chacha_const32, nr);
	memset(subnonce, 0, 4);
	memcpy(subnonce + 4, nonce + 16, 8);
	chacha_stream_neon(s, nbytes, blkno, subnonce, subkey, nr);
}

void
xchacha_stream_xor_neon(uint8_t *restrict c, const uint8_t *p, size_t nbytes,
    uint32_t blkno,
    const uint8_t nonce[static 24],
    const uint8_t k[static 32],
    unsigned nr)
{
	uint8_t subkey[32];
	uint8_t subnonce[12];

	hchacha_neon(subkey, nonce/*[0:16)*/, k, chacha_const32, nr);
	memset(subnonce, 0, 4);
	memcpy(subnonce + 4, nonce + 16, 8);
	chacha_stream_xor_neon(c, p, nbytes, blkno, subnonce, subkey, nr);
}
