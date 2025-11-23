/*	$NetBSD: aes_bear64.c,v 1.1 2025/11/23 22:44:13 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(1, "$NetBSD: aes_bear64.c,v 1.1 2025/11/23 22:44:13 riastradh Exp $");

#include <sys/types.h>
#include <sys/endian.h>

#ifdef _KERNEL
#include <sys/systm.h>
#else
#include <assert.h>
#include <err.h>
#include <string.h>
#define	KASSERT			assert
#define	panic(fmt, args...)	err(1, fmt, args)
#endif

#include <crypto/aes/aes.h>
#include <crypto/aes/aes_bear64.h>
#include <crypto/aes/aes_impl.h>

static void
aesbear64_setkey(uint64_t rk[static 30], const void *key, uint32_t nrounds)
{
	size_t key_len;

	switch (nrounds) {
	case 10:
		key_len = 16;
		break;
	case 12:
		key_len = 24;
		break;
	case 14:
		key_len = 32;
		break;
	default:
		panic("invalid AES nrounds: %u", nrounds);
	}

	br_aes_ct64_keysched(rk, key, key_len);
}

static void
aesbear64_setenckey(struct aesenc *enc, const uint8_t *key, uint32_t nrounds)
{

	aesbear64_setkey(enc->aese_aes.aes_rk64, key, nrounds);
}

static void
aesbear64_setdeckey(struct aesdec *dec, const uint8_t *key, uint32_t nrounds)
{

	/*
	 * BearSSL computes InvMixColumns on the fly -- no need for
	 * distinct decryption round keys.
	 */
	aesbear64_setkey(dec->aesd_aes.aes_rk64, key, nrounds);
}

static void
aesbear64_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Load input block interleaved with garbage blocks.  */
	w[0] = le32dec(in + 4*0);
	w[1] = le32dec(in + 4*1);
	w[2] = le32dec(in + 4*2);
	w[3] = le32dec(in + 4*3);
	br_aes_ct64_interleave_in(&q[0], &q[4], w);
	q[1] = q[2] = q[3] = 0;
	q[5] = q[6] = q[7] = 0;

	/* Transform to bitslice, encrypt, transform from bitslice.  */
	br_aes_ct64_ortho(q);
	br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
	br_aes_ct64_ortho(q);

	/* Store output block.  */
	br_aes_ct64_interleave_out(w, q[0], q[4]);
	le32enc(out + 4*0, w[0]);
	le32enc(out + 4*1, w[1]);
	le32enc(out + 4*2, w[2]);
	le32enc(out + 4*3, w[3]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static void
aesbear64_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk64);

	/* Load input block interleaved with garbage blocks.  */
	w[0] = le32dec(in + 4*0);
	w[1] = le32dec(in + 4*1);
	w[2] = le32dec(in + 4*2);
	w[3] = le32dec(in + 4*3);
	br_aes_ct64_interleave_in(&q[0], &q[4], w);
	q[1] = q[2] = q[3] = 0;
	q[5] = q[6] = q[7] = 0;

	/* Transform to bitslice, decrypt, transform from bitslice.  */
	br_aes_ct64_ortho(q);
	br_aes_ct64_bitslice_decrypt(nrounds, sk_exp, q);
	br_aes_ct64_ortho(q);

	/* Store output block.  */
	br_aes_ct64_interleave_out(w, q[0], q[4]);
	le32enc(out + 4*0, w[0]);
	le32enc(out + 4*1, w[1]);
	le32enc(out + 4*2, w[2]);
	le32enc(out + 4*3, w[3]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static void
aesbear64_cbc_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];
	uint32_t cv0, cv1, cv2, cv3;

	KASSERT(nbytes % 16 == 0);

	/* Skip if there's nothing to do.  */
	if (nbytes == 0)
		return;

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Initialize garbage blocks.  */
	q[1] = q[2] = q[3] = 0;
	q[5] = q[6] = q[7] = 0;

	/* Load IV.  */
	cv0 = le32dec(iv + 4*0);
	cv1 = le32dec(iv + 4*1);
	cv2 = le32dec(iv + 4*2);
	cv3 = le32dec(iv + 4*3);

	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		/* Load input block and apply CV.  */
		w[0] = cv0 ^ le32dec(in + 4*0);
		w[1] = cv1 ^ le32dec(in + 4*1);
		w[2] = cv2 ^ le32dec(in + 4*2);
		w[3] = cv3 ^ le32dec(in + 4*3);
		br_aes_ct64_interleave_in(&q[0], &q[4], w);

		/* Transform to bitslice, encrypt, transform from bitslice.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		/* Remember ciphertext as CV and store output block.  */
		br_aes_ct64_interleave_out(w, q[0], q[4]);
		cv0 = w[0];
		cv1 = w[1];
		cv2 = w[2];
		cv3 = w[3];
		le32enc(out + 4*0, cv0);
		le32enc(out + 4*1, cv1);
		le32enc(out + 4*2, cv2);
		le32enc(out + 4*3, cv3);
	}

	/* Store updated IV.  */
	le32enc(iv + 4*0, cv0);
	le32enc(iv + 4*1, cv1);
	le32enc(iv + 4*2, cv2);
	le32enc(iv + 4*3, cv3);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static void
aesbear64_cbc_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t iv[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];
	uint32_t cv0, cv1, cv2, cv3, iv0, iv1, iv2, iv3;
	unsigned i;

	KASSERT(nbytes % 16 == 0);

	/* Skip if there's nothing to do.  */
	if (nbytes == 0)
		return;

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk64);

	/* Load the IV.  */
	iv0 = le32dec(iv + 4*0);
	iv1 = le32dec(iv + 4*1);
	iv2 = le32dec(iv + 4*2);
	iv3 = le32dec(iv + 4*3);

	/* Load the last cipher block.  */
	cv0 = le32dec(in + nbytes - 16 + 4*0);
	cv1 = le32dec(in + nbytes - 16 + 4*1);
	cv2 = le32dec(in + nbytes - 16 + 4*2);
	cv3 = le32dec(in + nbytes - 16 + 4*3);

	/* Store the updated IV.  */
	le32enc(iv + 4*0, cv0);
	le32enc(iv + 4*1, cv1);
	le32enc(iv + 4*2, cv2);
	le32enc(iv + 4*3, cv3);

	/* Handle the last cipher block separately if odd number.  */
	if (nbytes % 64) {
		unsigned n = (nbytes % 64)/16;

		KASSERT(n == 1 || n == 2 || n == 3);

		for (i = 4; i --> n;)
			q[i] = q[4 + i] = 0;
		KASSERT(i == n - 1);
		w[0] = cv0;	/* le32dec(in + nbytes - 16*n + 16*i + 4*0) */
		w[1] = cv1;	/* le32dec(in + nbytes - 16*n + 16*i + 4*1) */
		w[2] = cv2;	/* le32dec(in + nbytes - 16*n + 16*i + 4*2) */
		w[3] = cv3;	/* le32dec(in + nbytes - 16*n + 16*i + 4*3) */
		br_aes_ct64_interleave_in(&q[i], &q[4 + i], w);
		while (i --> 0) {
			w[0] = le32dec(in + nbytes - 16*n + 16*i + 4*0);
			w[1] = le32dec(in + nbytes - 16*n + 16*i + 4*1);
			w[2] = le32dec(in + nbytes - 16*n + 16*i + 4*2);
			w[3] = le32dec(in + nbytes - 16*n + 16*i + 4*3);
			br_aes_ct64_interleave_in(&q[i], &q[4 + i], w);
		}

		/* Decrypt.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_decrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		for (i = n; i --> 1;) {
			br_aes_ct64_interleave_out(w, q[i], q[4 + i]);
			cv0 = le32dec(in + nbytes - 16*n + 16*(i - 1) + 4*0);
			cv1 = le32dec(in + nbytes - 16*n + 16*(i - 1) + 4*1);
			cv2 = le32dec(in + nbytes - 16*n + 16*(i - 1) + 4*2);
			cv3 = le32dec(in + nbytes - 16*n + 16*(i - 1) + 4*3);
			le32enc(out + nbytes - 16*n + 16*i + 4*0, w[0] ^ cv0);
			le32enc(out + nbytes - 16*n + 16*i + 4*1, w[1] ^ cv1);
			le32enc(out + nbytes - 16*n + 16*i + 4*2, w[2] ^ cv2);
			le32enc(out + nbytes - 16*n + 16*i + 4*3, w[3] ^ cv3);
		}
		br_aes_ct64_interleave_out(w, q[0], q[4]);

		/* If this was the only cipher block, we're done.  */
		nbytes -= nbytes % 64;
		if (nbytes == 0)
			goto out;

		/*
		 * Otherwise, load up the previous cipher block, and
		 * store the output block.
		 */
		cv0 = le32dec(in + nbytes - 16 + 4*0);
		cv1 = le32dec(in + nbytes - 16 + 4*1);
		cv2 = le32dec(in + nbytes - 16 + 4*2);
		cv3 = le32dec(in + nbytes - 16 + 4*3);
		le32enc(out + nbytes + 4*0, cv0 ^ w[0]);
		le32enc(out + nbytes + 4*1, cv1 ^ w[1]);
		le32enc(out + nbytes + 4*2, cv2 ^ w[2]);
		le32enc(out + nbytes + 4*3, cv3 ^ w[3]);
	}

	for (;;) {
		KASSERT(nbytes >= 64);

		/* Load the input blocks.  */
		w[0] = cv0;	/* le32dec(in + nbytes - 64 + 16*i + 4*0) */
		w[1] = cv1;	/* le32dec(in + nbytes - 64 + 16*i + 4*1) */
		w[2] = cv2;	/* le32dec(in + nbytes - 64 + 16*i + 4*2) */
		w[3] = cv3;	/* le32dec(in + nbytes - 64 + 16*i + 4*3) */
		br_aes_ct64_interleave_in(&q[3], &q[7], w);
		for (i = 3; i --> 0;) {
			w[0] = le32dec(in + nbytes - 64 + 16*i + 4*0);
			w[1] = le32dec(in + nbytes - 64 + 16*i + 4*1);
			w[2] = le32dec(in + nbytes - 64 + 16*i + 4*2);
			w[3] = le32dec(in + nbytes - 64 + 16*i + 4*3);
			br_aes_ct64_interleave_in(&q[i], &q[4 + i], w);
		}

		/* Decrypt.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_decrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		/* Store the upper output blocks.  */
		for (i = 4; i --> 1;) {
			br_aes_ct64_interleave_out(w, q[i], q[4 + i]);
			cv0 = le32dec(in + nbytes - 64 + 16*(i - 1) + 4*0);
			cv1 = le32dec(in + nbytes - 64 + 16*(i - 1) + 4*1);
			cv2 = le32dec(in + nbytes - 64 + 16*(i - 1) + 4*2);
			cv3 = le32dec(in + nbytes - 64 + 16*(i - 1) + 4*3);
			le32enc(out + nbytes - 64 + 16*i + 4*0, w[0] ^ cv0);
			le32enc(out + nbytes - 64 + 16*i + 4*1, w[1] ^ cv1);
			le32enc(out + nbytes - 64 + 16*i + 4*2, w[2] ^ cv2);
			le32enc(out + nbytes - 64 + 16*i + 4*3, w[3] ^ cv3);
		}

		/* Prepare the first output block.  */
		br_aes_ct64_interleave_out(w, q[0], q[4]);

		/* Stop if we've reached the first output block.  */
		nbytes -= 64;
		if (nbytes == 0)
			goto out;

		/*
		 * Load the preceding cipher block, and apply it as the
		 * chaining value to this one.
		 */
		cv0 = le32dec(in + nbytes - 16 + 4*0);
		cv1 = le32dec(in + nbytes - 16 + 4*1);
		cv2 = le32dec(in + nbytes - 16 + 4*2);
		cv3 = le32dec(in + nbytes - 16 + 4*3);
		le32enc(out + nbytes + 4*0, w[0] ^ cv0);
		le32enc(out + nbytes + 4*1, w[1] ^ cv1);
		le32enc(out + nbytes + 4*2, w[2] ^ cv2);
		le32enc(out + nbytes + 4*3, w[3] ^ cv3);
	}

out:	/* Store the first output block.  */
	le32enc(out + 4*0, w[0] ^ iv0);
	le32enc(out + 4*1, w[1] ^ iv1);
	le32enc(out + 4*2, w[2] ^ iv2);
	le32enc(out + 4*3, w[3] ^ iv3);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static inline void
aesbear64_xts_update(uint32_t *t0, uint32_t *t1, uint32_t *t2, uint32_t *t3)
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
aesbear64_xts_update_selftest(void)
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
		aesbear64_xts_update(&t0, &t1, &t2, &t3);
		if (t0 != cases[i].out[0] ||
		    t1 != cases[i].out[1] ||
		    t2 != cases[i].out[2] ||
		    t3 != cases[i].out[3])
			return -1;
	}

	/* Success!  */
	return 0;
}

static void
aesbear64_xts_enc(const struct aesenc *enc, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];
	uint32_t t0, t1, t2, t3, u0, u1, u2, u3;
	unsigned i;

	KASSERT(nbytes % 16 == 0);

	/* Skip if there's nothing to do.  */
	if (nbytes == 0)
		return;

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Load tweak.  */
	t0 = le32dec(tweak + 4*0);
	t1 = le32dec(tweak + 4*1);
	t2 = le32dec(tweak + 4*2);
	t3 = le32dec(tweak + 4*3);

	/* Handle the first blocks separately if odd number.  */
	if (nbytes % 64) {
		unsigned n = (nbytes % 64)/16;

		/* Load up the first blocks and garbage.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < n; i++) {
			w[0] = le32dec(in + 16*i + 4*0) ^ u0;
			w[1] = le32dec(in + 16*i + 4*1) ^ u1;
			w[2] = le32dec(in + 16*i + 4*2) ^ u2;
			w[3] = le32dec(in + 16*i + 4*3) ^ u3;
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
			br_aes_ct64_interleave_in(&q[i], &q[4 + i], w);
		}
		for (; i < 4; i++)
			q[i] = q[4 + i] = 0;

		/* Encrypt up to three blocks.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		/* Store up to three blocks.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < n; i++) {
			br_aes_ct64_interleave_out(w, q[i], q[4 + i]);
			le32enc(out + 16*i + 4*0, w[0] ^ u0);
			le32enc(out + 16*i + 4*1, w[1] ^ u1);
			le32enc(out + 16*i + 4*2, w[2] ^ u2);
			le32enc(out + 16*i + 4*3, w[3] ^ u3);
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
		}

		/* Advance to the next block.  */
		t0 = u0, t1 = u1, t2 = u2, t3 = u3;
		if ((nbytes -= 16*n) == 0)
			goto out;
		in += 16*n;
		out += 16*n;
	}

	do {
		KASSERT(nbytes >= 64);

		/* Load four blocks.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < 4; i++) {
			w[0] = le32dec(in + 16*i + 4*0) ^ u0;
			w[1] = le32dec(in + 16*i + 4*1) ^ u1;
			w[2] = le32dec(in + 16*i + 4*2) ^ u2;
			w[3] = le32dec(in + 16*i + 4*3) ^ u3;
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
			br_aes_ct64_interleave_in(&q[i], &q[4 + i], w);
		}

		/* Encrypt four blocks.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		/* Store four blocks.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < 4; i++) {
			br_aes_ct64_interleave_out(w, q[i], q[4 + i]);
			le32enc(out + 16*i + 4*0, w[0] ^ u0);
			le32enc(out + 16*i + 4*1, w[1] ^ u1);
			le32enc(out + 16*i + 4*2, w[2] ^ u2);
			le32enc(out + 16*i + 4*3, w[3] ^ u3);
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
		}

		/* Advance to the next pair of blocks.  */
		t0 = u0, t1 = u1, t2 = u2, t3 = u3;
		in += 64;
		out += 64;
	} while (nbytes -= 64, nbytes);

out:	/* Store the updated tweak.  */
	le32enc(tweak + 4*0, t0);
	le32enc(tweak + 4*1, t1);
	le32enc(tweak + 4*2, t2);
	le32enc(tweak + 4*3, t3);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static void
aesbear64_xts_dec(const struct aesdec *dec, const uint8_t in[static 16],
    uint8_t out[static 16], size_t nbytes, uint8_t tweak[static 16],
    uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];
	uint32_t t0, t1, t2, t3, u0, u1, u2, u3;
	unsigned i;

	KASSERT(nbytes % 16 == 0);

	/* Skip if there's nothing to do.  */
	if (nbytes == 0)
		return;

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, dec->aesd_aes.aes_rk64);

	/* Load tweak.  */
	t0 = le32dec(tweak + 4*0);
	t1 = le32dec(tweak + 4*1);
	t2 = le32dec(tweak + 4*2);
	t3 = le32dec(tweak + 4*3);

	/* Handle the first blocks separately if odd number.  */
	if (nbytes % 64) {
		unsigned n = (nbytes % 64)/16;

		/* Load up the first blocks and garbage.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < n; i++) {
			w[0] = le32dec(in + 16*i + 4*0) ^ u0;
			w[1] = le32dec(in + 16*i + 4*1) ^ u1;
			w[2] = le32dec(in + 16*i + 4*2) ^ u2;
			w[3] = le32dec(in + 16*i + 4*3) ^ u3;
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
			br_aes_ct64_interleave_in(&q[i], &q[4 + i], w);
		}
		for (; i < 4; i++)
			q[i] = q[4 + i] = 0;

		/* Decrypt up to three blocks.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_decrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		/* Store up to three blocks.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < n; i++) {
			br_aes_ct64_interleave_out(w, q[i], q[4 + i]);
			le32enc(out + 16*i + 4*0, w[0] ^ u0);
			le32enc(out + 16*i + 4*1, w[1] ^ u1);
			le32enc(out + 16*i + 4*2, w[2] ^ u2);
			le32enc(out + 16*i + 4*3, w[3] ^ u3);
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
		}

		/* Advance to the next block.  */
		t0 = u0, t1 = u1, t2 = u2, t3 = u3;
		if ((nbytes -= 16*n) == 0)
			goto out;
		in += 16*n;
		out += 16*n;
	}

	do {
		KASSERT(nbytes >= 64);

		/* Load four blocks.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < 4; i++) {
			w[0] = le32dec(in + 16*i + 4*0) ^ u0;
			w[1] = le32dec(in + 16*i + 4*1) ^ u1;
			w[2] = le32dec(in + 16*i + 4*2) ^ u2;
			w[3] = le32dec(in + 16*i + 4*3) ^ u3;
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
			br_aes_ct64_interleave_in(&q[i], &q[4 + i], w);
		}

		/* Decrypt four blocks.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_decrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		/* Store four blocks.  */
		for (i = 0, u0 = t0, u1 = t1, u2 = t2, u3 = t3; i < 4; i++) {
			br_aes_ct64_interleave_out(w, q[i], q[4 + i]);
			le32enc(out + 16*i + 4*0, w[0] ^ u0);
			le32enc(out + 16*i + 4*1, w[1] ^ u1);
			le32enc(out + 16*i + 4*2, w[2] ^ u2);
			le32enc(out + 16*i + 4*3, w[3] ^ u3);
			aesbear64_xts_update(&u0, &u1, &u2, &u3);
		}

		/* Advance to the next pair of blocks.  */
		t0 = u0, t1 = u1, t2 = u2, t3 = u3;
		in += 64;
		out += 64;
	} while (nbytes -= 64, nbytes);

out:	/* Store the updated tweak.  */
	le32enc(tweak + 4*0, t0);
	le32enc(tweak + 4*1, t1);
	le32enc(tweak + 4*2, t2);
	le32enc(tweak + 4*3, t3);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static void
aesbear64_cbcmac_update1(const struct aesenc *enc, const uint8_t in[static 16],
    size_t nbytes, uint8_t auth[static 16], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Initialize garbage blocks.  */
	q[1] = q[2] = q[3] = 0;
	q[5] = q[6] = q[7] = 0;

	/* Load initial authenticator.  */
	w[0] = le32dec(auth + 4*0);
	w[1] = le32dec(auth + 4*1);
	w[2] = le32dec(auth + 4*2);
	w[3] = le32dec(auth + 4*3);

	for (; nbytes; nbytes -= 16, in += 16) {
		/* Combine input block.  */
		w[0] ^= le32dec(in + 4*0);
		w[1] ^= le32dec(in + 4*1);
		w[2] ^= le32dec(in + 4*2);
		w[3] ^= le32dec(in + 4*3);
		br_aes_ct64_interleave_in(&q[0], &q[4], w);

		/* Transform to bitslice, encrypt, transform from bitslice.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		br_aes_ct64_interleave_out(w, q[0], q[4]);
	}

	/* Store updated authenticator.  */
	le32enc(auth + 4*0, w[0]);
	le32enc(auth + 4*1, w[1]);
	le32enc(auth + 4*2, w[2]);
	le32enc(auth + 4*3, w[3]);

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static void
aesbear64_ccm_enc1(const struct aesenc *enc, const uint8_t *in, uint8_t *out,
    size_t nbytes, uint8_t authctr[32], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];
	uint32_t c0, c1, c2, c3be;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Initialize garbage blocks.  */
	q[2] = q[3] = 0;
	q[6] = q[7] = 0;

	/* Set first block to authenticator.  */
	w[0] = le32dec(authctr + 4*0);
	w[1] = le32dec(authctr + 4*1);
	w[2] = le32dec(authctr + 4*2);
	w[3] = le32dec(authctr + 4*3);

	/* Load initial counter block, big-endian so we can increment it.  */
	c0 = le32dec(authctr + 16 + 4*0);
	c1 = le32dec(authctr + 16 + 4*1);
	c2 = le32dec(authctr + 16 + 4*2);
	c3be = bswap32(le32dec(authctr + 16 + 4*3));

	for (; nbytes; nbytes -= 16, in += 16, out += 16) {
		/* Update authenticator.  */
		w[0] ^= le32dec(in + 4*0);
		w[1] ^= le32dec(in + 4*1);
		w[2] ^= le32dec(in + 4*2);
		w[3] ^= le32dec(in + 4*3);
		br_aes_ct64_interleave_in(&q[0], &q[4], w);

		/* Increment 32-bit counter.  */
		w[0] = c0;
		w[1] = c1;
		w[2] = c2;
		w[3] = bswap32(++c3be);
		br_aes_ct64_interleave_in(&q[1], &q[5], w);

		/* Encrypt authenticator and counter.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);

		/* Encrypt with CTR output.  */
		br_aes_ct64_interleave_out(w, q[1], q[5]);
		le32enc(out + 4*0, le32dec(in + 4*0) ^ w[0]);
		le32enc(out + 4*1, le32dec(in + 4*1) ^ w[1]);
		le32enc(out + 4*2, le32dec(in + 4*2) ^ w[2]);
		le32enc(out + 4*3, le32dec(in + 4*3) ^ w[3]);

		/* Fish out the authenticator so far.  */
		br_aes_ct64_interleave_out(w, q[0], q[4]);
	}

	/* Update authenticator.  */
	le32enc(authctr + 4*0, w[0]);
	le32enc(authctr + 4*1, w[1]);
	le32enc(authctr + 4*2, w[2]);
	le32enc(authctr + 4*3, w[3]);

	/* Update counter.  */
	le32enc(authctr + 16 + 4*3, bswap32(c3be));

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static void
aesbear64_ccm_dec1(const struct aesenc *enc, const uint8_t *in, uint8_t *out,
    size_t nbytes, uint8_t authctr[32], uint32_t nrounds)
{
	uint64_t sk_exp[120];
	uint32_t w[4];
	uint64_t q[8];
	uint32_t c0, c1, c2, c3be;
	uint32_t b0, b1, b2, b3;

	KASSERT(nbytes);
	KASSERT(nbytes % 16 == 0);

	/* Expand round keys for bitslicing.  */
	br_aes_ct64_skey_expand(sk_exp, nrounds, enc->aese_aes.aes_rk64);

	/* Initialize garbage blocks.  */
	q[2] = q[3] = 0;
	q[6] = q[7] = 0;

	/* Load initial counter block, big-endian so we can increment it.  */
	c0 = le32dec(authctr + 16 + 4*0);
	c1 = le32dec(authctr + 16 + 4*1);
	c2 = le32dec(authctr + 16 + 4*2);
	c3be = bswap32(le32dec(authctr + 16 + 4*3));

	/* Increment 32-bit counter.  */
	w[0] = c0;
	w[1] = c1;
	w[2] = c2;
	w[3] = bswap32(++c3be);
	br_aes_ct64_interleave_in(&q[1], &q[5], w);

	/*
	 * Set the other block to garbage -- we don't have any
	 * plaintext to authenticate yet.
	 */
	q[0] = q[4] = 0;

	/* Encrypt first CTR.  */
	br_aes_ct64_ortho(q);
	br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
	br_aes_ct64_ortho(q);

	/* Load the initial authenticator.  */
	w[0] = le32dec(authctr + 4*0);
	w[1] = le32dec(authctr + 4*1);
	w[2] = le32dec(authctr + 4*2);
	w[3] = le32dec(authctr + 4*3);
	br_aes_ct64_interleave_in(&q[0], &q[4], w);

	for (;; in += 16, out += 16) {
		/* Decrypt the block.  */
		br_aes_ct64_interleave_out(w, q[1], q[5]);
		b0 = le32dec(in + 4*0) ^ w[0];
		b1 = le32dec(in + 4*1) ^ w[1];
		b2 = le32dec(in + 4*2) ^ w[2];
		b3 = le32dec(in + 4*3) ^ w[3];

		/* Update authenticator.  */
		br_aes_ct64_interleave_out(w, q[0], q[4]);
		w[0] ^= b0;
		w[1] ^= b1;
		w[2] ^= b2;
		w[3] ^= b3;
		br_aes_ct64_interleave_in(&q[0], &q[4], w);

		/* Store plaintext.  */
		le32enc(out + 4*0, b0);
		le32enc(out + 4*1, b1);
		le32enc(out + 4*2, b2);
		le32enc(out + 4*3, b3);

		/* If this is the last block, stop.  */
		if ((nbytes -= 16) == 0)
			break;

		/* Increment 32-bit counter.  */
		w[0] = c0;
		w[1] = c1;
		w[2] = c2;
		w[3] = bswap32(++c3be);
		br_aes_ct64_interleave_in(&q[1], &q[5], w);

		/* Authenticate previous plaintext, encrypt next CTR.  */
		br_aes_ct64_ortho(q);
		br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
		br_aes_ct64_ortho(q);
	}

	/*
	 * Authenticate last plaintext.  We're only doing this for the
	 * authenticator, not for the counter, so don't bother to
	 * initialize q[2*i].  (Even for the sake of sanitizers,
	 * they're already initialized to something by now.)
	 */
	br_aes_ct64_ortho(q);
	br_aes_ct64_bitslice_encrypt(nrounds, sk_exp, q);
	br_aes_ct64_ortho(q);

	/* Update authenticator.  */
	br_aes_ct64_interleave_out(w, q[0], q[4]);
	le32enc(authctr + 4*0, w[0]);
	le32enc(authctr + 4*1, w[1]);
	le32enc(authctr + 4*2, w[2]);
	le32enc(authctr + 4*3, w[3]);

	/* Update counter.  */
	le32enc(authctr + 16 + 4*3, bswap32(c3be));

	/* Paranoia: Zero temporary buffers.  */
	explicit_memset(sk_exp, 0, sizeof sk_exp);
	explicit_memset(q, 0, sizeof q);
}

static int
aesbear64_probe(void)
{

	if (aesbear64_xts_update_selftest())
		return -1;

	/* XXX test br_aes_ct64_bitslice_decrypt */
	/* XXX test br_aes_ct64_bitslice_encrypt */
	/* XXX test br_aes_ct64_keysched */
	/* XXX test br_aes_ct64_ortho */
	/* XXX test br_aes_ct64_skey_expand */

	return 0;
}

struct aes_impl aes_bear64_impl = {
	.ai_name = "BearSSL aes_ct64",
	.ai_probe = aesbear64_probe,
	.ai_setenckey = aesbear64_setenckey,
	.ai_setdeckey = aesbear64_setdeckey,
	.ai_enc = aesbear64_enc,
	.ai_dec = aesbear64_dec,
	.ai_cbc_enc = aesbear64_cbc_enc,
	.ai_cbc_dec = aesbear64_cbc_dec,
	.ai_xts_enc = aesbear64_xts_enc,
	.ai_xts_dec = aesbear64_xts_dec,
	.ai_cbcmac_update1 = aesbear64_cbcmac_update1,
	.ai_ccm_enc1 = aesbear64_ccm_enc1,
	.ai_ccm_dec1 = aesbear64_ccm_dec1,
};
