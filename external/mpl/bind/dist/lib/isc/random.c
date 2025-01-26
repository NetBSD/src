/*	$NetBSD: random.c,v 1.8 2025/01/26 16:25:38 christos Exp $	*/

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

#include <isc/entropy.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/thread.h>
#include <isc/types.h>
#include <isc/util.h>

/*
 * Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
 *
 * To the extent possible under law, the author has dedicated all
 * copyright and related and neighboring rights to this software to the
 * public domain worldwide. This software is distributed without any
 * warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

/*
 * This is xoshiro128** 1.0, our 32-bit all-purpose, rock-solid generator.
 * It has excellent (sub-ns) speed, a state size (128 bits) that is large
 * enough for mild parallelism, and it passes all tests we are aware of.
 *
 * The state must be seeded so that it is not everywhere zero.
 */

static thread_local bool initialized = false;
static thread_local uint32_t seed[4] = { 0 };

static uint32_t
rotl(const uint32_t x, int k) {
	return (x << k) | (x >> (32 - k));
}

static uint32_t
next(void) {
	uint32_t result_starstar, t;

	result_starstar = rotl(seed[0] * 5, 7) * 9;
	t = seed[1] << 9;

	seed[2] ^= seed[0];
	seed[3] ^= seed[1];
	seed[1] ^= seed[2];
	seed[0] ^= seed[3];

	seed[2] ^= t;

	seed[3] = rotl(seed[3], 11);

	return result_starstar;
}

static void
isc__random_initialize(void) {
	if (initialized) {
		return;
	}

#if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
	/*
	 * A fixed seed helps with problem reproduction when fuzzing. It must be
	 * non-zero else xoshiro128starstar will generate only zeroes, and the
	 * first result needs to be non-zero as expected by random_test.c
	 */
	seed[0] = 1;
#endif /* if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION */

	while (seed[0] == 0 && seed[1] == 0 && seed[2] == 0 && seed[3] == 0) {
		isc_entropy_get(seed, sizeof(seed));
	}
	initialized = true;
}

uint8_t
isc_random8(void) {
	isc__random_initialize();
	return (uint8_t)next();
}

uint16_t
isc_random16(void) {
	isc__random_initialize();
	return (uint16_t)next();
}

uint32_t
isc_random32(void) {
	isc__random_initialize();
	return next();
}

void
isc_random_buf(void *buf, size_t buflen) {
	REQUIRE(buf != NULL);
	REQUIRE(buflen > 0);

	int i;
	uint32_t r;

	isc__random_initialize();

	for (i = 0; i + sizeof(r) <= buflen; i += sizeof(r)) {
		r = next();
		memmove((uint8_t *)buf + i, &r, sizeof(r));
	}
	r = next();
	memmove((uint8_t *)buf + i, &r, buflen % sizeof(r));
	return;
}

uint32_t
isc_random_uniform(uint32_t limit) {
	isc__random_initialize();

	/*
	 * Daniel Lemire's nearly-divisionless unbiased bounded random numbers.
	 *
	 * https://lemire.me/blog/?p=17551
	 *
	 * The raw random number generator `next()` returns a 32-bit value.
	 * We do a 64-bit multiply `next() * limit` and treat the product as a
	 * 32.32 fixed-point value less than the limit. Our result will be the
	 * integer part (upper 32 bits), and we will use the fraction part
	 * (lower 32 bits) to determine whether or not we need to resample.
	 */
	uint64_t num = (uint64_t)next() * (uint64_t)limit;
	/*
	 * In the fast path, we avoid doing a division in most cases by
	 * comparing the fraction part of `num` with the limit, which is
	 * a slight over-estimate for the exact resample threshold.
	 */
	if ((uint32_t)(num) < limit) {
		/*
		 * We are in the slow path where we re-do the approximate test
		 * more accurately. The exact threshold for the resample loop
		 * is the remainder after dividing the raw RNG limit `1 << 32`
		 * by the caller's limit. We use a trick to calculate it
		 * within 32 bits:
		 *
		 *     (1 << 32) % limit
		 * == ((1 << 32) - limit) % limit
		 * ==  (uint32_t)(-limit) % limit
		 *
		 * This division is safe: we know that `limit` is strictly
		 * greater than zero because of the slow-path test above.
		 */
		uint32_t residue = (uint32_t)(-limit) % limit;
		/*
		 * Unless we get one of `N = (1 << 32) - residue` valid
		 * values, we reject the sample. This `N` is a multiple of
		 * `limit`, so our results will be unbiased; and `N` is the
		 * largest multiple that fits in 32 bits, so rejections are as
		 * rare as possible.
		 *
		 * There are `limit` possible values for the integer part of
		 * our fixed-point number. Each one corresponds to `N/limit`
		 * or `N/limit + 1` possible fraction parts. For our result to
		 * be unbiased, every possible integer part must have the same
		 * number of possible valid fraction parts. So, when we get
		 * the superfluous value in the `N/limit + 1` cases, we need
		 * to reject and resample.
		 *
		 * Because of the multiplication, the possible values in the
		 * fraction part are equally spaced by `limit`, with varying
		 * gaps at each end of the fraction's 32-bit range. We will
		 * choose a range of size `N` (a multiple of `limit`) into
		 * which valid fraction values must fall, with the rest of the
		 * 32-bit range covered by the `residue`. Lemire's paper says
		 * that exactly `N/limit` possible values spaced apart by
		 * `limit` will fit into our size `N` valid range, regardless
		 * of the size of the end gaps, the phase alignment of the
		 * values, or the position of the range.
		 *
		 * So, when a fraction value falls in the `residue` outside
		 * our valid range, it is superfluous, and we resample.
		 */
		while ((uint32_t)(num) < residue) {
			num = (uint64_t)next() * (uint64_t)limit;
		}
	}
	/*
	 * Return the integer part (upper 32 bits).
	 */
	return (uint32_t)(num >> 32);
}
