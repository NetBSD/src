/*	$NetBSD: siphash.c,v 1.8 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
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

#define ROTATE64(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define HALF_ROUND64(a, b, c, d, s, t) \
	a += b;                        \
	c += d;                        \
	b = ROTATE64(b, s) ^ a;        \
	d = ROTATE64(d, t) ^ c;        \
	a = ROTATE64(a, 32);

#define FULL_ROUND64(v0, v1, v2, v3)          \
	HALF_ROUND64(v0, v1, v2, v3, 13, 16); \
	HALF_ROUND64(v2, v1, v0, v3, 17, 21);

#define SIPROUND FULL_ROUND64

#define ROTATE32(x, b) (uint32_t)(((x) << (b)) | ((x) >> (32 - (b))))

#define HALF_ROUND32(a, b, c, d, s, t) \
	a += b;                        \
	c += d;                        \
	b = ROTATE32(b, s) ^ a;        \
	d = ROTATE32(d, t) ^ c;        \
	a = ROTATE32(a, 16);

#define FULL_ROUND32(v0, v1, v2, v3)        \
	HALF_ROUND32(v0, v1, v2, v3, 5, 8); \
	HALF_ROUND32(v2, v1, v0, v3, 13, 7);

#define HALFSIPROUND FULL_ROUND32

#define U32TO8_LE(p, v)                \
	(p)[0] = (uint8_t)((v));       \
	(p)[1] = (uint8_t)((v) >> 8);  \
	(p)[2] = (uint8_t)((v) >> 16); \
	(p)[3] = (uint8_t)((v) >> 24);

#define U8TO32_LE(p)                                        \
	(((uint32_t)((p)[0])) | ((uint32_t)((p)[1]) << 8) | \
	 ((uint32_t)((p)[2]) << 16) | ((uint32_t)((p)[3]) << 24))

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
	REQUIRE(inlen == 0 || in != NULL);

	uint64_t k0;
	uint64_t k1;

	memcpy(&k0, k, sizeof(k0));
	memcpy(&k1, k + sizeof(k0), sizeof(k1));

	k0 = le64toh(k0);
	k1 = le64toh(k1);

	uint64_t v0 = UINT64_C(0x736f6d6570736575) ^ k0;
	uint64_t v1 = UINT64_C(0x646f72616e646f6d) ^ k1;
	uint64_t v2 = UINT64_C(0x6c7967656e657261) ^ k0;
	uint64_t v3 = UINT64_C(0x7465646279746573) ^ k1;

	uint64_t b = ((uint64_t)inlen) << 56;

	const uint8_t *end = (in == NULL)
				     ? NULL
				     : in + inlen - (inlen % sizeof(uint64_t));
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
		FALLTHROUGH;
	case 6:
		b |= ((uint64_t)in[5]) << 40;
		FALLTHROUGH;
	case 5:
		b |= ((uint64_t)in[4]) << 32;
		FALLTHROUGH;
	case 4:
		b |= ((uint64_t)in[3]) << 24;
		FALLTHROUGH;
	case 3:
		b |= ((uint64_t)in[2]) << 16;
		FALLTHROUGH;
	case 2:
		b |= ((uint64_t)in[1]) << 8;
		FALLTHROUGH;
	case 1:
		b |= ((uint64_t)in[0]);
		FALLTHROUGH;
	case 0:
		break;
	default:
		UNREACHABLE();
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

void
isc_halfsiphash24(const uint8_t *k, const uint8_t *in, const size_t inlen,
		  uint8_t *out) {
	REQUIRE(k != NULL);
	REQUIRE(out != NULL);
	REQUIRE(inlen == 0 || in != NULL);

	uint32_t k0 = U8TO32_LE(k);
	uint32_t k1 = U8TO32_LE(k + 4);

	uint32_t v0 = UINT32_C(0x00000000) ^ k0;
	uint32_t v1 = UINT32_C(0x00000000) ^ k1;
	uint32_t v2 = UINT32_C(0x6c796765) ^ k0;
	uint32_t v3 = UINT32_C(0x74656462) ^ k1;

	uint32_t b = ((uint32_t)inlen) << 24;

	const uint8_t *end = (in == NULL)
				     ? NULL
				     : in + inlen - (inlen % sizeof(uint32_t));
	const int left = inlen & 3;

	for (; in != end; in += 4) {
		uint32_t m = U8TO32_LE(in);
		v3 ^= m;

		for (size_t i = 0; i < cROUNDS; ++i) {
			HALFSIPROUND(v0, v1, v2, v3);
		}

		v0 ^= m;
	}

	switch (left) {
	case 3:
		b |= ((uint32_t)in[2]) << 16;
		FALLTHROUGH;
	case 2:
		b |= ((uint32_t)in[1]) << 8;
		FALLTHROUGH;
	case 1:
		b |= ((uint32_t)in[0]);
		FALLTHROUGH;
	case 0:
		break;
	default:
		UNREACHABLE();
	}

	v3 ^= b;

	for (size_t i = 0; i < cROUNDS; ++i) {
		HALFSIPROUND(v0, v1, v2, v3);
	}

	v0 ^= b;

	v2 ^= 0xff;

	for (size_t i = 0; i < dROUNDS; ++i) {
		HALFSIPROUND(v0, v1, v2, v3);
	}

	b = v1 ^ v3;
	U32TO8_LE(out, b);
}
