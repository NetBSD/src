/*	$NetBSD: aes_neon.c,v 1.5 2020/08/08 14:47:01 riastradh Exp $	*/

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
 * Permutation-based AES using NEON, derived from Mike Hamburg's VPAES
 * software, at <https://crypto.stanford.edu/vpaes/>, described in
 *
 *	Mike Hamburg, `Accelerating AES with Vector Permute
 *	Instructions', in Christophe Clavier and Kris Gaj (eds.),
 *	Cryptographic Hardware and Embedded Systems -- CHES 2009,
 *	Springer LNCS 5747, pp. 18-32.
 *
 *	https://link.springer.com/chapter/10.1007/978-3-642-04138-9_2
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_neon.c,v 1.5 2020/08/08 14:47:01 riastradh Exp $");

#include <sys/types.h>

#ifdef _KERNEL
#include <sys/systm.h>
#else
#include <err.h>
#define	panic(fmt, args...)		err(1, fmt, ##args)
#endif

#include "aes_neon_impl.h"

#ifdef __aarch64__
#define	__aarch64_used
#else
#define	__aarch64_used	__unused
#endif

static const uint8x16_t
mc_forward[4] = {
	VQ_N_U8(0x01,0x02,0x03,0x00,0x05,0x06,0x07,0x04,
	    0x09,0x0A,0x0B,0x08,0x0D,0x0E,0x0F,0x0C),
	VQ_N_U8(0x05,0x06,0x07,0x04,0x09,0x0A,0x0B,0x08,
	    0x0D,0x0E,0x0F,0x0C,0x01,0x02,0x03,0x00),
	VQ_N_U8(0x09,0x0A,0x0B,0x08,0x0D,0x0E,0x0F,0x0C,
	    0x01,0x02,0x03,0x00,0x05,0x06,0x07,0x04),
	VQ_N_U8(0x0D,0x0E,0x0F,0x0C,0x01,0x02,0x03,0x00,
	    0x05,0x06,0x07,0x04,0x09,0x0A,0x0B,0x08),
},
mc_backward[4] __aarch64_used = {
	VQ_N_U8(0x03,0x00,0x01,0x02,0x07,0x04,0x05,0x06,
	    0x0B,0x08,0x09,0x0A,0x0F,0x0C,0x0D,0x0E),
	VQ_N_U8(0x0F,0x0C,0x0D,0x0E,0x03,0x00,0x01,0x02,
	    0x07,0x04,0x05,0x06,0x0B,0x08,0x09,0x0A),
	VQ_N_U8(0x0B,0x08,0x09,0x0A,0x0F,0x0C,0x0D,0x0E,
	    0x03,0x00,0x01,0x02,0x07,0x04,0x05,0x06),
	VQ_N_U8(0x07,0x04,0x05,0x06,0x0B,0x08,0x09,0x0A,
	    0x0F,0x0C,0x0D,0x0E,0x03,0x00,0x01,0x02),
},
ipt[2] __aarch64_used = {
	VQ_N_U8(0x00,0x70,0x2A,0x5A,0x98,0xE8,0xB2,0xC2,
	    0x08,0x78,0x22,0x52,0x90,0xE0,0xBA,0xCA),
	VQ_N_U8(0x00,0x4D,0x7C,0x31,0x7D,0x30,0x01,0x4C,
	    0x81,0xCC,0xFD,0xB0,0xFC,0xB1,0x80,0xCD),
},
opt[2] = {
	VQ_N_U8(0x00,0x60,0xB6,0xD6,0x29,0x49,0x9F,0xFF,
	    0x08,0x68,0xBE,0xDE,0x21,0x41,0x97,0xF7),
	VQ_N_U8(0x00,0xEC,0xBC,0x50,0x51,0xBD,0xED,0x01,
	    0xE0,0x0C,0x5C,0xB0,0xB1,0x5D,0x0D,0xE1),
},
dipt[2] __aarch64_used = {
	VQ_N_U8(0x00,0x5F,0x54,0x0B,0x04,0x5B,0x50,0x0F,
	    0x1A,0x45,0x4E,0x11,0x1E,0x41,0x4A,0x15),
	VQ_N_U8(0x00,0x65,0x05,0x60,0xE6,0x83,0xE3,0x86,
	    0x94,0xF1,0x91,0xF4,0x72,0x17,0x77,0x12),
},
sb1[2] __aarch64_used = {
	VQ_N_U8(0x00,0x3E,0x50,0xCB,0x8F,0xE1,0x9B,0xB1,
	    0x44,0xF5,0x2A,0x14,0x6E,0x7A,0xDF,0xA5),
	VQ_N_U8(0x00,0x23,0xE2,0xFA,0x15,0xD4,0x18,0x36,
	    0xEF,0xD9,0x2E,0x0D,0xC1,0xCC,0xF7,0x3B),
},
sb2[2] __aarch64_used = {
	VQ_N_U8(0x00,0x24,0x71,0x0B,0xC6,0x93,0x7A,0xE2,
	    0xCD,0x2F,0x98,0xBC,0x55,0xE9,0xB7,0x5E),
	VQ_N_U8(0x00,0x29,0xE1,0x0A,0x40,0x88,0xEB,0x69,
	    0x4A,0x23,0x82,0xAB,0xC8,0x63,0xA1,0xC2),
},
sbo[2] __aarch64_used = {
	VQ_N_U8(0x00,0xC7,0xBD,0x6F,0x17,0x6D,0xD2,0xD0,
	    0x78,0xA8,0x02,0xC5,0x7A,0xBF,0xAA,0x15),
	VQ_N_U8(0x00,0x6A,0xBB,0x5F,0xA5,0x74,0xE4,0xCF,
	    0xFA,0x35,0x2B,0x41,0xD1,0x90,0x1E,0x8E),
},
dsb9[2] __aarch64_used = {
	VQ_N_U8(0x00,0xD6,0x86,0x9A,0x53,0x03,0x1C,0x85,
	    0xC9,0x4C,0x99,0x4F,0x50,0x1F,0xD5,0xCA),
	VQ_N_U8(0x00,0x49,0xD7,0xEC,0x89,0x17,0x3B,0xC0,
	    0x65,0xA5,0xFB,0xB2,0x9E,0x2C,0x5E,0x72),
},
dsbd[2] __aarch64_used = {
	VQ_N_U8(0x00,0xA2,0xB1,0xE6,0xDF,0xCC,0x57,0x7D,
	    0x39,0x44,0x2A,0x88,0x13,0x9B,0x6E,0xF5),
	VQ_N_U8(0x00,0xCB,0xC6,0x24,0xF7,0xFA,0xE2,0x3C,
	    0xD3,0xEF,0xDE,0x15,0x0D,0x18,0x31,0x29),
},
dsbb[2] __aarch64_used = {
	VQ_N_U8(0x00,0x42,0xB4,0x96,0x92,0x64,0x22,0xD0,
	    0x04,0xD4,0xF2,0xB0,0xF6,0x46,0x26,0x60),
	VQ_N_U8(0x00,0x67,0x59,0xCD,0xA6,0x98,0x94,0xC1,
	    0x6B,0xAA,0x55,0x32,0x3E,0x0C,0xFF,0xF3),
},
dsbe[2] __aarch64_used = {
	VQ_N_U8(0x00,0xD0,0xD4,0x26,0x96,0x92,0xF2,0x46,
	    0xB0,0xF6,0xB4,0x64,0x04,0x60,0x42,0x22),
	VQ_N_U8(0x00,0xC1,0xAA,0xFF,0xCD,0xA6,0x55,0x0C,
	    0x32,0x3E,0x59,0x98,0x6B,0xF3,0x67,0x94),
},
dsbo[2] __aarch64_used = {
	VQ_N_U8(0x00,0x40,0xF9,0x7E,0x53,0xEA,0x87,0x13,
	    0x2D,0x3E,0x94,0xD4,0xB9,0x6D,0xAA,0xC7),
	VQ_N_U8(0x00,0x1D,0x44,0x93,0x0F,0x56,0xD7,0x12,
	    0x9C,0x8E,0xC5,0xD8,0x59,0x81,0x4B,0xCA),
},
dks1[2] = {
	VQ_N_U8(0x00,0xA7,0xD9,0x7E,0xC8,0x6F,0x11,0xB6,
	    0xFC,0x5B,0x25,0x82,0x34,0x93,0xED,0x4A),
	VQ_N_U8(0x00,0x33,0x14,0x27,0x62,0x51,0x76,0x45,
	    0xCE,0xFD,0xDA,0xE9,0xAC,0x9F,0xB8,0x8B),
},
dks2[2] = {
	VQ_N_U8(0x00,0x64,0xA8,0xCC,0xEB,0x8F,0x43,0x27,
	    0x61,0x05,0xC9,0xAD,0x8A,0xEE,0x22,0x46),
	VQ_N_U8(0x00,0xDD,0x92,0x4F,0xCE,0x13,0x5C,0x81,
	    0xF2,0x2F,0x60,0xBD,0x3C,0xE1,0xAE,0x73),
},
dks3[2] = {
	VQ_N_U8(0x00,0xC7,0xC6,0x01,0x02,0xC5,0xC4,0x03,
	    0xFB,0x3C,0x3D,0xFA,0xF9,0x3E,0x3F,0xF8),
	VQ_N_U8(0x00,0xF7,0xCF,0x38,0xD6,0x21,0x19,0xEE,
	    0x4B,0xBC,0x84,0x73,0x9D,0x6A,0x52,0xA5),
},
dks4[2] = {
	VQ_N_U8(0x00,0x20,0x73,0x53,0xB0,0x90,0xC3,0xE3,
	    0x43,0x63,0x30,0x10,0xF3,0xD3,0x80,0xA0),
	VQ_N_U8(0xE8,0x82,0x69,0x03,0x4B,0x21,0xCA,0xA0,
	    0x67,0x0D,0xE6,0x8C,0xC4,0xAE,0x45,0x2F),
},
deskew[2] = {
	VQ_N_U8(0x00,0xE3,0xA4,0x47,0x40,0xA3,0xE4,0x07,
	    0x1A,0xF9,0xBE,0x5D,0x5A,0xB9,0xFE,0x1D),
	VQ_N_U8(0x00,0x69,0xEA,0x83,0xDC,0xB5,0x36,0x5F,
	    0x77,0x1E,0x9D,0xF4,0xAB,0xC2,0x41,0x28),
},
sr[4] __aarch64_used = {
	VQ_N_U8(0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
	    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F),
	VQ_N_U8(0x00,0x05,0x0A,0x0F,0x04,0x09,0x0E,0x03,
	    0x08,0x0D,0x02,0x07,0x0C,0x01,0x06,0x0B),
	VQ_N_U8(0x00,0x09,0x02,0x0B,0x04,0x0D,0x06,0x0F,
	    0x08,0x01,0x0A,0x03,0x0C,0x05,0x0E,0x07),
	VQ_N_U8(0x00,0x0D,0x0A,0x07,0x04,0x01,0x0E,0x0B,
	    0x08,0x05,0x02,0x0F,0x0C,0x09,0x06,0x03),
},
rcon	= VQ_N_U8(0xB6,0xEE,0x9D,0xAF,0xB9,0x91,0x83,0x1F,
	    0x81,0x7D,0x7C,0x4D,0x08,0x98,0x2A,0x70),
of	= VQ_N_U8(0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
	    0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F),
s63	= VQ_N_U8(0x5B,0x5B,0x5B,0x5B,0x5B,0x5B,0x5B,0x5B,
	    0x5B,0x5B,0x5B,0x5B,0x5B,0x5B,0x5B,0x5B),
inv	= VQ_N_U8(0x80,0x01,0x08,0x0D,0x0F,0x06,0x05,0x0E,
	    0x02,0x0C,0x0B,0x0A,0x09,0x03,0x07,0x04),
inva	= VQ_N_U8(0x80,0x07,0x0B,0x0F,0x06,0x0A,0x04,0x01,
	    0x09,0x08,0x05,0x02,0x0C,0x0E,0x0D,0x03);

static inline uint8x16_t
loadroundkey(const void *rkp)
{
	return vld1q_u8(rkp);
}

static inline void
storeroundkey(void *rkp, uint8x16_t rk)
{
	vst1q_u8(rkp, rk);
}

/* Given abcdefgh, set *lo = 0b0d0f0h and *hi = 0a0c0e0g.  */
static inline void
bytes2nybbles(uint8x16_t *restrict lo, uint8x16_t *restrict hi, uint8x16_t x)
{

	*lo = of & x;
	*hi = of & vshrq_n_u8(x, 4);
}

/*
 * t is a pair of maps respectively from low and high nybbles to bytes.
 * Apply t the nybbles, and add the results in GF(2).
 */
static uint8x16_t
aes_schedule_transform(uint8x16_t x, const uint8x16_t t[static 2])
{
	uint8x16_t lo, hi;

	bytes2nybbles(&lo, &hi, x);
	return vqtbl1q_u8(t[0], lo) ^ vqtbl1q_u8(t[1], hi);
}

static inline void
subbytes(uint8x16_t *io, uint8x16_t *jo, uint8x16_t x, uint8x16_t inv_,
    uint8x16_t inva_)
{
	uint8x16_t k, i, ak, j;

	bytes2nybbles(&k, &i, x);
	ak = vqtbl1q_u8(inva_, k);
	j = i ^ k;
	*io = j ^ vqtbl1q_u8(inv_, ak ^ vqtbl1q_u8(inv_, i));
	*jo = i ^ vqtbl1q_u8(inv_, ak ^ vqtbl1q_u8(inv_, j));
}

static uint8x16_t
aes_schedule_low_round(uint8x16_t rk, uint8x16_t prk)
{
	uint8x16_t io, jo;

	/* smear prk */
	prk ^= vextq_u8(vdupq_n_u8(0), prk, 12);
	prk ^= vextq_u8(vdupq_n_u8(0), prk, 8);
	prk ^= s63;

	/* subbytes */
	subbytes(&io, &jo, rk, inv, inva);
	rk = vqtbl1q_u8(sb1[0], io) ^ vqtbl1q_u8(sb1[1], jo);

	/* add in smeared stuff */
	return rk ^ prk;
}

static uint8x16_t
aes_schedule_round(uint8x16_t rk, uint8x16_t prk, uint8x16_t *rcon_rot)
{
	uint32x4_t rk32;

	/* extract rcon from rcon_rot */
	prk ^= vextq_u8(*rcon_rot, vdupq_n_u8(0), 15);
	*rcon_rot = vextq_u8(*rcon_rot, *rcon_rot, 15);

	/* rotate */
	rk32 = vreinterpretq_u32_u8(rk);
	rk32 = vdupq_n_u32(vgetq_lane_u32(rk32, 3));
	rk = vreinterpretq_u8_u32(rk32);
	rk = vextq_u8(rk, rk, 1);

	return aes_schedule_low_round(rk, prk);
}

static uint8x16_t
aes_schedule_mangle_enc(uint8x16_t x, uint8x16_t sr_i)
{
	uint8x16_t y = vdupq_n_u8(0);

	x ^= s63;

	x = vqtbl1q_u8(x, mc_forward[0]);
	y ^= x;
	x = vqtbl1q_u8(x, mc_forward[0]);
	y ^= x;
	x = vqtbl1q_u8(x, mc_forward[0]);
	y ^= x;

	return vqtbl1q_u8(y, sr_i);
}

static uint8x16_t
aes_schedule_mangle_last_enc(uint8x16_t x, uint8x16_t sr_i)
{

	return aes_schedule_transform(vqtbl1q_u8(x, sr_i) ^ s63, opt);
}

static uint8x16_t
aes_schedule_mangle_dec(uint8x16_t x, uint8x16_t sr_i)
{
	uint8x16_t y = vdupq_n_u8(0);

	x = aes_schedule_transform(x, dks1);
	y = vqtbl1q_u8(y ^ x, mc_forward[0]);
	x = aes_schedule_transform(x, dks2);
	y = vqtbl1q_u8(y ^ x, mc_forward[0]);
	x = aes_schedule_transform(x, dks3);
	y = vqtbl1q_u8(y ^ x, mc_forward[0]);
	x = aes_schedule_transform(x, dks4);
	y = vqtbl1q_u8(y ^ x, mc_forward[0]);

	return vqtbl1q_u8(y, sr_i);
}

static uint8x16_t
aes_schedule_mangle_last_dec(uint8x16_t x)
{

	return aes_schedule_transform(x ^ s63, deskew);
}

static uint8x16_t
aes_schedule_192_smear(uint8x16_t prkhi, uint8x16_t prk)
{
	uint32x4_t prkhi32 = vreinterpretq_u32_u8(prkhi);
	uint32x4_t prk32 = vreinterpretq_u32_u8(prk);
	uint32x4_t rk32;

	rk32 = prkhi32;
	rk32 ^= vsetq_lane_u32(vgetq_lane_u32(prkhi32, 2),
	    vdupq_n_u32(vgetq_lane_u32(prkhi32, 0)),
	    3);
	rk32 ^= vsetq_lane_u32(vgetq_lane_u32(prk32, 2),
	    vdupq_n_u32(vgetq_lane_u32(prk32, 3)),
	    0);

	return vreinterpretq_u8_u32(rk32);
}

static uint8x16_t
aes_schedule_192_smearhi(uint8x16_t rk)
{
	uint64x2_t rk64 = vreinterpretq_u64_u8(rk);

	rk64 = vsetq_lane_u64(0, rk64, 0);

	return vreinterpretq_u8_u64(rk64);
}

void
aes_neon_setenckey(struct aesenc *enc, const uint8_t *key, unsigned nrounds)
{
	uint32_t *rk32 = enc->aese_aes.aes_rk;
	uint8x16_t mrk;		/* mangled round key */
	uint8x16_t rk;		/* round key */
	uint8x16_t prk;		/* previous round key */
	uint8x16_t rcon_rot = rcon;
	uint64_t i = 3;

	/* input transform */
	rk = aes_schedule_transform(vld1q_u8(key), ipt);
	storeroundkey(rk32, rk);
	rk32 += 4;

	switch (nrounds) {
	case 10:
		for (;;) {
			rk = aes_schedule_round(rk, rk, &rcon_rot);
			if (--nrounds == 0)
				break;
			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 += 4;
		}
		break;
	case 12: {
		uint8x16_t prkhi;	/* high half of previous round key */

		prk = rk;
		rk = aes_schedule_transform(vld1q_u8(key + 8), ipt);
		prkhi = aes_schedule_192_smearhi(rk);
		for (;;) {
			prk = aes_schedule_round(rk, prk, &rcon_rot);
			rk = vextq_u8(prkhi, prk, 8);

			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);

			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 3) == 0)
				break;

			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);
		}
		break;
	}
	case 14: {
		uint8x16_t pprk;	/* previous previous round key */

		prk = rk;
		rk = aes_schedule_transform(vld1q_u8(key + 16), ipt);
		for (;;) {
			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			pprk = rk;

			/* high round */
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 2) == 0)
				break;
			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 += 4;

			/* low round */
			rk = vreinterpretq_u8_u32(
				vdupq_n_u32(
				    vgetq_lane_u32(vreinterpretq_u32_u8(rk),
					3)));
			rk = aes_schedule_low_round(rk, pprk);
		}
		break;
	}
	default:
		panic("invalid number of AES rounds: %u", nrounds);
	}
	storeroundkey(rk32, aes_schedule_mangle_last_enc(rk, sr[i-- % 4]));
}

void
aes_neon_setdeckey(struct aesdec *dec, const uint8_t *key, unsigned nrounds)
{
	uint32_t *rk32 = dec->aesd_aes.aes_rk;
	uint8x16_t mrk;		/* mangled round key */
	uint8x16_t ork;		/* original round key */
	uint8x16_t rk;		/* round key */
	uint8x16_t prk;		/* previous round key */
	uint8x16_t rcon_rot = rcon;
	unsigned i = nrounds == 12 ? 0 : 2;

	ork = vld1q_u8(key);

	/* input transform */
	rk = aes_schedule_transform(ork, ipt);

	/* go from end */
	rk32 += 4*nrounds;
	storeroundkey(rk32, vqtbl1q_u8(ork, sr[i]));
	rk32 -= 4;
	i ^= 3;

	switch (nrounds) {
	case 10:
		for (;;) {
			rk = aes_schedule_round(rk, rk, &rcon_rot);
			if (--nrounds == 0)
				break;
			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
		}
		break;
	case 12: {
		uint8x16_t prkhi;	/* high half of previous round key */

		prk = rk;
		rk = aes_schedule_transform(vld1q_u8(key + 8), ipt);
		prkhi = aes_schedule_192_smearhi(rk);
		for (;;) {
			prk = aes_schedule_round(rk, prk, &rcon_rot);
			rk = vextq_u8(prkhi, prk, 8);

			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);

			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 3) == 0)
				break;

			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);
		}
		break;
	}
	case 14: {
		uint8x16_t pprk;	/* previous previous round key */

		prk = rk;
		rk = aes_schedule_transform(vld1q_u8(key + 16), ipt);
		for (;;) {
			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			pprk = rk;

			/* high round */
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 2) == 0)
				break;
			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4]);
			storeroundkey(rk32, mrk);
			rk32 -= 4;

			/* low round */
			rk = vreinterpretq_u8_u32(
				vdupq_n_u32(
				    vgetq_lane_u32(vreinterpretq_u32_u8(rk),
					3)));
			rk = aes_schedule_low_round(rk, pprk);
		}
		break;
	}
	default:
		panic("invalid number of AES rounds: %u", nrounds);
	}
	storeroundkey(rk32, aes_schedule_mangle_last_dec(rk));
}

#ifdef __aarch64__

/*
 * GCC does a lousy job of compiling NEON intrinsics for arm32, so we
 * do the performance-critical parts -- encryption and decryption -- in
 * hand-written assembly on arm32.
 */

uint8x16_t
aes_neon_enc1(const struct aesenc *enc, uint8x16_t x, unsigned nrounds)
{
	const uint32_t *rk32 = enc->aese_aes.aes_rk;
	uint8x16_t inv_ = *(const volatile uint8x16_t *)&inv;
	uint8x16_t inva_ = *(const volatile uint8x16_t *)&inva;
	uint8x16_t sb1_0 = ((const volatile uint8x16_t *)sb1)[0];
	uint8x16_t sb1_1 = ((const volatile uint8x16_t *)sb1)[1];
	uint8x16_t sb2_0 = ((const volatile uint8x16_t *)sb2)[0];
	uint8x16_t sb2_1 = ((const volatile uint8x16_t *)sb2)[1];
	uint8x16_t io, jo;
	unsigned rmod4 = 0;

	x = aes_schedule_transform(x, ipt);
	x ^= loadroundkey(rk32);
	for (;;) {
		uint8x16_t A, A2, A2_B, A2_B_D;

		subbytes(&io, &jo, x, inv_, inva_);

		rk32 += 4;
		rmod4 = (rmod4 + 1) % 4;
		if (--nrounds == 0)
			break;

		A = vqtbl1q_u8(sb1_0, io) ^ vqtbl1q_u8(sb1_1, jo);
		A ^= loadroundkey(rk32);
		A2 = vqtbl1q_u8(sb2_0, io) ^ vqtbl1q_u8(sb2_1, jo);
		A2_B = A2 ^ vqtbl1q_u8(A, mc_forward[rmod4]);
		A2_B_D = A2_B ^ vqtbl1q_u8(A, mc_backward[rmod4]);
		x = A2_B_D ^ vqtbl1q_u8(A2_B, mc_forward[rmod4]);
	}
	x = vqtbl1q_u8(sbo[0], io) ^ vqtbl1q_u8(sbo[1], jo);
	x ^= loadroundkey(rk32);
	return vqtbl1q_u8(x, sr[rmod4]);
}

uint8x16x2_t
aes_neon_enc2(const struct aesenc *enc, uint8x16x2_t x, unsigned nrounds)
{
	const uint32_t *rk32 = enc->aese_aes.aes_rk;
	uint8x16_t inv_ = *(const volatile uint8x16_t *)&inv;
	uint8x16_t inva_ = *(const volatile uint8x16_t *)&inva;
	uint8x16_t sb1_0 = ((const volatile uint8x16_t *)sb1)[0];
	uint8x16_t sb1_1 = ((const volatile uint8x16_t *)sb1)[1];
	uint8x16_t sb2_0 = ((const volatile uint8x16_t *)sb2)[0];
	uint8x16_t sb2_1 = ((const volatile uint8x16_t *)sb2)[1];
	uint8x16_t x0 = x.val[0], x1 = x.val[1];
	uint8x16_t io0, jo0, io1, jo1;
	unsigned rmod4 = 0;

	x0 = aes_schedule_transform(x0, ipt);
	x1 = aes_schedule_transform(x1, ipt);
	x0 ^= loadroundkey(rk32);
	x1 ^= loadroundkey(rk32);
	for (;;) {
		uint8x16_t A_0, A2_0, A2_B_0, A2_B_D_0;
		uint8x16_t A_1, A2_1, A2_B_1, A2_B_D_1;

		subbytes(&io0, &jo0, x0, inv_, inva_);
		subbytes(&io1, &jo1, x1, inv_, inva_);

		rk32 += 4;
		rmod4 = (rmod4 + 1) % 4;
		if (--nrounds == 0)
			break;

		A_0 = vqtbl1q_u8(sb1_0, io0) ^ vqtbl1q_u8(sb1_1, jo0);
		A_1 = vqtbl1q_u8(sb1_0, io1) ^ vqtbl1q_u8(sb1_1, jo1);
		A_0 ^= loadroundkey(rk32);
		A_1 ^= loadroundkey(rk32);
		A2_0 = vqtbl1q_u8(sb2_0, io0) ^ vqtbl1q_u8(sb2_1, jo0);
		A2_1 = vqtbl1q_u8(sb2_0, io1) ^ vqtbl1q_u8(sb2_1, jo1);
		A2_B_0 = A2_0 ^ vqtbl1q_u8(A_0, mc_forward[rmod4]);
		A2_B_1 = A2_1 ^ vqtbl1q_u8(A_1, mc_forward[rmod4]);
		A2_B_D_0 = A2_B_0 ^ vqtbl1q_u8(A_0, mc_backward[rmod4]);
		A2_B_D_1 = A2_B_1 ^ vqtbl1q_u8(A_1, mc_backward[rmod4]);
		x0 = A2_B_D_0 ^ vqtbl1q_u8(A2_B_0, mc_forward[rmod4]);
		x1 = A2_B_D_1 ^ vqtbl1q_u8(A2_B_1, mc_forward[rmod4]);
	}
	x0 = vqtbl1q_u8(sbo[0], io0) ^ vqtbl1q_u8(sbo[1], jo0);
	x1 = vqtbl1q_u8(sbo[0], io1) ^ vqtbl1q_u8(sbo[1], jo1);
	x0 ^= loadroundkey(rk32);
	x1 ^= loadroundkey(rk32);
	return (uint8x16x2_t) { .val = {
		[0] = vqtbl1q_u8(x0, sr[rmod4]),
		[1] = vqtbl1q_u8(x1, sr[rmod4]),
	} };
}

uint8x16_t
aes_neon_dec1(const struct aesdec *dec, uint8x16_t x, unsigned nrounds)
{
	const uint32_t *rk32 = dec->aesd_aes.aes_rk;
	unsigned i = 3 & ~(nrounds - 1);
	uint8x16_t inv_ = *(const volatile uint8x16_t *)&inv;
	uint8x16_t inva_ = *(const volatile uint8x16_t *)&inva;
	uint8x16_t io, jo, mc;

	x = aes_schedule_transform(x, dipt);
	x ^= loadroundkey(rk32);
	rk32 += 4;

	mc = mc_forward[3];
	for (;;) {
		subbytes(&io, &jo, x, inv_, inva_);
		if (--nrounds == 0)
			break;

		x = vqtbl1q_u8(dsb9[0], io) ^ vqtbl1q_u8(dsb9[1], jo);
		x ^= loadroundkey(rk32);
		rk32 += 4;				/* next round key */

		x = vqtbl1q_u8(x, mc);
		x ^= vqtbl1q_u8(dsbd[0], io) ^ vqtbl1q_u8(dsbd[1], jo);

		x = vqtbl1q_u8(x, mc);
		x ^= vqtbl1q_u8(dsbb[0], io) ^ vqtbl1q_u8(dsbb[1], jo);

		x = vqtbl1q_u8(x, mc);
		x ^= vqtbl1q_u8(dsbe[0], io) ^ vqtbl1q_u8(dsbe[1], jo);

		mc = vextq_u8(mc, mc, 12);
	}
	x = vqtbl1q_u8(dsbo[0], io) ^ vqtbl1q_u8(dsbo[1], jo);
	x ^= loadroundkey(rk32);
	return vqtbl1q_u8(x, sr[i]);
}

uint8x16x2_t
aes_neon_dec2(const struct aesdec *dec, uint8x16x2_t x, unsigned nrounds)
{
	const uint32_t *rk32 = dec->aesd_aes.aes_rk;
	unsigned i = 3 & ~(nrounds - 1);
	uint8x16_t inv_ = *(const volatile uint8x16_t *)&inv;
	uint8x16_t inva_ = *(const volatile uint8x16_t *)&inva;
	uint8x16_t x0 = x.val[0], x1 = x.val[1];
	uint8x16_t io0, jo0, io1, jo1, mc;

	x0 = aes_schedule_transform(x0, dipt);
	x1 = aes_schedule_transform(x1, dipt);
	x0 ^= loadroundkey(rk32);
	x1 ^= loadroundkey(rk32);
	rk32 += 4;

	mc = mc_forward[3];
	for (;;) {
		subbytes(&io0, &jo0, x0, inv_, inva_);
		subbytes(&io1, &jo1, x1, inv_, inva_);
		if (--nrounds == 0)
			break;

		x0 = vqtbl1q_u8(dsb9[0], io0) ^ vqtbl1q_u8(dsb9[1], jo0);
		x1 = vqtbl1q_u8(dsb9[0], io1) ^ vqtbl1q_u8(dsb9[1], jo1);
		x0 ^= loadroundkey(rk32);
		x1 ^= loadroundkey(rk32);
		rk32 += 4;				/* next round key */

		x0 = vqtbl1q_u8(x0, mc);
		x1 = vqtbl1q_u8(x1, mc);
		x0 ^= vqtbl1q_u8(dsbd[0], io0) ^ vqtbl1q_u8(dsbd[1], jo0);
		x1 ^= vqtbl1q_u8(dsbd[0], io1) ^ vqtbl1q_u8(dsbd[1], jo1);

		x0 = vqtbl1q_u8(x0, mc);
		x1 = vqtbl1q_u8(x1, mc);
		x0 ^= vqtbl1q_u8(dsbb[0], io0) ^ vqtbl1q_u8(dsbb[1], jo0);
		x1 ^= vqtbl1q_u8(dsbb[0], io1) ^ vqtbl1q_u8(dsbb[1], jo1);

		x0 = vqtbl1q_u8(x0, mc);
		x1 = vqtbl1q_u8(x1, mc);
		x0 ^= vqtbl1q_u8(dsbe[0], io0) ^ vqtbl1q_u8(dsbe[1], jo0);
		x1 ^= vqtbl1q_u8(dsbe[0], io1) ^ vqtbl1q_u8(dsbe[1], jo1);

		mc = vextq_u8(mc, mc, 12);
	}
	x0 = vqtbl1q_u8(dsbo[0], io0) ^ vqtbl1q_u8(dsbo[1], jo0);
	x1 = vqtbl1q_u8(dsbo[0], io1) ^ vqtbl1q_u8(dsbo[1], jo1);
	x0 ^= loadroundkey(rk32);
	x1 ^= loadroundkey(rk32);
	return (uint8x16x2_t) { .val = {
		[0] = vqtbl1q_u8(x0, sr[i]),
		[1] = vqtbl1q_u8(x1, sr[i]),
	} };
}

#endif
