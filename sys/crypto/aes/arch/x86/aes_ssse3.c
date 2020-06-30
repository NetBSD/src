/*	$NetBSD: aes_ssse3.c,v 1.2 2020/06/30 20:32:11 riastradh Exp $	*/

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
 * Permutation-based AES using SSSE3, derived from Mike Hamburg's VPAES
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
__KERNEL_RCSID(1, "$NetBSD: aes_ssse3.c,v 1.2 2020/06/30 20:32:11 riastradh Exp $");

#include <sys/types.h>

#ifdef _KERNEL
#include <sys/systm.h>
#else
#include <err.h>
#define	panic(fmt, args...)	err(1, fmt, ##args)
#endif

#include "aes_ssse3_impl.h"

static const union m128const {
	uint64_t u64[2];
	__m128i m;
}
mc_forward[4] = {
	{.u64 = {0x0407060500030201, 0x0C0F0E0D080B0A09}},
	{.u64 = {0x080B0A0904070605, 0x000302010C0F0E0D}},
	{.u64 = {0x0C0F0E0D080B0A09, 0x0407060500030201}},
	{.u64 = {0x000302010C0F0E0D, 0x080B0A0904070605}},
},
mc_backward[4] = {
	{.u64 = {0x0605040702010003, 0x0E0D0C0F0A09080B}},
	{.u64 = {0x020100030E0D0C0F, 0x0A09080B06050407}},
	{.u64 = {0x0E0D0C0F0A09080B, 0x0605040702010003}},
	{.u64 = {0x0A09080B06050407, 0x020100030E0D0C0F}},
},
ipt[2] = {
	{.u64 = {0xC2B2E8985A2A7000, 0xCABAE09052227808}},
	{.u64 = {0x4C01307D317C4D00, 0xCD80B1FCB0FDCC81}},
},
opt[2] = {
	{.u64 = {0xFF9F4929D6B66000, 0xF7974121DEBE6808}},
	{.u64 = {0x01EDBD5150BCEC00, 0xE10D5DB1B05C0CE0}},
},
dipt[2] = {
	{.u64 = {0x0F505B040B545F00, 0x154A411E114E451A}},
	{.u64 = {0x86E383E660056500, 0x12771772F491F194}},
},
sb1[2] = {
	{.u64 = {0xB19BE18FCB503E00, 0xA5DF7A6E142AF544}},
	{.u64 = {0x3618D415FAE22300, 0x3BF7CCC10D2ED9EF}},
},
sb2[2] = {
	{.u64 = {0xE27A93C60B712400, 0x5EB7E955BC982FCD}},
	{.u64 = {0x69EB88400AE12900, 0xC2A163C8AB82234A}},
},
sbo[2] = {
	{.u64 = {0xD0D26D176FBDC700, 0x15AABF7AC502A878}},
	{.u64 = {0xCFE474A55FBB6A00, 0x8E1E90D1412B35FA}},
},
dsb9[2] = {
	{.u64 = {0x851C03539A86D600, 0xCAD51F504F994CC9}},
	{.u64 = {0xC03B1789ECD74900, 0x725E2C9EB2FBA565}},
},
dsbd[2] = {
	{.u64 = {0x7D57CCDFE6B1A200, 0xF56E9B13882A4439}},
	{.u64 = {0x3CE2FAF724C6CB00, 0x2931180D15DEEFD3}},
},
dsbb[2] = {
	{.u64 = {0xD022649296B44200, 0x602646F6B0F2D404}},
	{.u64 = {0xC19498A6CD596700, 0xF3FF0C3E3255AA6B}},
},
dsbe[2] = {
	{.u64 = {0x46F2929626D4D000, 0x2242600464B4F6B0}},
	{.u64 = {0x0C55A6CDFFAAC100, 0x9467F36B98593E32}},
},
dsbo[2] = {
	{.u64 = {0x1387EA537EF94000, 0xC7AA6DB9D4943E2D}},
	{.u64 = {0x12D7560F93441D00, 0xCA4B8159D8C58E9C}},
},
dks1[2] = {
	{.u64 = {0xB6116FC87ED9A700, 0x4AED933482255BFC}},
	{.u64 = {0x4576516227143300, 0x8BB89FACE9DAFDCE}},
},
dks2[2] = {
	{.u64 = {0x27438FEBCCA86400, 0x4622EE8AADC90561}},
	{.u64 = {0x815C13CE4F92DD00, 0x73AEE13CBD602FF2}},
},
dks3[2] = {
	{.u64 = {0x03C4C50201C6C700, 0xF83F3EF9FA3D3CFB}},
	{.u64 = {0xEE1921D638CFF700, 0xA5526A9D7384BC4B}},
},
dks4[2] = {
	{.u64 = {0xE3C390B053732000, 0xA080D3F310306343}},
	{.u64 = {0xA0CA214B036982E8, 0x2F45AEC48CE60D67}},
},
deskew[2] = {
	{.u64 = {0x07E4A34047A4E300, 0x1DFEB95A5DBEF91A}},
	{.u64 = {0x5F36B5DC83EA6900, 0x2841C2ABF49D1E77}},
},
sr[4] = {
	{.u64 = {0x0706050403020100, 0x0F0E0D0C0B0A0908}},
	{.u64 = {0x030E09040F0A0500, 0x0B06010C07020D08}},
	{.u64 = {0x0F060D040B020900, 0x070E050C030A0108}},
	{.u64 = {0x0B0E0104070A0D00, 0x0306090C0F020508}},
},
rcon =	{.u64 = {0x1F8391B9AF9DEEB6, 0x702A98084D7C7D81}},
s63 =	{.u64 = {0x5B5B5B5B5B5B5B5B, 0x5B5B5B5B5B5B5B5B}},
of =	{.u64 = {0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F}},
inv =	{.u64 = {0x0E05060F0D080180, 0x040703090A0B0C02}},
inva =	{.u64 = {0x01040A060F0B0780, 0x030D0E0C02050809}};

static inline __m128i
loadroundkey(const uint32_t *rk32)
{
	return _mm_load_si128((const void *)rk32);
}

static inline void
storeroundkey(uint32_t *rk32, __m128i rk)
{
	_mm_store_si128((void *)rk32, rk);
}

/* Given abcdefgh, set *lo = 0b0d0f0h and *hi = 0a0c0e0g.  */
static inline void
bytes2nybbles(__m128i *restrict lo, __m128i *restrict hi, __m128i x)
{

	*lo = x & of.m;
	*hi = _mm_srli_epi32(x & ~of.m, 4);
}

/* Given 0p0q0r0s, return 0x0y0z0w where x = a/p, y = a/q, &c.  */
static inline __m128i
gf16_inva(__m128i x)
{
	return _mm_shuffle_epi8(inva.m, x);
}

/* Given 0p0q0r0s, return 0x0y0z0w where x = 1/p, y = 1/q, &c.  */
static inline __m128i
gf16_inv(__m128i x)
{
	return _mm_shuffle_epi8(inv.m, x);
}

/*
 * t is a pair of maps respectively from low and high nybbles to bytes.
 * Apply t the nybbles, and add the results in GF(2).
 */
static __m128i
aes_schedule_transform(__m128i x, const union m128const t[static 2])
{
	__m128i lo, hi;

	bytes2nybbles(&lo, &hi, x);
	return _mm_shuffle_epi8(t[0].m, lo) ^ _mm_shuffle_epi8(t[1].m, hi);
}

static inline void
subbytes(__m128i *io, __m128i *jo, __m128i x)
{
	__m128i k, i, ak, j;

	bytes2nybbles(&k, &i, x);
	ak = gf16_inva(k);
	j = i ^ k;
	*io = j ^ gf16_inv(ak ^ gf16_inv(i));
	*jo = i ^ gf16_inv(ak ^ gf16_inv(j));
}

static __m128i
aes_schedule_low_round(__m128i rk, __m128i prk)
{
	__m128i io, jo;

	/* smear prk */
	prk ^= _mm_slli_si128(prk, 4);
	prk ^= _mm_slli_si128(prk, 8);
	prk ^= s63.m;

	/* subbytes */
	subbytes(&io, &jo, rk);
	rk = _mm_shuffle_epi8(sb1[0].m, io) ^ _mm_shuffle_epi8(sb1[1].m, jo);

	/* add in smeared stuff */
	return rk ^ prk;
}

static __m128i
aes_schedule_round(__m128i rk, __m128i prk, __m128i *rcon_rot)
{

	/* extract rcon from rcon_rot */
	prk ^= _mm_alignr_epi8(_mm_setzero_si128(), *rcon_rot, 15);
	*rcon_rot = _mm_alignr_epi8(*rcon_rot, *rcon_rot, 15);

	/* rotate */
	rk = _mm_shuffle_epi32(rk, 0xff);
	rk = _mm_alignr_epi8(rk, rk, 1);

	return aes_schedule_low_round(rk, prk);
}

static __m128i
aes_schedule_mangle_enc(__m128i x, __m128i sr_i)
{
	__m128i y = _mm_setzero_si128();

	x ^= s63.m;

	x = _mm_shuffle_epi8(x, mc_forward[0].m);
	y ^= x;
	x = _mm_shuffle_epi8(x, mc_forward[0].m);
	y ^= x;
	x = _mm_shuffle_epi8(x, mc_forward[0].m);
	y ^= x;

	return _mm_shuffle_epi8(y, sr_i);
}

static __m128i
aes_schedule_mangle_last_enc(__m128i x, __m128i sr_i)
{

	return aes_schedule_transform(_mm_shuffle_epi8(x, sr_i) ^ s63.m, opt);
}

static __m128i
aes_schedule_mangle_dec(__m128i x, __m128i sr_i)
{
	__m128i y = _mm_setzero_si128();

	x = aes_schedule_transform(x, dks1);
	y = _mm_shuffle_epi8(y ^ x, mc_forward[0].m);
	x = aes_schedule_transform(x, dks2);
	y = _mm_shuffle_epi8(y ^ x, mc_forward[0].m);
	x = aes_schedule_transform(x, dks3);
	y = _mm_shuffle_epi8(y ^ x, mc_forward[0].m);
	x = aes_schedule_transform(x, dks4);
	y = _mm_shuffle_epi8(y ^ x, mc_forward[0].m);

	return _mm_shuffle_epi8(y, sr_i);
}

static __m128i
aes_schedule_mangle_last_dec(__m128i x)
{

	return aes_schedule_transform(x ^ s63.m, deskew);
}

static __m128i
aes_schedule_192_smear(__m128i prkhi, __m128i prk)
{
	__m128i rk;

	rk = prkhi;
	rk ^= _mm_shuffle_epi32(prkhi, 0x80);
	rk ^= _mm_shuffle_epi32(prk, 0xfe);

	return rk;
}

static __m128i
aes_schedule_192_smearhi(__m128i rk)
{
	return (__m128i)_mm_movehl_ps((__m128)rk, _mm_setzero_ps());
}

void
aes_ssse3_setenckey(struct aesenc *enc, const uint8_t *key, unsigned nrounds)
{
	uint32_t *rk32 = enc->aese_aes.aes_rk;
	__m128i mrk;		/* mangled round key */
	__m128i rk;		/* round key */
	__m128i prk;		/* previous round key */
	__m128i rcon_rot = rcon.m;
	uint64_t i = 3;

	/* input transform */
	rk = aes_schedule_transform(_mm_loadu_epi8(key), ipt);
	storeroundkey(rk32, rk);
	rk32 += 4;

	switch (nrounds) {
	case 10:
		for (;;) {
			rk = aes_schedule_round(rk, rk, &rcon_rot);
			if (--nrounds == 0)
				break;
			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 += 4;
		}
		break;
	case 12: {
		__m128i prkhi;		/* high half of previous round key */

		prk = rk;
		rk = aes_schedule_transform(_mm_loadu_epi8(key + 8), ipt);
		prkhi = aes_schedule_192_smearhi(rk);
		for (;;) {
			prk = aes_schedule_round(rk, prk, &rcon_rot);
			rk = _mm_alignr_epi8(prk, prkhi, 8);

			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);

			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 3) == 0)
				break;

			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);
		}
		break;
	}
	case 14: {
		__m128i pprk;		/* previous previous round key */

		prk = rk;
		rk = aes_schedule_transform(_mm_loadu_epi8(key + 16), ipt);
		for (;;) {
			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 += 4;
			pprk = rk;

			/* high round */
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 2) == 0)
				break;
			mrk = aes_schedule_mangle_enc(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 += 4;

			/* low round */
			rk = _mm_shuffle_epi32(rk, 0xff);
			rk = aes_schedule_low_round(rk, pprk);
		}
		break;
	}
	default:
		panic("invalid number of AES rounds: %u", nrounds);
	}
	storeroundkey(rk32, aes_schedule_mangle_last_enc(rk, sr[i-- % 4].m));
}

void
aes_ssse3_setdeckey(struct aesdec *dec, const uint8_t *key, unsigned nrounds)
{
	uint32_t *rk32 = dec->aesd_aes.aes_rk;
	__m128i mrk;		/* mangled round key */
	__m128i ork;		/* original round key */
	__m128i rk;		/* round key */
	__m128i prk;		/* previous round key */
	__m128i rcon_rot = rcon.m;
	unsigned i = nrounds == 12 ? 0 : 2;

	ork = _mm_loadu_epi8(key);

	/* input transform */
	rk = aes_schedule_transform(ork, ipt);

	/* go from end */
	rk32 += 4*nrounds;
	storeroundkey(rk32, _mm_shuffle_epi8(ork, sr[i].m));
	rk32 -= 4;
	i ^= 3;

	switch (nrounds) {
	case 10:
		for (;;) {
			rk = aes_schedule_round(rk, rk, &rcon_rot);
			if (--nrounds == 0)
				break;
			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
		}
		break;
	case 12: {
		__m128i prkhi;		/* high half of previous round key */

		prk = rk;
		rk = aes_schedule_transform(_mm_loadu_epi8(key + 8), ipt);
		prkhi = aes_schedule_192_smearhi(rk);
		for (;;) {
			prk = aes_schedule_round(rk, prk, &rcon_rot);
			rk = _mm_alignr_epi8(prk, prkhi, 8);

			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);

			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 3) == 0)
				break;

			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			rk = aes_schedule_192_smear(prkhi, prk);
			prkhi = aes_schedule_192_smearhi(rk);
		}
		break;
	}
	case 14: {
		__m128i pprk;		/* previous previous round key */

		prk = rk;
		rk = aes_schedule_transform(_mm_loadu_epi8(key + 16), ipt);
		for (;;) {
			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 -= 4;
			pprk = rk;

			/* high round */
			rk = prk = aes_schedule_round(rk, prk, &rcon_rot);
			if ((nrounds -= 2) == 0)
				break;
			mrk = aes_schedule_mangle_dec(rk, sr[i-- % 4].m);
			storeroundkey(rk32, mrk);
			rk32 -= 4;

			/* low round */
			rk = _mm_shuffle_epi32(rk, 0xff);
			rk = aes_schedule_low_round(rk, pprk);
		}
		break;
	}
	default:
		panic("invalid number of AES rounds: %u", nrounds);
	}
	storeroundkey(rk32, aes_schedule_mangle_last_dec(rk));
}

__m128i
aes_ssse3_enc1(const struct aesenc *enc, __m128i x, unsigned nrounds)
{
	const uint32_t *rk32 = enc->aese_aes.aes_rk;
	__m128i io, jo;
	unsigned rmod4 = 0;

	x = aes_schedule_transform(x, ipt);
	x ^= loadroundkey(rk32);
	for (;;) {
		__m128i A, A2, A2_B, A2_B_D;

		subbytes(&io, &jo, x);

		rk32 += 4;
		rmod4 = (rmod4 + 1) % 4;
		if (--nrounds == 0)
			break;

		A = _mm_shuffle_epi8(sb1[0].m, io) ^
		    _mm_shuffle_epi8(sb1[1].m, jo);
		A ^= loadroundkey(rk32);
		A2 = _mm_shuffle_epi8(sb2[0].m, io) ^
		    _mm_shuffle_epi8(sb2[1].m, jo);
		A2_B = A2 ^ _mm_shuffle_epi8(A, mc_forward[rmod4].m);
		A2_B_D = A2_B ^ _mm_shuffle_epi8(A, mc_backward[rmod4].m);
		x = A2_B_D ^ _mm_shuffle_epi8(A2_B, mc_forward[rmod4].m);
	}
	x = _mm_shuffle_epi8(sbo[0].m, io) ^ _mm_shuffle_epi8(sbo[1].m, jo);
	x ^= loadroundkey(rk32);
	return _mm_shuffle_epi8(x, sr[rmod4].m);
}

__m128i
aes_ssse3_dec1(const struct aesdec *dec, __m128i x, unsigned nrounds)
{
	const uint32_t *rk32 = dec->aesd_aes.aes_rk;
	unsigned i = 3 & ~(nrounds - 1);
	__m128i io, jo, mc;

	x = aes_schedule_transform(x, dipt);
	x ^= loadroundkey(rk32);
	rk32 += 4;

	mc = mc_forward[3].m;
	for (;;) {
		subbytes(&io, &jo, x);
		if (--nrounds == 0)
			break;

		x = _mm_shuffle_epi8(dsb9[0].m, io) ^
		    _mm_shuffle_epi8(dsb9[1].m, jo);
		x ^= loadroundkey(rk32);
		rk32 += 4;				/* next round key */

		x = _mm_shuffle_epi8(x, mc);
		x ^= _mm_shuffle_epi8(dsbd[0].m, io) ^
		    _mm_shuffle_epi8(dsbd[1].m, jo);

		x = _mm_shuffle_epi8(x, mc);
		x ^= _mm_shuffle_epi8(dsbb[0].m, io) ^
		    _mm_shuffle_epi8(dsbb[1].m, jo);

		x = _mm_shuffle_epi8(x, mc);
		x ^= _mm_shuffle_epi8(dsbe[0].m, io) ^
		    _mm_shuffle_epi8(dsbe[1].m, jo);

		mc = _mm_alignr_epi8(mc, mc, 12);
	}
	x = _mm_shuffle_epi8(dsbo[0].m, io) ^ _mm_shuffle_epi8(dsbo[1].m, jo);
	x ^= loadroundkey(rk32);
	return _mm_shuffle_epi8(x, sr[i].m);
}
