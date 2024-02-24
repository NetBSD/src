/*	$NetBSD: iterated_hash.c,v 1.1.2.2 2024/02/24 13:07:20 martin Exp $	*/

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

#include <stdio.h>

#include <openssl/opensslv.h>
#include <openssl/sha.h>

#include <isc/iterated_hash.h>
#include <isc/thread.h>
#include <isc/util.h>

int
isc_iterated_hash(unsigned char *out, const unsigned int hashalg,
		  const int iterations, const unsigned char *salt,
		  const int saltlength, const unsigned char *in,
		  const int inlength) {
	REQUIRE(out != NULL);

	int n = 0;
	size_t len;
	const unsigned char *buf;
	SHA_CTX ctx;

	if (hashalg != 1) {
		return (0);
	}

	buf = in;
	len = inlength;

	do {
		if (SHA1_Init(&ctx) != 1) {
			return (0);
		}

		if (SHA1_Update(&ctx, buf, len) != 1) {
			return (0);
		}

		if (SHA1_Update(&ctx, salt, saltlength) != 1) {
			return (0);
		}

		if (SHA1_Final(out, &ctx) != 1) {
			return (0);
		}

		buf = out;
		len = SHA_DIGEST_LENGTH;
	} while (n++ < iterations);

	return (SHA_DIGEST_LENGTH);
}
