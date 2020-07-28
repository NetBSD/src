/*	$NetBSD: aes_via.c,v 1.6 2020/07/28 14:01:35 riastradh Exp $	*/

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
__KERNEL_RCSID(1, "$NetBSD: aes_via.c,v 1.6 2020/07/28 14:01:35 riastradh Exp $");

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/evcnt.h>
#include <sys/systm.h>
#else
#include <assert.h>
#include <err.h>
#include <stdint.h>
#include <string.h>
#define	KASSERT			assert
#define	panic(fmt, args...)	err(1, fmt, args)
struct evcnt { uint64_t ev_count; };
#define	EVCNT_INITIALIZER(a,b,c,d) {0}
#define	EVCNT_ATTACH_STATIC(name)	static char name##_attach __unused = 0
#endif

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_bear.h>
#include <crypto/aes/aes_impl.h>

#ifdef _KERNEL
#include <x86/cpufunc.h>
#include <x86/cpuvar.h>
#include <x86/fpu.h>
#include <x86/specialreg.h>
#include <x86/via_padlock.h>
#else
#include <cpuid.h>
#define	fpu_kern_enter()	((void)0)
#define	fpu_kern_leave()	((void)0)
#define C3_CRYPT_CWLO_ROUND_M		0x0000000f
#define C3_CRYPT_CWLO_ALG_M		0x00000070
#define C3_CRYPT_CWLO_ALG_AES		0x00000000
#define C3_CRYPT_CWLO_KEYGEN_M		0x00000080
#define C3_CRYPT_CWLO_KEYGEN_HW		0x00000000
#define C3_CRYPT_CWLO_KEYGEN_SW		0x00000080
#define C3_CRYPT_CWLO_NORMAL		0x00000000
#define C3_CRYPT_CWLO_INTERMEDIATE	0x00000100
#define C3_CRYPT_CWLO_ENCRYPT		0x00000000
#define C3_CRYPT_CWLO_DECRYPT		0x00000200
#define C3_CRYPT_CWLO_KEY128		0x0000000a      /* 128bit, 10 rds */
#define C3_CRYPT_CWLO_KEY192		0x0000040c      /* 192bit, 12 rds */
#define C3_CRYPT_CWLO_KEY256		0x0000080e      /* 256bit, 15 rds */
#endif

static void
aesvia_reload_keys(void)
{

	asm volatile("pushf; popf");
}

static uint32_t
aesvia_keylen_cw0(unsigned nrounds)
{

	/*
	 * Determine the control word bits for the key size / number of
	 * rounds.  For AES-128, the hardware can do key expansion on
	 * the fly; for AES-192 and AES-256, software must do it.
	 */
	switch (nrounds) {
	case AES_128_NROUNDS:
		return C3_CRYPT_CWLO_KEY128;
	case AES_192_NROUNDS:
		return C3_CRYPT_CWLO_KEY192 | C3_CRYPT_CWLO_KEYGEN_SW;
	case AES_256_NROUNDS:
		return C3_CRYPT_CWLO_KEY256 | C3_CRYPT_CWLO_KEYGEN_SW;
	default:
		panic("invalid AES nrounds: %u", nrounds);
	}
}

static void
aesvia_setenckey(struct aesenc *enc, const uint8_t *key, uint32_t nrounds)
{
	size_t key_len;

	switch (nrounds) {
	case AES_128_NROUNDS:
		enc->aese_aes.aes_rk[0] = le32dec(key + 4*0);
		enc->aese_aes.aes_rk[1] = le32dec(key + 4*1);
		enc->aese_aes.aes_rk[2] = le32dec(key + 4*2);
		enc->aese_aes.aes_rk[3] = le32dec(key + 4*3);
		return;
	case AES_192_NROUNDS:
		key_len = 24;
		break;
	case AES_256_NROUNDS:
		key_len = 32;
		break;
	default:
		panic("invalid AES nrounds: %u", nrounds);
	}
	br_aes_ct_keysched_stdenc(enc->aese_aes.aes_rk, key, key_len);
}

static void
aesvia_setdeckey(struct aesdec *dec, const uint8_t *key, uint32_t nrounds)
{
	size_t key_len;

	switch (nrounds) {
	case AES_128_NROUNDS:
		dec->aesd_aes.aes_rk[0] = le32dec(key + 4*0);
		dec->aesd_aes.aes_rk[1] = le32dec(key + 4*1);
		dec->aesd_aes.aes_rk[2] = le32dec(key + 4*2);
		dec->aesd_aes.aes_rk[3] = le32dec(key + 4*3);
		return;
	case AES_192_NROUNDS:
		key_len = 24;
		break;
	case AES_256_NROUNDS:
		key_len = 32;
		break;
	default:
		panic("invalid AES nrounds: %u", nrounds);
	}
	br_aes_ct_keysched_stddec(dec->aesd_aes.aes_rk, key, key_len);
}

static inline void
aesvia_encN(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nblocks, uint32_t cw0)
{
	const uint32_t cw[4] __aligned(16) = {
		[0] = (cw0
		    | C3_CRYPT_CWLO_ALG_AES
		    | C3_CRYPT_CWLO_ENCRYPT
		    | C3_CRYPT_CWLO_NORMAL),
	};

	KASSERT(((uintptr_t)enc & 0xf) == 0);
	KASSERT(((uintptr_t)in & 0xf) == 0);
	KASSERT(((uintptr_t)out & 0xf) == 0);

	asm volatile("rep xcryptecb"
	    : "+c"(nblocks), "+S"(in), "+D"(out)
	    : "b"(enc), "d"(cw)
	    : "memory", "cc");
}

static inline void
aesvia_decN(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nblocks, uint32_t cw0)
{
	const uint32_t cw[4] __aligned(16) = {
		[0] = (cw0
		    | C3_CRYPT_CWLO_ALG_AES
		    | C3_CRYPT_CWLO_DECRYPT
		    | C3_CRYPT_CWLO_NORMAL),
	};

	KASSERT(((uintptr_t)dec & 0xf) == 0);
	KASSERT(((uintptr_t)in & 0xf) == 0);
	KASSERT(((uintptr_t)out & 0xf) == 0);

	asm volatile("rep xcryptecb"
	    : "+c"(nblocks), "+S"(in), "+D"(out)
	    : "b"(dec), "d"(cw)
	    : "memory", "cc");
}

static struct evcnt enc_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "enc aligned");
EVCNT_ATTACH_STATIC(enc_aligned_evcnt);
static struct evcnt enc_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "dec unaligned");
EVCNT_ATTACH_STATIC(enc_unaligned_evcnt);

static void
aesvia_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);

	fpu_kern_enter();
	aesvia_reload_keys();
	if ((((uintptr_t)in | (uintptr_t)out) & 0xf) == 0 &&
	    ((uintptr_t)in & 0xff0) != 0xff0) {
		enc_aligned_evcnt.ev_count++;
		aesvia_encN(enc, in, out, 1, cw0);
	} else {
		enc_unaligned_evcnt.ev_count++;
		/*
		 * VIA requires 16-byte/128-bit alignment, and
		 * xcrypt-ecb reads one block past the one we're
		 * working on -- which may go past the end of the page
		 * into unmapped territory.  Use a bounce buffer if
		 * either constraint is violated.
		 */
		uint8_t inbuf[16] __aligned(16);
		uint8_t outbuf[16] __aligned(16);

		memcpy(inbuf, in, 16);
		aesvia_encN(enc, inbuf, outbuf, 1, cw0);
		memcpy(out, outbuf, 16);

		explicit_memset(inbuf, 0, sizeof inbuf);
		explicit_memset(outbuf, 0, sizeof outbuf);
	}
	fpu_kern_leave();
}

static struct evcnt dec_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "dec aligned");
EVCNT_ATTACH_STATIC(dec_aligned_evcnt);
static struct evcnt dec_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "dec unaligned");
EVCNT_ATTACH_STATIC(dec_unaligned_evcnt);

static void
aesvia_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);

	fpu_kern_enter();
	aesvia_reload_keys();
	if ((((uintptr_t)in | (uintptr_t)out) & 0xf) == 0 &&
	    ((uintptr_t)in & 0xff0) != 0xff0) {
		dec_aligned_evcnt.ev_count++;
		aesvia_decN(dec, in, out, 1, cw0);
	} else {
		dec_unaligned_evcnt.ev_count++;
		/*
		 * VIA requires 16-byte/128-bit alignment, and
		 * xcrypt-ecb reads one block past the one we're
		 * working on -- which may go past the end of the page
		 * into unmapped territory.  Use a bounce buffer if
		 * either constraint is violated.
		 */
		uint8_t inbuf[16] __aligned(16);
		uint8_t outbuf[16] __aligned(16);

		memcpy(inbuf, in, 16);
		aesvia_decN(dec, inbuf, outbuf, 1, cw0);
		memcpy(out, outbuf, 16);

		explicit_memset(inbuf, 0, sizeof inbuf);
		explicit_memset(outbuf, 0, sizeof outbuf);
	}
	fpu_kern_leave();
}

static inline void
aesvia_cbc_encN(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nblocks, uint8_t **ivp, uint32_t cw0)
{
	const uint32_t cw[4] __aligned(16) = {
		[0] = (cw0
		    | C3_CRYPT_CWLO_ALG_AES
		    | C3_CRYPT_CWLO_ENCRYPT
		    | C3_CRYPT_CWLO_NORMAL),
	};

	KASSERT(((uintptr_t)enc & 0xf) == 0);
	KASSERT(((uintptr_t)in & 0xf) == 0);
	KASSERT(((uintptr_t)out & 0xf) == 0);
	KASSERT(((uintptr_t)*ivp & 0xf) == 0);

	/*
	 * Register effects:
	 * - Counts nblocks down to zero.
	 * - Advances in by nblocks (units of blocks).
	 * - Advances out by nblocks (units of blocks).
	 * - Updates *ivp to point at the last block of out.
	 */
	asm volatile("rep xcryptcbc"
	    : "+c"(nblocks), "+S"(in), "+D"(out), "+a"(*ivp)
	    : "b"(enc), "d"(cw)
	    : "memory", "cc");
}

static inline void
aesvia_cbc_decN(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nblocks, uint8_t iv[static 16],
    uint32_t cw0)
{
	const uint32_t cw[4] __aligned(16) = {
		[0] = (cw0
		    | C3_CRYPT_CWLO_ALG_AES
		    | C3_CRYPT_CWLO_DECRYPT
		    | C3_CRYPT_CWLO_NORMAL),
	};

	KASSERT(((uintptr_t)dec & 0xf) == 0);
	KASSERT(((uintptr_t)in & 0xf) == 0);
	KASSERT(((uintptr_t)out & 0xf) == 0);
	KASSERT(((uintptr_t)iv & 0xf) == 0);

	/*
	 * Register effects:
	 * - Counts nblocks down to zero.
	 * - Advances in by nblocks (units of blocks).
	 * - Advances out by nblocks (units of blocks).
	 * Memory side effects:
	 * - Writes what was the last block of in at the address iv.
	 */
	asm volatile("rep xcryptcbc"
	    : "+c"(nblocks), "+S"(in), "+D"(out)
	    : "a"(iv), "b"(dec), "d"(cw)
	    : "memory", "cc");
}

static inline void
xor128(void *x, const void *a, const void *b)
{
	uint32_t *x32 = x;
	const uint32_t *a32 = a;
	const uint32_t *b32 = b;

	x32[0] = a32[0] ^ b32[0];
	x32[1] = a32[1] ^ b32[1];
	x32[2] = a32[2] ^ b32[2];
	x32[3] = a32[3] ^ b32[3];
}

static struct evcnt cbcenc_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "cbcenc aligned");
EVCNT_ATTACH_STATIC(cbcenc_aligned_evcnt);
static struct evcnt cbcenc_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "cbcenc unaligned");
EVCNT_ATTACH_STATIC(cbcenc_unaligned_evcnt);

static void
aesvia_cbc_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);

	KASSERT(nbytes % 16 == 0);
	if (nbytes == 0)
		return;

	fpu_kern_enter();
	aesvia_reload_keys();
	if ((((uintptr_t)in | (uintptr_t)out | (uintptr_t)iv) & 0xf) == 0) {
		cbcenc_aligned_evcnt.ev_count++;
		uint8_t *ivp = iv;
		aesvia_cbc_encN(enc, in, out, nbytes/16, &ivp, cw0);
		memcpy(iv, ivp, 16);
	} else {
		cbcenc_unaligned_evcnt.ev_count++;
		uint8_t cv[16] __aligned(16);
		uint8_t tmp[16] __aligned(16);

		memcpy(cv, iv, 16);
		for (; nbytes; nbytes -= 16, in += 16, out += 16) {
			memcpy(tmp, in, 16);
			xor128(tmp, tmp, cv);
			aesvia_encN(enc, tmp, cv, 1, cw0);
			memcpy(out, cv, 16);
		}
		memcpy(iv, cv, 16);
	}
	fpu_kern_leave();
}

static struct evcnt cbcdec_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "cbcdec aligned");
EVCNT_ATTACH_STATIC(cbcdec_aligned_evcnt);
static struct evcnt cbcdec_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "cbcdec unaligned");
EVCNT_ATTACH_STATIC(cbcdec_unaligned_evcnt);

static void
aesvia_cbc_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);

	KASSERT(nbytes % 16 == 0);
	if (nbytes == 0)
		return;

	fpu_kern_enter();
	aesvia_reload_keys();
	if ((((uintptr_t)in | (uintptr_t)out | (uintptr_t)iv) & 0xf) == 0) {
		cbcdec_aligned_evcnt.ev_count++;
		aesvia_cbc_decN(dec, in, out, nbytes/16, iv, cw0);
	} else {
		cbcdec_unaligned_evcnt.ev_count++;
		uint8_t iv0[16] __aligned(16);
		uint8_t cv[16] __aligned(16);
		uint8_t tmp[16] __aligned(16);

		memcpy(iv0, iv, 16);
		memcpy(cv, in + nbytes - 16, 16);
		memcpy(iv, cv, 16);

		for (;;) {
			aesvia_decN(dec, cv, tmp, 1, cw0);
			if ((nbytes -= 16) == 0)
				break;
			memcpy(cv, in + nbytes - 16, 16);
			xor128(tmp, tmp, cv);
			memcpy(out + nbytes, tmp, 16);
		}

		xor128(tmp, tmp, iv0);
		memcpy(out, tmp, 16);
		explicit_memset(tmp, 0, sizeof tmp);
	}
	fpu_kern_leave();
}

static inline void
aesvia_xts_update(uint32_t *t0, uint32_t *t1, uint32_t *t2, uint32_t *t3)
{
	uint32_t s0, s1, s2, s3;

	s0 = *t0 >> 31;
	s1 = *t1 >> 31;
	s2 = *t2 >> 31;
	s3 = *t3 >> 31;
	*t0 = (*t0 << 1) ^ (-s3 & 0x87);
	*t1 = (*t1 << 1) ^ s0;
	*t2 = (*t2 << 1) ^ s1;
	*t3 = (*t3 << 1) ^ s2;
}

static int
aesvia_xts_update_selftest(void)
{
	static const struct {
		uint32_t in[4], out[4];
	} cases[] = {
		{ {1}, {2} },
		{ {0x80000000U,0,0,0}, {0,1,0,0} },
		{ {0,0x80000000U,0,0}, {0,0,1,0} },
		{ {0,0,0x80000000U,0}, {0,0,0,1} },
		{ {0,0,0,0x80000000U}, {0x87,0,0,0} },
		{ {0,0x80000000U,0,0x80000000U}, {0x87,0,1,0} },
	};
	unsigned i;
	uint32_t t0, t1, t2, t3;

	for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
		t0 = cases[i].in[0];
		t1 = cases[i].in[1];
		t2 = cases[i].in[2];
		t3 = cases[i].in[3];
		aesvia_xts_update(&t0, &t1, &t2, &t3);
		if (t0 != cases[i].out[0] ||
		    t1 != cases[i].out[1] ||
		    t2 != cases[i].out[2] ||
		    t3 != cases[i].out[3])
			return -1;
	}

	/* Success!  */
	return 0;
}

static struct evcnt xtsenc_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "xtsenc aligned");
EVCNT_ATTACH_STATIC(xtsenc_aligned_evcnt);
static struct evcnt xtsenc_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "xtsenc unaligned");
EVCNT_ATTACH_STATIC(xtsenc_unaligned_evcnt);

static void
aesvia_xts_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);
	uint32_t t[4];

	KASSERT(nbytes % 16 == 0);

	memcpy(t, tweak, 16);

	fpu_kern_enter();
	aesvia_reload_keys();
	if ((((uintptr_t)in | (uintptr_t)out) & 0xf) == 0) {
		xtsenc_aligned_evcnt.ev_count++;
		unsigned lastblock = 0;
		uint32_t buf[8*4] __aligned(16);

		/*
		 * Make sure the last block is not the last block of a
		 * page.  (Note that we store the AES input in `out' as
		 * a temporary buffer, rather than reading it directly
		 * from `in', since we have to combine the tweak
		 * first.)
		 */
		lastblock = 16*(((uintptr_t)(out + nbytes) & 0xfff) == 0);
		nbytes -= lastblock;

		/*
		 * Handle an odd number of initial blocks so we can
		 * process the rest in eight-block (128-byte) chunks.
		 */
		if (nbytes % 128) {
			unsigned nbytes128 = nbytes % 128;

			nbytes -= nbytes128;
			for (; nbytes128; nbytes128 -= 16, in += 16, out += 16)
			{
				xor128(out, in, t);
				aesvia_encN(enc, out, out, 1, cw0);
				xor128(out, out, t);
				aesvia_xts_update(&t[0], &t[1], &t[2], &t[3]);
			}
		}

		/* Process eight blocks at a time.  */
		for (; nbytes; nbytes -= 128, in += 128, out += 128) {
			unsigned i;
			for (i = 0; i < 8; i++) {
				memcpy(buf + 4*i, t, 16);
				xor128(out + 4*i, in + 4*i, t);
				aesvia_xts_update(&t[0], &t[1], &t[2], &t[3]);
			}
			aesvia_encN(enc, out, out, 8, cw0);
			for (i = 0; i < 8; i++)
				xor128(out + 4*i, in + 4*i, buf + 4*i);
		}

		/* Handle the last block of a page, if necessary.  */
		if (lastblock) {
			xor128(buf, in, t);
			aesvia_encN(enc, (const void *)buf, out, 1, cw0);
		}

		explicit_memset(buf, 0, sizeof buf);
	} else {
		xtsenc_unaligned_evcnt.ev_count++;
		uint8_t buf[16] __aligned(16);

		for (; nbytes; nbytes -= 16, in += 16, out += 16) {
			memcpy(buf, in, 16);
			xor128(buf, buf, t);
			aesvia_encN(enc, buf, buf, 1, cw0);
			xor128(buf, buf, t);
			memcpy(out, buf, 16);
			aesvia_xts_update(&t[0], &t[1], &t[2], &t[3]);
		}

		explicit_memset(buf, 0, sizeof buf);
	}
	fpu_kern_leave();

	memcpy(tweak, t, 16);
	explicit_memset(t, 0, sizeof t);
}

static struct evcnt xtsdec_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "xtsdec aligned");
EVCNT_ATTACH_STATIC(xtsdec_aligned_evcnt);
static struct evcnt xtsdec_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "xtsdec unaligned");
EVCNT_ATTACH_STATIC(xtsdec_unaligned_evcnt);

static void
aesvia_xts_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);
	uint32_t t[4];

	KASSERT(nbytes % 16 == 0);

	memcpy(t, tweak, 16);

	fpu_kern_enter();
	aesvia_reload_keys();
	if ((((uintptr_t)in | (uintptr_t)out) & 0xf) == 0) {
		xtsdec_aligned_evcnt.ev_count++;
		unsigned lastblock = 0;
		uint32_t buf[8*4] __aligned(16);

		/*
		 * Make sure the last block is not the last block of a
		 * page.  (Note that we store the AES input in `out' as
		 * a temporary buffer, rather than reading it directly
		 * from `in', since we have to combine the tweak
		 * first.)
		 */
		lastblock = 16*(((uintptr_t)(out + nbytes) & 0xfff) == 0);
		nbytes -= lastblock;

		/*
		 * Handle an odd number of initial blocks so we can
		 * process the rest in eight-block (128-byte) chunks.
		 */
		if (nbytes % 128) {
			unsigned nbytes128 = nbytes % 128;

			nbytes -= nbytes128;
			for (; nbytes128; nbytes128 -= 16, in += 16, out += 16)
			{
				xor128(out, in, t);
				aesvia_decN(dec, out, out, 1, cw0);
				xor128(out, out, t);
				aesvia_xts_update(&t[0], &t[1], &t[2], &t[3]);
			}
		}

		/* Process eight blocks at a time.  */
		for (; nbytes; nbytes -= 128, in += 128, out += 128) {
			unsigned i;
			for (i = 0; i < 8; i++) {
				memcpy(buf + 4*i, t, 16);
				xor128(out + 4*i, in + 4*i, t);
				aesvia_xts_update(&t[0], &t[1], &t[2], &t[3]);
			}
			aesvia_decN(dec, out, out, 8, cw0);
			for (i = 0; i < 8; i++)
				xor128(out + 4*i, in + 4*i, buf + 4*i);
		}

		/* Handle the last block of a page, if necessary.  */
		if (lastblock) {
			xor128(buf, in, t);
			aesvia_decN(dec, (const void *)buf, out, 1, cw0);
		}

		explicit_memset(buf, 0, sizeof buf);
	} else {
		xtsdec_unaligned_evcnt.ev_count++;
		uint8_t buf[16] __aligned(16);

		for (; nbytes; nbytes -= 16, in += 16, out += 16) {
			memcpy(buf, in, 16);
			xor128(buf, buf, t);
			aesvia_decN(dec, buf, buf, 1, cw0);
			xor128(buf, buf, t);
			memcpy(out, buf, 16);
			aesvia_xts_update(&t[0], &t[1], &t[2], &t[3]);
		}

		explicit_memset(buf, 0, sizeof buf);
	}
	fpu_kern_leave();

	memcpy(tweak, t, 16);
	explicit_memset(t, 0, sizeof t);
}

static struct evcnt cbcmac_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "cbcmac aligned");
EVCNT_ATTACH_STATIC(cbcmac_aligned_evcnt);
static struct evcnt cbcmac_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "cbcmac unaligned");
EVCNT_ATTACH_STATIC(cbcmac_unaligned_evcnt);

static void
aesvia_cbcmac_update1(const struct aesenc *enc, const uint8_t in[static 16],
    size_t nbytes, uint8_t auth0[static 16], uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);
	uint8_t authbuf[16] __aligned(16);
	uint8_t *auth = auth0;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	if ((uintptr_t)auth0 & 0xf) {
		memcpy(authbuf, auth0, 16);
		auth = authbuf;
		cbcmac_unaligned_evcnt.ev_count++;
	} else {
		cbcmac_aligned_evcnt.ev_count++;
	}

	fpu_kern_enter();
	aesvia_reload_keys();
	for (; nbytes; nbytes -= 16, in += 16) {
		xor128(auth, auth, in);
		aesvia_encN(enc, auth, auth, 1, cw0);
	}
	fpu_kern_leave();

	if ((uintptr_t)auth0 & 0xf) {
		memcpy(auth0, authbuf, 16);
		explicit_memset(authbuf, 0, sizeof authbuf);
	}
}

static struct evcnt ccmenc_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "ccmenc aligned");
EVCNT_ATTACH_STATIC(ccmenc_aligned_evcnt);
static struct evcnt ccmenc_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "ccmenc unaligned");
EVCNT_ATTACH_STATIC(ccmenc_unaligned_evcnt);

static void
aesvia_ccm_enc1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr0[static 32],
    uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);
	uint8_t authctrbuf[32] __aligned(16);
	uint8_t *authctr;
	uint32_t c0, c1, c2, c3;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	if ((uintptr_t)authctr0 & 0xf) {
		memcpy(authctrbuf, authctr0, 16);
		authctr = authctrbuf;
		ccmenc_unaligned_evcnt.ev_count++;
	} else {
		authctr = authctr0;
		ccmenc_aligned_evcnt.ev_count++;
	}
	c0 = le32dec(authctr0 + 16 + 4*0);
	c1 = le32dec(authctr0 + 16 + 4*1);
	c2 = le32dec(authctr0 + 16 + 4*2);
	c3 = be32dec(authctr0 + 16 + 4*3);

	/*
	 * In principle we could use REP XCRYPTCTR here, but that
	 * doesn't help to compute the CBC-MAC step, and certain VIA
	 * CPUs have some weird errata with REP XCRYPTCTR that make it
	 * kind of a pain to use.  So let's just use REP XCRYPTECB to
	 * simultaneously compute the CBC-MAC step and the CTR step.
	 * (Maybe some VIA CPUs will compute REP XCRYPTECB in parallel,
	 * who knows...)
	 */
	fpu_kern_enter();
	aesvia_reload_keys();
	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		xor128(authctr, authctr, in);
		le32enc(authctr + 16 + 4*0, c0);
		le32enc(authctr + 16 + 4*1, c1);
		le32enc(authctr + 16 + 4*2, c2);
		be32enc(authctr + 16 + 4*3, ++c3);
		aesvia_encN(enc, authctr, authctr, 2, cw0);
		xor128(out, in, authctr + 16);
	}
	fpu_kern_leave();

	if ((uintptr_t)authctr0 & 0xf) {
		memcpy(authctr0, authctrbuf, 16);
		explicit_memset(authctrbuf, 0, sizeof authctrbuf);
	}

	le32enc(authctr0 + 16 + 4*0, c0);
	le32enc(authctr0 + 16 + 4*1, c1);
	le32enc(authctr0 + 16 + 4*2, c2);
	be32enc(authctr0 + 16 + 4*3, c3);
}

static struct evcnt ccmdec_aligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "ccmdec aligned");
EVCNT_ATTACH_STATIC(ccmdec_aligned_evcnt);
static struct evcnt ccmdec_unaligned_evcnt = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "aesvia", "ccmdec unaligned");
EVCNT_ATTACH_STATIC(ccmdec_unaligned_evcnt);

static void
aesvia_ccm_dec1(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t authctr0[static 32],
    uint32_t nrounds)
{
	const uint32_t cw0 = aesvia_keylen_cw0(nrounds);
	uint8_t authctrbuf[32] __aligned(16);
	uint8_t *authctr;
	uint32_t c0, c1, c2, c3;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	c0 = le32dec(authctr0 + 16 + 4*0);
	c1 = le32dec(authctr0 + 16 + 4*1);
	c2 = le32dec(authctr0 + 16 + 4*2);
	c3 = be32dec(authctr0 + 16 + 4*3);

	if ((uintptr_t)authctr0 & 0xf) {
		memcpy(authctrbuf, authctr0, 16);
		authctr = authctrbuf;
		le32enc(authctr + 16 + 4*0, c0);
		le32enc(authctr + 16 + 4*1, c1);
		le32enc(authctr + 16 + 4*2, c2);
		ccmdec_unaligned_evcnt.ev_count++;
	} else {
		authctr = authctr0;
		ccmdec_aligned_evcnt.ev_count++;
	}

	fpu_kern_enter();
	aesvia_reload_keys();
	be32enc(authctr + 16 + 4*3, ++c3);
	aesvia_encN(enc, authctr + 16, authctr + 16, 1, cw0);
	for (;; in += 16, out += 16) {
		xor128(out, authctr + 16, in);
		xor128(authctr, authctr, out);
		if ((nbytes -= 16) == 0)
			break;
		le32enc(authctr + 16 + 4*0, c0);
		le32enc(authctr + 16 + 4*1, c1);
		le32enc(authctr + 16 + 4*2, c2);
		be32enc(authctr + 16 + 4*3, ++c3);
		aesvia_encN(enc, authctr, authctr, 2, cw0);
	}
	aesvia_encN(enc, authctr, authctr, 1, cw0);
	fpu_kern_leave();

	if ((uintptr_t)authctr0 & 0xf) {
		memcpy(authctr0, authctrbuf, 16);
		explicit_memset(authctrbuf, 0, sizeof authctrbuf);
	}

	le32enc(authctr0 + 16 + 4*0, c0);
	le32enc(authctr0 + 16 + 4*1, c1);
	le32enc(authctr0 + 16 + 4*2, c2);
	be32enc(authctr0 + 16 + 4*3, c3);
}

static int
aesvia_probe(void)
{

	/* Verify that the CPU advertises VIA ACE support.  */
#ifdef _KERNEL
	if ((cpu_feature[4] & CPUID_VIA_HAS_ACE) == 0)
		return -1;
#else
	/*
	 * From the VIA PadLock Programming Guide:
	 * http://linux.via.com.tw/support/beginDownload.action?eleid=181&fid=261
	 */
	unsigned eax, ebx, ecx, edx;
	if (!__get_cpuid(0, &eax, &ebx, &ecx, &edx))
		return -1;
	if (ebx != signature_CENTAUR_ebx ||
	    ecx != signature_CENTAUR_ecx ||
	    edx != signature_CENTAUR_edx)
		return -1;
	if (eax < 0xc0000000)
		return -1;
	if (!__get_cpuid(0xc0000000, &eax, &ebx, &ecx, &edx))
		return -1;
	if (eax < 0xc0000001)
		return -1;
	if (!__get_cpuid(0xc0000001, &eax, &ebx, &ecx, &edx))
		return -1;
	/* Check whether ACE or ACE2 is both supported and enabled.  */
	if ((edx & 0x000000c0) != 0x000000c0 ||
	    (edx & 0x00000300) != 0x00000300)
		return -1;
#endif

	/* Verify that our XTS tweak update logic works.  */
	if (aesvia_xts_update_selftest())
		return -1;

	/* Success!  */
	return 0;
}

struct aes_impl aes_via_impl = {
	.ai_name = "VIA ACE",
	.ai_probe = aesvia_probe,
	.ai_setenckey = aesvia_setenckey,
	.ai_setdeckey = aesvia_setdeckey,
	.ai_enc = aesvia_enc,
	.ai_dec = aesvia_dec,
	.ai_cbc_enc = aesvia_cbc_enc,
	.ai_cbc_dec = aesvia_cbc_dec,
	.ai_xts_enc = aesvia_xts_enc,
	.ai_xts_dec = aesvia_xts_dec,
	.ai_cbcmac_update1 = aesvia_cbcmac_update1,
	.ai_ccm_enc1 = aesvia_ccm_enc1,
	.ai_ccm_dec1 = aesvia_ccm_dec1,
};
