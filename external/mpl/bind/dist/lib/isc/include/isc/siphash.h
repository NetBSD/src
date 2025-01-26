/*	$NetBSD: siphash.h,v 1.8 2025/01/26 16:25:42 christos Exp $	*/

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

/*! \file isc/siphash.h */

#pragma once

#include <isc/ascii.h>
#include <isc/endian.h>
#include <isc/lang.h>
#include <isc/types.h>
#include <isc/util.h>

#define ISC_SIPHASH24_KEY_LENGTH 128 / 8
#define ISC_SIPHASH24_TAG_LENGTH 64 / 8

#define ISC_HALFSIPHASH24_KEY_LENGTH 64 / 8
#define ISC_HALFSIPHASH24_TAG_LENGTH 32 / 8

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

#define U8TO32_ONE(case_sensitive, byte) \
	(uint32_t)(case_sensitive ? byte : isc__ascii_tolower1(byte))

#define U8TO64_ONE(case_sensitive, byte) \
	(uint64_t)(case_sensitive ? byte : isc__ascii_tolower1(byte))

ISC_LANG_BEGINDECLS

typedef struct isc_siphash24 {
	uint64_t k0;
	uint64_t k1;

	uint64_t v0;
	uint64_t v1;
	uint64_t v2;
	uint64_t v3;

	uint64_t b;

	size_t inlen;
} isc_siphash24_t;

typedef struct isc_halfsiphash24 {
	uint32_t k0;
	uint32_t k1;

	uint32_t v0;
	uint32_t v1;
	uint32_t v2;
	uint32_t v3;

	uint32_t b;

	size_t inlen;
} isc_halfsiphash24_t;

static inline void
isc_siphash24_init(isc_siphash24_t *state, const uint8_t *k) {
	REQUIRE(k != NULL);

	uint64_t k0 = ISC_U8TO64_LE(k);
	uint64_t k1 = ISC_U8TO64_LE(k + 8);

	*state = (isc_siphash24_t){
		.k0 = k0,
		.k1 = k1,
		.v0 = UINT64_C(0x736f6d6570736575) ^ k0,
		.v1 = UINT64_C(0x646f72616e646f6d) ^ k1,
		.v2 = UINT64_C(0x6c7967656e657261) ^ k0,
		.v3 = UINT64_C(0x7465646279746573) ^ k1,
	};
}

static inline void
isc_siphash24_one(isc_siphash24_t *restrict state, const uint64_t m) {
	state->v3 ^= m;

	for (size_t i = 0; i < cROUNDS; ++i) {
		SIPROUND(state->v0, state->v1, state->v2, state->v3);
	}

	state->v0 ^= m;
}

static inline void
isc_siphash24_hash(isc_siphash24_t *restrict state, const uint8_t *in,
		   const size_t inlen, const bool case_sensitive) {
	REQUIRE(inlen == 0 || in != NULL);

	if (in == NULL || inlen == 0) {
		return;
	}

	size_t	  len = inlen;
	const int right = state->inlen & 7;

	switch (right) {
	case 0:
		break;
	case 1:
		state->b |= U8TO64_ONE(case_sensitive, in[0]) << 8;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 2:
		state->b |= U8TO64_ONE(case_sensitive, in[0]) << 16;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 3:
		state->b |= U8TO64_ONE(case_sensitive, in[0]) << 24;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 4:
		state->b |= U8TO64_ONE(case_sensitive, in[0]) << 32;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 5:
		state->b |= U8TO64_ONE(case_sensitive, in[0]) << 40;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 6:
		state->b |= U8TO64_ONE(case_sensitive, in[0]) << 48;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 7:
		state->b |= U8TO64_ONE(case_sensitive, in[0]) << 56;
		state->inlen++;
		++in;

		isc_siphash24_one(state, state->b);
		state->b = 0; /* consumed */

		if (--len == 0) {
			return;
		}
		break;
	default:
		UNREACHABLE();
	}

	const uint8_t *end = in + len - (len % sizeof(uint64_t));
	const size_t   left = len & 7;

	for (; in != end; in += 8) {
		uint64_t m = case_sensitive
				     ? ISC_U8TO64_LE(in)
				     : isc_ascii_tolower8(ISC_U8TO64_LE(in));

		isc_siphash24_one(state, m);
	}

	INSIST(state->b == 0);
	switch (left) {
	case 7:
		state->b |= U8TO64_ONE(case_sensitive, in[6]) << 48;
		FALLTHROUGH;
	case 6:
		state->b |= U8TO64_ONE(case_sensitive, in[5]) << 40;
		FALLTHROUGH;
	case 5:
		state->b |= U8TO64_ONE(case_sensitive, in[4]) << 32;
		FALLTHROUGH;
	case 4:
		state->b |= U8TO64_ONE(case_sensitive, in[3]) << 24;
		FALLTHROUGH;
	case 3:
		state->b |= U8TO64_ONE(case_sensitive, in[2]) << 16;
		FALLTHROUGH;
	case 2:
		state->b |= U8TO64_ONE(case_sensitive, in[1]) << 8;
		FALLTHROUGH;
	case 1:
		state->b |= U8TO64_ONE(case_sensitive, in[0]);
		FALLTHROUGH;
	case 0:
		break;
	default:
		UNREACHABLE();
	}

	state->inlen += len;
}

static inline void
isc_siphash24_finalize(isc_siphash24_t *restrict state, uint8_t *out) {
	REQUIRE(out != NULL);

	uint64_t b = ((uint64_t)state->inlen) << 56 | state->b;

	isc_siphash24_one(state, b);

	state->v2 ^= 0xff;

	for (size_t i = 0; i < dROUNDS; ++i) {
		SIPROUND(state->v0, state->v1, state->v2, state->v3);
	}

	b = state->v0 ^ state->v1 ^ state->v2 ^ state->v3;

	ISC_U64TO8_LE(out, b);
}

static inline void
isc_siphash24(const uint8_t *key, const uint8_t *in, const size_t inlen,
	      bool case_sensitive, uint8_t *out) {
	isc_siphash24_t state;
	isc_siphash24_init(&state, key);
	isc_siphash24_hash(&state, in, inlen, case_sensitive);
	isc_siphash24_finalize(&state, out);
}

static inline void
isc_halfsiphash24_init(isc_halfsiphash24_t *restrict state, const uint8_t *k) {
	REQUIRE(k != NULL);

	uint32_t k0 = ISC_U8TO32_LE(k);
	uint32_t k1 = ISC_U8TO32_LE(k + 4);

	*state = (isc_halfsiphash24_t){
		.k0 = k0,
		.k1 = k1,
		.v0 = UINT32_C(0x00000000) ^ k0,
		.v1 = UINT32_C(0x00000000) ^ k1,
		.v2 = UINT32_C(0x6c796765) ^ k0,
		.v3 = UINT32_C(0x74656462) ^ k1,
	};
}

static inline void
isc_halfsiphash24_one(isc_halfsiphash24_t *restrict state, const uint32_t m) {
	state->v3 ^= m;

	for (size_t i = 0; i < cROUNDS; ++i) {
		HALFSIPROUND(state->v0, state->v1, state->v2, state->v3);
	}

	state->v0 ^= m;
}

static inline void
isc_halfsiphash24_hash(isc_halfsiphash24_t *restrict state, const uint8_t *in,
		       const size_t inlen, const bool case_sensitive) {
	REQUIRE(inlen == 0 || in != NULL);

	if (in == NULL || inlen == 0) {
		return;
	}

	size_t	  len = inlen;
	const int right = state->inlen & 3;

	switch (right) {
	case 0:
		break;
	case 1:
		state->b |= U8TO32_ONE(case_sensitive, in[0]) << 8;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 2:
		state->b |= U8TO32_ONE(case_sensitive, in[0]) << 16;
		state->inlen++;
		++in;

		if (--len == 0) {
			return;
		}
		FALLTHROUGH;
	case 3:
		state->b |= U8TO32_ONE(case_sensitive, in[0]) << 24;
		state->inlen++;
		++in;

		isc_halfsiphash24_one(state, state->b);
		state->b = 0; /* consumed */

		if (--len == 0) {
			return;
		}
		break;
	default:
		UNREACHABLE();
	}

	const uint8_t *end = in + len - (len % sizeof(uint32_t));
	const int      left = len & 3;

	for (; in != end; in += 4) {
		uint32_t m = case_sensitive
				     ? ISC_U8TO32_LE(in)
				     : isc_ascii_tolower4(ISC_U8TO32_LE(in));

		isc_halfsiphash24_one(state, m);
	}

	INSIST(state->b == 0);
	switch (left) {
	case 3:
		state->b |= U8TO32_ONE(case_sensitive, in[2]) << 16;
		FALLTHROUGH;
	case 2:
		state->b |= U8TO32_ONE(case_sensitive, in[1]) << 8;
		FALLTHROUGH;
	case 1:
		state->b |= U8TO32_ONE(case_sensitive, in[0]);
		FALLTHROUGH;
	case 0:
		break;
	default:
		UNREACHABLE();
	}

	state->inlen += len;
}

static inline void
isc_halfsiphash24_finalize(isc_halfsiphash24_t *restrict state, uint8_t *out) {
	REQUIRE(out != NULL);

	uint32_t b = ((uint32_t)state->inlen) << 24 | state->b;

	isc_halfsiphash24_one(state, b);

	state->v2 ^= 0xff;

	for (size_t i = 0; i < dROUNDS; ++i) {
		HALFSIPROUND(state->v0, state->v1, state->v2, state->v3);
	}

	b = state->v1 ^ state->v3;
	ISC_U32TO8_LE(out, b);
}

static inline void
isc_halfsiphash24(const uint8_t *k, const uint8_t *in, const size_t inlen,
		  bool case_sensitive, uint8_t *out) {
	isc_halfsiphash24_t state;

	isc_halfsiphash24_init(&state, k);
	isc_halfsiphash24_hash(&state, in, inlen, case_sensitive);
	isc_halfsiphash24_finalize(&state, out);
}

ISC_LANG_ENDDECLS
