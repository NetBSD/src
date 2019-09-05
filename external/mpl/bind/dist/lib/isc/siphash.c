/*	$NetBSD: siphash.c,v 1.2 2019/09/05 19:32:59 christos Exp $	*/

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

/*
   siphash() function is SipHash reference C implementation

   Copyright (c) 2012-2016 Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
   Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along
   with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

/*! \file isc/siphash.c */

#include <config.h>

#include <inttypes.h>
#include <unistd.h>
#include <string.h>

#include <openssl/opensslv.h>

#include <isc/endian.h>
#include <isc/util.h>
#include <isc/siphash.h>

#define ROTATE(x, b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define HALF_ROUND(a, b, c, d, s, t)		 \
	a += b; c += d;				 \
	b = ROTATE(b, s) ^ a;			 \
	d = ROTATE(d, t) ^ c;			 \
	a = ROTATE(a, 32);

#define FULL_ROUND(v0, v1, v2, v3)			      \
	HALF_ROUND(v0, v1, v2, v3, 13, 16);		      \
	HALF_ROUND(v2, v1, v0, v3, 17, 21);

#define DOUBLE_ROUND(v0, v1, v2, v3)		\
	FULL_ROUND(v0, v1, v2, v3)			\
	FULL_ROUND(v0, v1, v2, v3)

#define SIPROUND FULL_ROUND

void
isc_siphash24(const uint8_t *k, const uint8_t *in, size_t inlen, uint8_t *out)
{
	const uint64_t *key = (const uint64_t *)k;
	uint64_t k0 = le64toh(key[0]);
	uint64_t k1 = le64toh(key[1]);

	uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
	uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
	uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
	uint64_t v3 = 0x7465646279746573ULL ^ k1;

	size_t left = inlen;

	uint64_t b = ((uint64_t)inlen) << 56;

	const uint64_t *inbuf = (const uint64_t *)in;
	while (left >= 8) {
		uint64_t m = le64toh(*inbuf);

		v3 ^= m;

		SIPROUND(v0, v1, v2, v3);
		SIPROUND(v0, v1, v2, v3);

		v0 ^= m;

		inbuf++; left -= 8;
	}

	const uint8_t *end = in + (inlen - left);

	switch (left) {
	case 7:
		b |= ((uint64_t)end[6]) << 48;
		/* FALLTHROUGH */
	case 6:
		b |= ((uint64_t)end[5]) << 40;
		/* FALLTHROUGH */
	case 5:
		b |= ((uint64_t)end[4]) << 32;
		/* FALLTHROUGH */
	case 4:
		b |= ((uint64_t)end[3]) << 24;
		/* FALLTHROUGH */
	case 3:
		b |= ((uint64_t)end[2]) << 16;
		/* FALLTHROUGH */
	case 2:
		b |= ((uint64_t)end[1]) << 8;
		/* FALLTHROUGH */
	case 1:
		b |= ((uint64_t)end[0]);
		/* FALLTHROUGH */
	case 0:
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	v3 ^= b;

	SIPROUND(v0, v1, v2, v3);
	SIPROUND(v0, v1, v2, v3);

	v0 ^= b;

	v2 ^= 0xff;

	SIPROUND(v0, v1, v2, v3);
	SIPROUND(v0, v1, v2, v3);
	SIPROUND(v0, v1, v2, v3);
	SIPROUND(v0, v1, v2, v3);

	b = v0 ^ v1 ^ v2 ^ v3;

	uint64_t *outbuf = (uint64_t *)out;
	*outbuf = htole64(b);
}
