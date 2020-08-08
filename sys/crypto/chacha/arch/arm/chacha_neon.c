/*	$NetBSD: chacha_neon.c,v 1.8 2020/08/08 14:47:01 riastradh Exp $	*/

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
#include "arm_neon_imm.h"
#include "chacha_neon.h"

/*
 * Tempting to use VSHL/VSRI instead of VSHL/VSHR/VORR, but in practice
 * it hurts performance at least on Cortex-A8.
 */
#if 1
#define	vrolq_n_u32(x, n)	(vshlq_n_u32(x, n) | vshrq_n_u32(x, 32 - (n)))
#else
#define	vrolq_n_u32(x, n)	vsriq_n_u32(vshlq_n_u32(x, n), x, 32 - (n))
#endif

static inline uint32x4_t
rol16(uint32x4_t x)
{
	uint16x8_t y16, x16 = vreinterpretq_u16_u32(x);

	y16 = vrev32q_u16(x16);

	return vreinterpretq_u32_u16(y16);
}

static inline uint32x4_t
rol12(uint32x4_t x)
{

	return vrolq_n_u32(x, 12);
}

static inline uint32x4_t
rol8(uint32x4_t x)
{
#if defined(__aarch64__)
	static const uint8x16_t rol8_tab = VQ_N_U8(
		  3, 0, 1, 2,  7, 4, 5, 6,
		 11, 8, 9,10, 15,12,13,14
	);
	uint8x16_t y8, x8 = vreinterpretq_u8_u32(x);

	y8 = vqtbl1q_u8(x8, rol8_tab);

	return vreinterpretq_u32_u8(y8);
#elif 0
	/*
	 * GCC does a lousy job with this, spilling two 64-bit vector
	 * registers to the stack every time.  There should be plenty
	 * of vector registers free, requiring no spills at all, and
	 * GCC should be able to hoist the load of rol8_tab out of any
	 * loops, but it doesn't and so attempting to use VTBL hurts
	 * more than it helps.
	 */
	static const uint8x8_t rol8_tab = V_N_U8(
		 3, 0, 1, 2,  7, 4, 5, 6
	);

	uint64x2_t y64, x64 = vreinterpretq_u64_u32(x);

	y64 = (uint64x2_t) {
		(uint64_t)vtbl1_u8((uint8x8_t)x64[0], rol8_tab),
		(uint64_t)vtbl1_u8((uint8x8_t)x64[1], rol8_tab),
	};

	return vreinterpretq_u32_u64(y64);
#else
	return vrolq_n_u32(x, 8);
#endif
}

static inline uint32x4_t
rol7(uint32x4_t x)
{

	return vrolq_n_u32(x, 7);
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
		r0 = vaddq_u32(r0, r1); r3 ^= r0; r3 = rol16(r3);
		r2 = vaddq_u32(r2, r3); r1 ^= r2; r1 = rol12(r1);
		r0 = vaddq_u32(r0, r1); r3 ^= r0; r3 = rol8(r3);
		r2 = vaddq_u32(r2, r3); r1 ^= r2; r1 = rol7(r1);

		c0 = r0;
		c1 = vextq_u32(r1, r1, 1);
		c2 = vextq_u32(r2, r2, 2);
		c3 = vextq_u32(r3, r3, 3);

		c0 = vaddq_u32(c0, c1); c3 ^= c0; c3 = rol16(c3);
		c2 = vaddq_u32(c2, c3); c1 ^= c2; c1 = rol12(c1);
		c0 = vaddq_u32(c0, c1); c3 ^= c0; c3 = rol8(c3);
		c2 = vaddq_u32(c2, c3); c1 ^= c2; c1 = rol7(c1);

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

	r0 = in0 = vreinterpretq_u32_u8(vld1q_u8(c));
	r1 = in1 = vreinterpretq_u32_u8(vld1q_u8(k + 0));
	r2 = in2 = vreinterpretq_u32_u8(vld1q_u8(k + 16));
	r3 = in3 = vreinterpretq_u32_u8(vld1q_u8(in));

	chacha_permute(&r0, &r1, &r2, &r3, nr);

	vst1q_u8(out + 0, vreinterpretq_u8_u32(vaddq_u32(r0, in0)));
	vst1q_u8(out + 16, vreinterpretq_u8_u32(vaddq_u32(r1, in1)));
	vst1q_u8(out + 32, vreinterpretq_u8_u32(vaddq_u32(r2, in2)));
	vst1q_u8(out + 48, vreinterpretq_u8_u32(vaddq_u32(r3, in3)));
}

void
hchacha_neon(uint8_t out[restrict static 32],
    const uint8_t in[static 16],
    const uint8_t k[static 32],
    const uint8_t c[static 16],
    unsigned nr)
{
	uint32x4_t r0, r1, r2, r3;

	r0 = vreinterpretq_u32_u8(vld1q_u8(c));
	r1 = vreinterpretq_u32_u8(vld1q_u8(k + 0));
	r2 = vreinterpretq_u32_u8(vld1q_u8(k + 16));
	r3 = vreinterpretq_u32_u8(vld1q_u8(in));

	chacha_permute(&r0, &r1, &r2, &r3, nr);

	vst1q_u8(out + 0, vreinterpretq_u8_u32(r0));
	vst1q_u8(out + 16, vreinterpretq_u8_u32(r3));
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
		const uint32x4_t blkno_inc = /* (1,0,0,0) */
		    vsetq_lane_u32(1, vdupq_n_u32(0), 0);
		uint32x4_t in0, in1, in2, in3;
		uint32x4_t r0, r1, r2, r3;

		in0 = vreinterpretq_u32_u8(vld1q_u8(chacha_const32));
		in1 = vreinterpretq_u32_u8(vld1q_u8(k + 0));
		in2 = vreinterpretq_u32_u8(vld1q_u8(k + 16));
		in3 = (uint32x4_t) VQ_N_U32(
			blkno,
			le32dec(nonce),
			le32dec(nonce + 4),
			le32dec(nonce + 8)
		);

		for (; n; s += 64, n -= 64) {
			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = vaddq_u32(r0, in0);
			r1 = vaddq_u32(r1, in1);
			r2 = vaddq_u32(r2, in2);
			r3 = vaddq_u32(r3, in3);

			if (n < 64) {
				uint8_t buf[64] __aligned(16);

				vst1q_u8(buf + 0, vreinterpretq_u8_u32(r0));
				vst1q_u8(buf + 16, vreinterpretq_u8_u32(r1));
				vst1q_u8(buf + 32, vreinterpretq_u8_u32(r2));
				vst1q_u8(buf + 48, vreinterpretq_u8_u32(r3));
				memcpy(s, buf, n);

				break;
			}

			vst1q_u8(s + 0, vreinterpretq_u8_u32(r0));
			vst1q_u8(s + 16, vreinterpretq_u8_u32(r1));
			vst1q_u8(s + 32, vreinterpretq_u8_u32(r2));
			vst1q_u8(s + 48, vreinterpretq_u8_u32(r3));
			in3 = vaddq_u32(in3, blkno_inc);
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
		const uint32x4_t blkno_inc = /* (1,0,0,0) */
		    vsetq_lane_u32(1, vdupq_n_u32(0), 0);
		uint32x4_t in0, in1, in2, in3;
		uint32x4_t r0, r1, r2, r3;

		in0 = vreinterpretq_u32_u8(vld1q_u8(chacha_const32));
		in1 = vreinterpretq_u32_u8(vld1q_u8(k + 0));
		in2 = vreinterpretq_u32_u8(vld1q_u8(k + 16));
		in3 = (uint32x4_t) VQ_N_U32(
			blkno,
			le32dec(nonce),
			le32dec(nonce + 4),
			le32dec(nonce + 8)
		);

		for (; n; s += 64, p += 64, n -= 64) {
			r0 = in0;
			r1 = in1;
			r2 = in2;
			r3 = in3;
			chacha_permute(&r0, &r1, &r2, &r3, nr);
			r0 = vaddq_u32(r0, in0);
			r1 = vaddq_u32(r1, in1);
			r2 = vaddq_u32(r2, in2);
			r3 = vaddq_u32(r3, in3);

			if (n < 64) {
				uint8_t buf[64] __aligned(16);
				unsigned i;

				vst1q_u8(buf + 0, vreinterpretq_u8_u32(r0));
				vst1q_u8(buf + 16, vreinterpretq_u8_u32(r1));
				vst1q_u8(buf + 32, vreinterpretq_u8_u32(r2));
				vst1q_u8(buf + 48, vreinterpretq_u8_u32(r3));

				for (i = 0; i < n - n%4; i += 4)
					le32enc(s + i,
					    le32dec(p + i) ^ le32dec(buf + i));
				for (; i < n; i++)
					s[i] = p[i] ^ buf[i];

				break;
			}

			r0 ^= vreinterpretq_u32_u8(vld1q_u8(p + 0));
			r1 ^= vreinterpretq_u32_u8(vld1q_u8(p + 16));
			r2 ^= vreinterpretq_u32_u8(vld1q_u8(p + 32));
			r3 ^= vreinterpretq_u32_u8(vld1q_u8(p + 48));
			vst1q_u8(s + 0, vreinterpretq_u8_u32(r0));
			vst1q_u8(s + 16, vreinterpretq_u8_u32(r1));
			vst1q_u8(s + 32, vreinterpretq_u8_u32(r2));
			vst1q_u8(s + 48, vreinterpretq_u8_u32(r3));
			in3 = vaddq_u32(in3, blkno_inc);
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
