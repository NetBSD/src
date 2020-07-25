/*	$NetBSD: aes_neon_subr.c,v 1.3 2020/07/25 22:36:06 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_neon_subr.c,v 1.3 2020/07/25 22:36:06 riastradh Exp $");

#include <sys/endian.h>

#ifdef _KERNEL
#include <sys/systm.h>
#include <lib/libkern/libkern.h>
#else
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#define	KASSERT			assert
#endif

#include <crypto/aes/arch/arm/aes_neon.h>

#include "aes_neon_impl.h"

static inline uint8x16_t
loadblock(const void *in)
{
	return vld1q_u8(in);
}

static inline void
storeblock(void *out, uint8x16_t block)
{
	vst1q_u8(out, block);
}

void
aes_neon_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint8x16_t block;

	block = loadblock(in);
	block = aes_neon_enc1(enc, block, nrounds);
	storeblock(out, block);
}

void
aes_neon_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint8x16_t block;

	block = loadblock(in);
	block = aes_neon_dec1(dec, block, nrounds);
	storeblock(out, block);
}

void
aes_neon_cbc_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	uint8x16_t cv;

	KASSERT(nbytes);

	cv = loadblock(iv);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		cv ^= loadblock(in);
		cv = aes_neon_enc1(enc, cv, nrounds);
		storeblock(out, cv);
	}
	storeblock(iv, cv);
}

void
aes_neon_cbc_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	uint8x16_t iv0, cv, b;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	iv0 = loadblock(iv);
	cv = loadblock(in + nbytes - 16);
	storeblock(iv, cv);

	for (;;) {
		b = aes_neon_dec1(dec, cv, nrounds);
		if ((nbytes -= 16) == 0)
			break;
		cv = loadblock(in + nbytes - 16);
		storeblock(out + nbytes, b ^ cv);
	}
	storeblock(out, b ^ iv0);
}

static inline uint8x16_t
aes_neon_xts_update(uint8x16_t t8)
{
	const int32x4_t zero = vdupq_n_s32(0);
	const int32x4_t carry = {0x87, 1, 1, 1};
	int32x4_t t, t_;
	uint32x4_t mask;

	t = vreinterpretq_s32_u8(t8);
	mask = vcltq_s32(t, zero);		/* -1 if high bit set else 0 */
	mask = vextq_u32(mask, mask, 3);	/* rotate quarters */
	t_ = vsliq_n_s32(zero, t, 1);		/* shift */
	t_ ^= carry & mask;

	return vreinterpretq_u8_s32(t_);
}

static int
aes_neon_xts_update_selftest(void)
{
	static const struct {
		uint32_t in[4], out[4];
	} cases[] = {
		[0] = { {1}, {2} },
		[1] = { {0x80000000U,0,0,0}, {0,1,0,0} },
		[2] = { {0,0x80000000U,0,0}, {0,0,1,0} },
		[3] = { {0,0,0x80000000U,0}, {0,0,0,1} },
		[4] = { {0,0,0,0x80000000U}, {0x87,0,0,0} },
		[5] = { {0,0x80000000U,0,0x80000000U}, {0x87,0,1,0} },
	};
	unsigned i;
	uint32_t t[4];
	int result = 0;

	for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		t[0] = cases[i].in[0];
		t[1] = cases[i].in[1];
		t[2] = cases[i].in[2];
		t[3] = cases[i].in[3];
		storeblock(t, aes_neon_xts_update(loadblock(t)));
		if (t[0] != cases[i].out[0] ||
		    t[1] != cases[i].out[1] ||
		    t[2] != cases[i].out[2] ||
		    t[3] != cases[i].out[3]) {
			printf("%s %u:"
			    " %"PRIx32" %"PRIx32" %"PRIx32" %"PRIx32"\n",
			    __func__, i, t[0], t[1], t[2], t[3]);
			result = -1;
		}
	}

	return result;
}

void
aes_neon_xts_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint8x16_t t, b;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	t = loadblock(tweak);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		b = t ^ loadblock(in);
		b = aes_neon_enc1(enc, b, nrounds);
		storeblock(out, t ^ b);
		t = aes_neon_xts_update(t);
	}
	storeblock(tweak, t);
}

void
aes_neon_xts_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint8x16_t t, b;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	t = loadblock(tweak);
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		b = t ^ loadblock(in);
		b = aes_neon_dec1(dec, b, nrounds);
		storeblock(out, t ^ b);
		t = aes_neon_xts_update(t);
	}
	storeblock(tweak, t);
}

void
aes_neon_cbcmac_update1(const struct aesenc *enc, const uint8_t in[static 16],
    size_t nbytes, uint8_t auth0[static 16], uint32_t nrounds)
{
	uint8x16_t auth;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	auth = loadblock(auth0);
	for (; nbytes; nbytes -= 16, in += 16)
		auth = aes_neon_enc1(enc, auth ^ loadblock(in), nrounds);
	storeblock(auth0, auth);
}

/*
 * XXX On aarch64, we have enough registers that we should be able to
 * pipeline two simultaneous vpaes computations in an `aes_neon_enc2'
 * function, which should substantially improve CCM throughput.
 */

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	vbetoh32q_u8	vrev32q_u8
#define	vhtobe32q_u8	vrev32q_u8
#elif _BYTE_ORDER == _BIG_ENDIAN
#define	vbetoh32q_u8(x)	(x)
#define	vhtobe32q_u8(x)	(x)
#else
#error what kind of endian are you anyway
#endif

void
aes_neon_ccm_enc1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{
	const uint32x4_t ctr32_inc = {0, 0, 0, 1};
	uint8x16_t auth, ptxt, ctr_be;
	uint32x4_t ctr;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	auth = loadblock(authctr);
	ctr_be = loadblock(authctr + 16);
	ctr = vreinterpretq_u32_u8(vbetoh32q_u8(ctr_be));
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		ptxt = loadblock(in);
		auth = aes_neon_enc1(enc, auth ^ ptxt, nrounds);
		ctr = vaddq_u32(ctr, ctr32_inc);
		ctr_be = vhtobe32q_u8(vreinterpretq_u8_u32(ctr));
		storeblock(out, ptxt ^ aes_neon_enc1(enc, ctr_be, nrounds));
	}
	storeblock(authctr, auth);
	storeblock(authctr + 16, ctr_be);
}

void
aes_neon_ccm_dec1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr[static 32],
    uint32_t nrounds)
{
	const uint32x4_t ctr32_inc = {0, 0, 0, 1};
	uint8x16_t auth, ctr_be, ptxt;
	uint32x4_t ctr;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	auth = loadblock(authctr);
	ctr_be = loadblock(authctr + 16);
	ctr = vreinterpretq_u32_u8(vbetoh32q_u8(ctr_be));
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		ctr = vaddq_u32(ctr, ctr32_inc);
		ctr_be = vhtobe32q_u8(vreinterpretq_u8_u32(ctr));
		ptxt = loadblock(in) ^ aes_neon_enc1(enc, ctr_be, nrounds);
		storeblock(out, ptxt);
		auth = aes_neon_enc1(enc, auth ^ ptxt, nrounds);
	}
	storeblock(authctr, auth);
	storeblock(authctr + 16, ctr_be);
}

int
aes_neon_selftest(void)
{

	if (aes_neon_xts_update_selftest())
		return -1;

	return 0;
}
