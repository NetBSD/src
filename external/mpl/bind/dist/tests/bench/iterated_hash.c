/*	$NetBSD: iterated_hash.c,v 1.1.1.1 2025/01/26 16:12:38 christos Exp $	*/

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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <isc/iterated_hash.h>
#include <isc/random.h>
#include <isc/time.h>

#include <dns/name.h>

static void
time_it(const int count, const int iterations, const unsigned char *salt,
	const int saltlen, const unsigned char *in, const int inlen) {
	uint8_t out[NSEC3_MAX_HASH_LENGTH] = { 0 };
	isc_time_t start, finish;

	printf("%d iterations, %d salt length, %d input length: ", iterations,
	       saltlen, inlen);
	fflush(stdout);

	start = isc_time_now_hires();

	int i = 0;
	while (i++ < count) {
		isc_iterated_hash(out, 1, iterations, salt, saltlen, in, inlen);
	}

	finish = isc_time_now_hires();

	uint64_t microseconds = isc_time_microdiff(&finish, &start);
	printf("%0.2f us per iterated_hash()\n", (double)microseconds / count);
	fflush(stdout);
}

int
main(void) {
	uint8_t salt[DNS_NAME_MAXWIRE];
	uint8_t in[DNS_NAME_MAXWIRE];
	size_t saltlen = sizeof(salt);
	size_t inlen = sizeof(in);

	isc_random_buf(salt, saltlen);
	isc_random_buf(in, inlen);

	time_it(10000, 150, salt, saltlen, in, inlen);
	time_it(10000, 15, salt, saltlen, in, inlen);
	time_it(10000, 0, salt, saltlen, in, inlen);

	saltlen = 32;
	inlen = 32;

	time_it(10000, 150, salt, 32, in, inlen);
	time_it(10000, 15, salt, 32, in, inlen);
	time_it(10000, 0, salt, saltlen, in, inlen);

	saltlen = 0;
	inlen = 1;

	time_it(10000, 150, salt, 32, in, inlen);
	time_it(10000, 15, salt, 32, in, inlen);
	time_it(10000, 0, salt, saltlen, in, inlen);
}
