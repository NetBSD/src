/*	$NetBSD: aes_ccm.c,v 1.5 2020/08/10 06:27:29 rin Exp $	*/

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
 * AES-CCM, as defined in:
 *
 *	D. Whiting, R. Housley, and N. Ferguson, `Counter with CBC-MAC
 *	(CCM)', IETF RFC 3610, September 2003.
 *	https://tools.ietf.org/html/rfc3610
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: aes_ccm.c,v 1.5 2020/08/10 06:27:29 rin Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_ccm.h>
#include <crypto/aes/aes_impl.h>

static inline void
xor(uint8_t *x, const uint8_t *a, const uint8_t *b, size_t n)
{

	while (n --> 0)
		*x++ = *a++ ^ *b++;
}

/* RFC 3610, §2.2 Authentication */
#define	CCM_AFLAGS_ADATA	__BIT(6)
#define	CCM_AFLAGS_M		__BITS(5,3)
#define	CCM_AFLAGS_L		__BITS(2,0)

/* RFC 3610, §2.3 Encryption */
#define	CCM_EFLAGS_L		__BITS(2,0)

static void
aes_ccm_inc(struct aes_ccm *C)
{
	uint8_t *ctr = C->authctr + 16;

	KASSERT(C->L == 2);
	if (++ctr[15] == 0 && ++ctr[14] == 0)
		panic("AES-CCM overflow");
}

static void
aes_ccm_zero_ctr(struct aes_ccm *C)
{
	uint8_t *ctr = C->authctr + 16;

	KASSERT(C->L == 2);
	ctr[14] = ctr[15] = 0;
}

void
aes_ccm_init(struct aes_ccm *C, unsigned nr, const struct aesenc *enc,
    unsigned L, unsigned M,
    const uint8_t *nonce, unsigned noncelen, const void *ad, size_t adlen,
    size_t mlen)
{
	const uint8_t *adp = ad;
	uint8_t *auth = C->authctr;
	uint8_t *ctr = C->authctr + 16;
	unsigned i;

	KASSERT(L == 2);
	KASSERT(M % 2 == 0);
	KASSERT(M >= 4);
	KASSERT(M <= 16);
	KASSERT(noncelen == 15 - L);

	C->enc = enc;
	C->nr = nr;
	C->L = L;
	C->M = M;
	C->mlen = C->mleft = mlen;

	/* Encode B0, the initial authenticated data block.  */
	auth[0] = __SHIFTIN(adlen == 0 ? 0 : 1, CCM_AFLAGS_ADATA);
	auth[0] |= __SHIFTIN((M - 2)/2, CCM_AFLAGS_M);
	auth[0] |= __SHIFTIN(L - 1, CCM_AFLAGS_L);
	memcpy(auth + 1, nonce, noncelen);
	for (i = 0; i < L; i++, mlen >>= 8) {
		KASSERT(i < 16 - 1 - noncelen);
		auth[16 - i - 1] = mlen & 0xff;
	}
	aes_enc(enc, auth, auth, C->nr);

	/* Process additional authenticated data, if any.  */
	if (adlen) {
		/* Encode the length according to the table on p. 4.  */
		if (adlen < 0xff00) {
			auth[0] ^= adlen >> 8;
			auth[1] ^= adlen;
			i = 2;
		} else if (adlen < 0xffffffff) {
			auth[0] ^= 0xff;
			auth[1] ^= 0xfe;
			auth[2] ^= adlen >> 24;
			auth[3] ^= adlen >> 16;
			auth[4] ^= adlen >> 8;
			auth[5] ^= adlen;
			i = 6;
#if SIZE_MAX > 0xffffffffU
		} else {
			CTASSERT(SIZE_MAX <= 0xffffffffffffffff);
			auth[0] ^= 0xff;
			auth[1] ^= 0xff;
			auth[2] ^= adlen >> 56;
			auth[3] ^= adlen >> 48;
			auth[4] ^= adlen >> 40;
			auth[5] ^= adlen >> 32;
			auth[6] ^= adlen >> 24;
			auth[7] ^= adlen >> 16;
			auth[8] ^= adlen >> 8;
			auth[9] ^= adlen;
			i = 10;
#endif
		}

		/* Fill out the partial block if we can, and encrypt.  */
		xor(auth + i, auth + i, adp, MIN(adlen, 16 - i));
		adp += MIN(adlen, 16 - i);
		adlen -= MIN(adlen, 16 - i);
		aes_enc(enc, auth, auth, C->nr);

		/* If there was anything more, process 16 bytes at a time.  */
		if (adlen - (adlen % 16)) {
			aes_cbcmac_update1(enc, adp, adlen - (adlen % 16),
			    auth, C->nr);
			adlen %= 16;
		}

		/*
		 * If there's anything at the end, enter it in (padded
		 * with zeros, which is a no-op) and process it.
		 */
		if (adlen) {
			xor(auth, auth, adp, adlen);
			aes_enc(enc, auth, auth, C->nr);
		}
	}

	/* Set up the AES input for AES-CTR encryption.  */
	ctr[0] = __SHIFTIN(L - 1, CCM_EFLAGS_L);
	memcpy(ctr + 1, nonce, noncelen);
	memset(ctr + 1 + noncelen, 0, 16 - 1 - noncelen);

	/* Start on a block boundary.  */
	C->i = 0;
}

void
aes_ccm_enc(struct aes_ccm *C, const void *in, void *out, size_t nbytes)
{
	uint8_t *auth = C->authctr;
	uint8_t *ctr = C->authctr + 16;
	const uint8_t *p = in;
	uint8_t *q = out;

	KASSERTMSG(C->i != ~0u,
	    "%s not allowed after message complete", __func__);
	KASSERTMSG(nbytes <= C->mleft,
	    "message too long: promised %zu bytes, processing >=%zu",
	    C->mlen, C->mlen - C->mleft + nbytes);
	C->mleft -= nbytes;

	/* Finish a partial block if it was already started.  */
	if (C->i) {
		unsigned m = MIN(16 - C->i, nbytes);

		xor(auth + C->i, auth + C->i, p, m);
		xor(q, C->out + C->i, p, m);
		C->i += m;
		p += m;
		q += m;
		nbytes -= m;

		if (C->i == 16) {
			/* Finished a block; authenticate it.  */
			aes_enc(C->enc, auth, auth, C->nr);
			C->i = 0;
		} else {
			/* Didn't finish block, must be done with input. */
			KASSERT(nbytes == 0);
			return;
		}
	}

	/* Process 16 bytes at a time.  */
	if (nbytes - (nbytes % 16)) {
		aes_ccm_enc1(C->enc, p, q, nbytes - (nbytes % 16), auth,
		    C->nr);
		p += nbytes - (nbytes % 16);
		q += nbytes - (nbytes % 16);
		nbytes %= 16;
	}

	/* Incorporate any <16-byte unit as a partial block.  */
	if (nbytes) {
		/* authenticate */
		xor(auth, auth, p, nbytes);

		/* encrypt */
		aes_ccm_inc(C);
		aes_enc(C->enc, ctr, C->out, C->nr);
		xor(q, C->out, p, nbytes);

		C->i = nbytes;
	}
}

void
aes_ccm_dec(struct aes_ccm *C, const void *in, void *out, size_t nbytes)
{
	uint8_t *auth = C->authctr;
	uint8_t *ctr = C->authctr + 16;
	const uint8_t *p = in;
	uint8_t *q = out;

	KASSERTMSG(C->i != ~0u,
	    "%s not allowed after message complete", __func__);
	KASSERTMSG(nbytes <= C->mleft,
	    "message too long: promised %zu bytes, processing >=%zu",
	    C->mlen, C->mlen - C->mleft + nbytes);
	C->mleft -= nbytes;

	/* Finish a partial block if it was already started.  */
	if (C->i) {
		unsigned m = MIN(16 - C->i, nbytes);

		xor(q, C->out + C->i, p, m);
		xor(auth + C->i, auth + C->i, q, m);
		C->i += m;
		p += m;
		q += m;
		nbytes -= m;

		if (C->i == 16) {
			/* Finished a block; authenticate it.  */
			aes_enc(C->enc, auth, auth, C->nr);
			C->i = 0;
		} else {
			/* Didn't finish block, must be done with input. */
			KASSERT(nbytes == 0);
			return;
		}
	}

	/* Process 16 bytes at a time.  */
	if (nbytes - (nbytes % 16)) {
		aes_ccm_dec1(C->enc, p, q, nbytes - (nbytes % 16), auth,
		    C->nr);
		p += nbytes - (nbytes % 16);
		q += nbytes - (nbytes % 16);
		nbytes %= 16;
	}

	/* Incorporate any <16-byte unit as a partial block.  */
	if (nbytes) {
		/* decrypt */
		aes_ccm_inc(C);
		aes_enc(C->enc, ctr, C->out, C->nr);
		xor(q, C->out, p, nbytes);

		/* authenticate */
		xor(auth, auth, q, nbytes);

		C->i = nbytes;
	}
}

void
#if defined(__m68k__) && __GNUC_PREREQ__(8, 0)
__attribute__((__optimize__("O0")))
#endif
aes_ccm_tag(struct aes_ccm *C, void *out)
{
	uint8_t *auth = C->authctr;
	const uint8_t *ctr = C->authctr + 16;

	KASSERTMSG(C->mleft == 0,
	    "message too short: promised %zu bytes, processed %zu",
	    C->mlen, C->mlen - C->mleft);

	/* Zero-pad and munch up a partial block, if any.  */
	if (C->i)
		aes_enc(C->enc, auth, auth, C->nr);

	/* Zero the counter and generate a pad for the tag.  */
	aes_ccm_zero_ctr(C);
	aes_enc(C->enc, ctr, C->out, C->nr);

	/* Copy out as many bytes as requested.  */
	xor(out, C->out, auth, C->M);

	C->i = ~0u;		/* paranoia: prevent future misuse */
}

int
aes_ccm_verify(struct aes_ccm *C, const void *tag)
{
	uint8_t expected[16];
	int result;

	aes_ccm_tag(C, expected);
	result = consttime_memequal(tag, expected, C->M);
	explicit_memset(expected, 0, sizeof expected);

	return result;
}

/* RFC 3610, §8 */

static const uint8_t keyC[16] = {
	0xc0,0xc1,0xc2,0xc3, 0xc4,0xc5,0xc6,0xc7,
	0xc8,0xc9,0xca,0xcb, 0xcc,0xcd,0xce,0xcf,
};

static const uint8_t keyD[16] = {
	0xd7,0x82,0x8d,0x13, 0xb2,0xb0,0xbd,0xc3,
	0x25,0xa7,0x62,0x36, 0xdf,0x93,0xcc,0x6b,
};

static const uint8_t ptxt_seq[] = {
	0x00,0x01,0x02,0x03, 0x04,0x05,0x06,0x07,
	0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17,
	0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e,0x1f,
	0x20,
};

static const uint8_t ptxt_rand[] = {
	0x6e,0x37,0xa6,0xef, 0x54,0x6d,0x95,0x5d,
	0x34,0xab,0x60,0x59, 0xab,0xf2,0x1c,0x0b,
	0x02,0xfe,0xb8,0x8f, 0x85,0x6d,0xf4,0xa3,
	0x73,0x81,0xbc,0xe3, 0xcc,0x12,0x85,0x17,
	0xd4,
};

static const struct {
	const uint8_t *key;
	size_t noncelen;
	const uint8_t nonce[13];
	size_t adlen;
	const uint8_t *ad;
	size_t mlen;
	const uint8_t *ptxt;
	unsigned M;
	const uint8_t tag[16];
	const uint8_t *ctxt;
} T[] = {
	[0] = {		/* Packet Vector #1, p. 11 */
		.key = keyC,
		.nonce = {
			0x00,0x00,0x00,0x03, 0x02,0x01,0x00,0xa0,
			0xa1,0xa2,0xa3,0xa4, 0xa5,
		},
		.adlen = 8,
		.ad = ptxt_seq,
		.mlen = 23,
		.ptxt = ptxt_seq + 8,
		.M = 8,
		.tag = {0x17,0xe8,0xd1,0x2c,0xfd, 0xf9,0x26,0xe0},
		.ctxt = (const uint8_t[23]) {
			0x58,0x8c,0x97,0x9a, 0x61,0xc6,0x63,0xd2,
			0xf0,0x66,0xd0,0xc2, 0xc0,0xf9,0x89,0x80,
			0x6d,0x5f,0x6b,0x61, 0xda,0xc3,0x84,
		},
	},
	[1] = {			/* Packet Vector #2, p. 11 */
		.key = keyC,
		.nonce = {
			0x00,0x00,0x00,0x04, 0x03,0x02,0x01,0xa0,
			0xa1,0xa2,0xa3,0xa4, 0xa5,
		},
		.adlen = 8,
		.ad = ptxt_seq,
		.mlen = 24,
		.ptxt = ptxt_seq + 8,
		.M = 8,
		.tag = {0xa0,0x91,0xd5,0x6e, 0x10,0x40,0x09,0x16},
		.ctxt = (const uint8_t[24]) {
			0x72,0xc9,0x1a,0x36, 0xe1,0x35,0xf8,0xcf,
			0x29,0x1c,0xa8,0x94, 0x08,0x5c,0x87,0xe3,
			0xcc,0x15,0xc4,0x39, 0xc9,0xe4,0x3a,0x3b,
		},
	},
	[2] = {			/* Packet Vector #3, p. 12 */
		.key = keyC,
		.nonce = {
			0x00,0x00,0x00,0x05, 0x04,0x03,0x02,0xa0,
			0xa1,0xa2,0xa3,0xa4, 0xa5,
		},
		.adlen = 8,
		.ad = ptxt_seq,
		.mlen = 25,
		.ptxt = ptxt_seq + 8,
		.M = 8,
		.tag = {0x4a,0xda,0xa7,0x6f, 0xbd,0x9f,0xb0,0xc5},
		.ctxt = (const uint8_t[25]) {
			0x51,0xb1,0xe5,0xf4, 0x4a,0x19,0x7d,0x1d,
			0xa4,0x6b,0x0f,0x8e, 0x2d,0x28,0x2a,0xe8,
			0x71,0xe8,0x38,0xbb, 0x64,0xda,0x85,0x96,
			0x57,
		},
	},
	[3] = {			/* Packet Vector #4, p. 13 */
		.key = keyC,
		.nonce = {
			0x00,0x00,0x00,0x06, 0x05,0x04,0x03,0xa0,
			0xa1,0xa2,0xa3,0xa4, 0xa5,
		},
		.adlen = 12,
		.ad = ptxt_seq,
		.mlen = 19,
		.ptxt = ptxt_seq + 12,
		.M = 8,
		.tag = {0x96,0xc8,0x61,0xb9, 0xc9,0xe6,0x1e,0xf1},
		.ctxt = (const uint8_t[19]) {
			0xa2,0x8c,0x68,0x65, 0x93,0x9a,0x9a,0x79,
			0xfa,0xaa,0x5c,0x4c, 0x2a,0x9d,0x4a,0x91,
			0xcd,0xac,0x8c,
		},
	},
	[4] = {			/* Packet Vector #5, p. 13 */
		.key = keyC,
		.nonce = {
			0x00,0x00,0x00,0x07, 0x06,0x05,0x04,0xa0,
			0xa1,0xa2,0xa3,0xa4, 0xa5,
		},
		.adlen = 12,
		.ad = ptxt_seq,
		.mlen = 20,
		.ptxt = ptxt_seq + 12,
		.M = 8,
		.tag = {0x51,0xe8,0x3f,0x07, 0x7d,0x9c,0x2d,0x93},
		.ctxt = (const uint8_t[20]) {
			0xdc,0xf1,0xfb,0x7b, 0x5d,0x9e,0x23,0xfb,
			0x9d,0x4e,0x13,0x12, 0x53,0x65,0x8a,0xd8,
			0x6e,0xbd,0xca,0x3e,
		},
	},
	[5] = {			/* Packet Vector #6, p. 13 */
		.key = keyC,
		.nonce = {
			0x00,0x00,0x00,0x08, 0x07,0x06,0x05,0xa0,
			0xa1,0xa2,0xa3,0xa4, 0xa5,
		},
		.adlen = 12,
		.ad = ptxt_seq,
		.mlen = 21,
		.ptxt = ptxt_seq + 12,
		.M = 8,
		.tag = {0x40,0x5a,0x04,0x43, 0xac,0x91,0xcb,0x94},
		.ctxt = (const uint8_t[21]) {
			0x6f,0xc1,0xb0,0x11, 0xf0,0x06,0x56,0x8b,
			0x51,0x71,0xa4,0x2d, 0x95,0x3d,0x46,0x9b,
			0x25,0x70,0xa4,0xbd, 0x87,
		},
	},
	[6] = {			/* Packet Vector #24 */
		.key = keyD,
		.nonce = {
			0x00,0x8d,0x49,0x3b, 0x30,0xae,0x8b,0x3c,
			0x96,0x96,0x76,0x6c, 0xfa,
		},
		.adlen = 12,
		.ad = ptxt_rand,
		.mlen = 21,
		.ptxt = ptxt_rand + 12,
		.M = 10,
		.tag = {0x6d,0xce,0x9e,0x82, 0xef,0xa1,0x6d,0xa6, 0x20,0x59},
		.ctxt = (const uint8_t[21]) {
			0xf3,0x29,0x05,0xb8, 0x8a,0x64,0x1b,0x04,
			0xb9,0xc9,0xff,0xb5, 0x8c,0xc3,0x90,0x90,
			0x0f,0x3d,0xa1,0x2a, 0xb1,
		},
	},
};

int
aes_ccm_selftest(void)
{
	const unsigned L = 2;
	const unsigned noncelen = 13;
	struct aesenc enc, *AE = &enc;
	struct aes_ccm ccm, *C = &ccm;
	uint8_t buf[33 + 2], *bufp = buf + 1;
	uint8_t tag[16 + 2], *tagp = tag + 1;
	unsigned i;
	int result = 0;

	bufp[-1] = bufp[33] = 0x1a;
	tagp[-1] = tagp[16] = 0x53;

	for (i = 0; i < __arraycount(T); i++) {
		const unsigned nr = aes_setenckey128(AE, T[i].key);

		/* encrypt and authenticate */
		aes_ccm_init(C, nr, AE, L, T[i].M, T[i].nonce, noncelen,
		    T[i].ad, T[i].adlen, T[i].mlen);
		aes_ccm_enc(C, T[i].ptxt, bufp, 1);
		aes_ccm_enc(C, T[i].ptxt + 1, bufp + 1, 2);
		aes_ccm_enc(C, T[i].ptxt + 3, bufp + 3, T[i].mlen - 4);
		aes_ccm_enc(C, T[i].ptxt + T[i].mlen - 1,
		    bufp + T[i].mlen - 1, 1);
		aes_ccm_tag(C, tagp);
		if (memcmp(bufp, T[i].ctxt, T[i].mlen)) {
			char name[32];
			snprintf(name, sizeof name, "%s: ctxt %u", __func__,
			    i);
			hexdump(printf, name, bufp, T[i].mlen);
			result = -1;
		}
		if (memcmp(tagp, T[i].tag, T[i].M)) {
			char name[32];
			snprintf(name, sizeof name, "%s: tag %u", __func__, i);
			hexdump(printf, name, tagp, T[i].M);
			result = -1;
		}

		/* decrypt and verify */
		aes_ccm_init(C, nr, AE, L, T[i].M, T[i].nonce, noncelen,
		    T[i].ad, T[i].adlen, T[i].mlen);
		aes_ccm_dec(C, T[i].ctxt, bufp, 1);
		aes_ccm_dec(C, T[i].ctxt + 1, bufp + 1, 2);
		aes_ccm_dec(C, T[i].ctxt + 3, bufp + 3, T[i].mlen - 4);
		aes_ccm_dec(C, T[i].ctxt + T[i].mlen - 1,
		    bufp + T[i].mlen - 1, 1);
		if (!aes_ccm_verify(C, T[i].tag)) {
			printf("%s: verify %u failed\n", __func__, i);
			result = -1;
		}
		if (memcmp(bufp, T[i].ptxt, T[i].mlen)) {
			char name[32];
			snprintf(name, sizeof name, "%s: ptxt %u", __func__,
			    i);
			hexdump(printf, name, bufp, T[i].mlen);
			result = -1;
		}

		/* decrypt and verify with a bit flipped */
		memcpy(tagp, T[i].tag, T[i].M);
		tagp[0] ^= 0x80;
		aes_ccm_init(C, nr, AE, L, T[i].M, T[i].nonce, noncelen,
		    T[i].ad, T[i].adlen, T[i].mlen);
		aes_ccm_dec(C, T[i].ctxt, bufp, 1);
		aes_ccm_dec(C, T[i].ctxt + 1, bufp + 1, 2);
		aes_ccm_dec(C, T[i].ctxt + 3, bufp + 3, T[i].mlen - 4);
		aes_ccm_dec(C, T[i].ctxt + T[i].mlen - 1,
		    bufp + T[i].mlen - 1, 1);
		if (aes_ccm_verify(C, tagp)) {
			printf("%s: forgery %u succeeded\n", __func__, i);
			result = -1;
		}
	}

	if (bufp[-1] != 0x1a || bufp[33] != 0x1a) {
		printf("%s: buffer overrun\n", __func__);
		result = -1;
	}
	if (tagp[-1] != 0x53 || tagp[16] != 0x53) {
		printf("%s: tag overrun\n", __func__);
		result = -1;
	}

	return result;
}

/* XXX provisional hack */
#include <sys/module.h>

MODULE(MODULE_CLASS_MISC, aes_ccm, "aes");

static int
aes_ccm_modcmd(modcmd_t cmd, void *opaque)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (aes_ccm_selftest())
			return EIO;
		aprint_verbose("aes_ccm: self-test passed\n");
		return 0;
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}
