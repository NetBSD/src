/*	$NetBSD: random.c,v 1.1.2.2 2024/02/24 13:07:21 martin Exp $	*/

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
 * Portions of isc_random_uniform():
 *
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <isc/once.h>
#include <isc/platform.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/thread.h>
#include <isc/types.h>
#include <isc/util.h>

#include "entropy_private.h"

/*
 * The specific implementation for PRNG is included as a C file
 * that has to provide a static variable named seed, and a function
 * uint32_t next(void) that provides next random number.
 *
 * The implementation must be thread-safe.
 */

/*
 * Two contestants have been considered: the xoroshiro family of the
 * functions by Villa&Blackman, and PCG by O'Neill.  After
 * consideration, the xoshiro128starstar function has been chosen as
 * the uint32_t random number provider because it is very fast and has
 * good enough properties for our usage pattern.
 */
#include "xoshiro128starstar.c"

ISC_THREAD_LOCAL isc_once_t isc_random_once = ISC_ONCE_INIT;

static void
isc_random_initialize(void) {
	int useed[4] = { 0, 0, 0, 1 };
#if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
	/*
	 * Set a constant seed to help in problem reproduction should fuzzing
	 * find a crash or a hang.  The seed array must be non-zero else
	 * xoshiro128starstar will generate an infinite series of zeroes.
	 */
#else  /* if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION */
	isc_entropy_get(useed, sizeof(useed));
#endif /* if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION */
	memmove(seed, useed, sizeof(seed));
}

uint8_t
isc_random8(void) {
	RUNTIME_CHECK(isc_once_do(&isc_random_once, isc_random_initialize) ==
		      ISC_R_SUCCESS);
	return (next() & 0xff);
}

uint16_t
isc_random16(void) {
	RUNTIME_CHECK(isc_once_do(&isc_random_once, isc_random_initialize) ==
		      ISC_R_SUCCESS);
	return (next() & 0xffff);
}

uint32_t
isc_random32(void) {
	RUNTIME_CHECK(isc_once_do(&isc_random_once, isc_random_initialize) ==
		      ISC_R_SUCCESS);
	return (next());
}

void
isc_random_buf(void *buf, size_t buflen) {
	int i;
	uint32_t r;

	REQUIRE(buf != NULL);
	REQUIRE(buflen > 0);

	RUNTIME_CHECK(isc_once_do(&isc_random_once, isc_random_initialize) ==
		      ISC_R_SUCCESS);

	for (i = 0; i + sizeof(r) <= buflen; i += sizeof(r)) {
		r = next();
		memmove((uint8_t *)buf + i, &r, sizeof(r));
	}
	r = next();
	memmove((uint8_t *)buf + i, &r, buflen % sizeof(r));
	return;
}

uint32_t
isc_random_uniform(uint32_t upper_bound) {
	/* Copy of arc4random_uniform from OpenBSD */
	uint32_t r, min;

	RUNTIME_CHECK(isc_once_do(&isc_random_once, isc_random_initialize) ==
		      ISC_R_SUCCESS);

	if (upper_bound < 2) {
		return (0);
	}

#if (ULONG_MAX > 0xffffffffUL)
	min = 0x100000000UL % upper_bound;
#else  /* if (ULONG_MAX > 0xffffffffUL) */
	/* Calculate (2**32 % upper_bound) avoiding 64-bit math */
	if (upper_bound > 0x80000000) {
		min = 1 + ~upper_bound; /* 2**32 - upper_bound */
	} else {
		/* (2**32 - (x * 2)) % x == 2**32 % x when x <= 2**31 */
		min = ((0xffffffff - (upper_bound * 2)) + 1) % upper_bound;
	}
#endif /* if (ULONG_MAX > 0xffffffffUL) */

	/*
	 * This could theoretically loop forever but each retry has
	 * p > 0.5 (worst case, usually far better) of selecting a
	 * number inside the range we need, so it should rarely need
	 * to re-roll.
	 */
	for (;;) {
		r = next();
		if (r >= min) {
			break;
		}
	}

	return (r % upper_bound);
}
