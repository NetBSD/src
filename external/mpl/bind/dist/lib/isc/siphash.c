/*	$NetBSD: siphash.c,v 1.5 2020/05/24 19:46:26 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <isc/endian.h>
#include <isc/siphash.h>
#include <isc/util.h>

/*
 * Creation of EVP_MD_CTX and EVP_PKEY is quite expensive, until
 * we fix the code to reuse the context and key we'll use our own
 * implementation of siphash.
 */
#if 0 /* HAVE_OPENSSL_SIPHASH */
#include <openssl/evp.h>

void
isc_siphash24(const uint8_t*k,const uint8_t*in,const size_t inlen,uint8_t*out)
{
	REQUIRE(k != NULL);
	REQUIRE(out != NULL);
	size_t outlen = 8;
	EVP_PKEY_CTX*pctx = NULL;

	EVP_MD_CTX*mctx = EVP_MD_CTX_new();
	EVP_PKEY*key = EVP_PKEY_new_raw_private_key(EVP_PKEY_SIPHASH,NULL,
						    k,16);
	RUNTIME_CHECK(mctx != NULL);
	RUNTIME_CHECK(key != NULL);

	RUNTIME_CHECK(EVP_DigestSignInit(mctx,&pctx,NULL,NULL,key) == 1);
	RUNTIME_CHECK(EVP_PKEY_CTX_ctrl(pctx,EVP_PKEY_SIPHASH,
					EVP_PKEY_OP_SIGNCTX,
					EVP_PKEY_CTRL_SET_DIGEST_SIZE,outlen,
					NULL) == 1);
	RUNTIME_CHECK(EVP_DigestSignUpdate(mctx,in,inlen) == 1);
	RUNTIME_CHECK(EVP_DigestSignFinal(mctx,out,&outlen) == 1);

	ENSURE(outlen == 8);

	EVP_PKEY_free(key);
	EVP_MD_CTX_free(mctx);
}

#else /* HAVE_OPENSSL_SIPHASH */

/*
 * The implementation is based on SipHash reference C implementation by
 *
 * Copyright (c) 2012-2016 Jean-Philippe Aumasson
 * <jeanphilippe.aumasson@gmail.com> Copyright (c) 2012-2014 Daniel J. Bernstein
 * <djb@cr.yp.to>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.  You should
 * have received a copy of the CC0 Public Domain Dedication along with this
 * software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#define cROUNDS 2
#define dROUNDS 4

#define ROTATE(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define HALF_ROUND(a, b, c, d, s, t) \
	a += b;                      \
	c += d;                      \
	b = ROTATE(b, s) ^ a;        \
	d = ROTATE(d, t) ^ c;        \
	a = ROTATE(a, 32);

#define FULL_ROUND(v0, v1, v2, v3)          \
	HALF_ROUND(v0, v1, v2, v3, 13, 16); \
	HALF_ROUND(v2, v1, v0, v3, 17, 21);

#define DOUBLE_ROUND(v0, v1, v2, v3) \
	FULL_ROUND(v0, v1, v2, v3)   \
	FULL_ROUND(v0, v1, v2, v3)

#define SIPROUND FULL_ROUND

#define U32TO8_LE(p, v)                \
	(p)[0] = (uint8_t)((v));       \
	(p)[1] = (uint8_t)((v) >> 8);  \
	(p)[2] = (uint8_t)((v) >> 16); \
	(p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)                  \
	U32TO8_LE((p), (uint32_t)((v))); \
	U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#define U8TO64_LE(p)                                               \
	(((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |        \
	 ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) | \
	 ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) | \
	 ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))

void
isc_siphash24(const uint8_t *k, const uint8_t *in, const size_t inlen,
	      uint8_t *out) {
	REQUIRE(k != NULL);
	REQUIRE(out != NULL);

	uint64_t k0;
	uint64_t k1;

	memcpy(&k0, k, sizeof(k0));
	memcpy(&k1, k + sizeof(k0), sizeof(k1));

	k0 = le64toh(k0);
	k1 = le64toh(k1);

	uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
	uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
	uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
	uint64_t v3 = 0x7465646279746573ULL ^ k1;

	uint64_t b = ((uint64_t)inlen) << 56;

	const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
	const size_t left = inlen & 7;

	for (; in != end; in += 8) {
		uint64_t m = U8TO64_LE(in);

		v3 ^= m;

		for (size_t i = 0; i < cROUNDS; ++i) {
			SIPROUND(v0, v1, v2, v3);
		}

		v0 ^= m;
	}

	switch (left) {
	case 7:
		b |= ((uint64_t)in[6]) << 48;
	/* FALLTHROUGH */
	case 6:
		b |= ((uint64_t)in[5]) << 40;
	/* FALLTHROUGH */
	case 5:
		b |= ((uint64_t)in[4]) << 32;
	/* FALLTHROUGH */
	case 4:
		b |= ((uint64_t)in[3]) << 24;
	/* FALLTHROUGH */
	case 3:
		b |= ((uint64_t)in[2]) << 16;
	/* FALLTHROUGH */
	case 2:
		b |= ((uint64_t)in[1]) << 8;
	/* FALLTHROUGH */
	case 1:
		b |= ((uint64_t)in[0]);
	/* FALLTHROUGH */
	case 0:
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	v3 ^= b;

	for (size_t i = 0; i < cROUNDS; ++i) {
		SIPROUND(v0, v1, v2, v3);
	}

	v0 ^= b;

	v2 ^= 0xff;

	for (size_t i = 0; i < dROUNDS; ++i) {
		SIPROUND(v0, v1, v2, v3);
	}

	b = v0 ^ v1 ^ v2 ^ v3;

	U64TO8_LE(out, b);
}
#endif /* HAVE_OPENSSL_SIPHASH */
