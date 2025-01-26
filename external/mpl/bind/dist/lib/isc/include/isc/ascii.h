/*	$NetBSD: ascii.h,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <isc/endian.h>

/*
 * ASCII case conversion
 */
extern const uint8_t isc__ascii_tolower[256];
extern const uint8_t isc__ascii_toupper[256];

/*
 * Wrappers so we don't have to cast all over the place like <ctype.h>
 */
#define isc_ascii_tolower(c) isc__ascii_tolower[(uint8_t)(c)]
#define isc_ascii_toupper(c) isc__ascii_toupper[(uint8_t)(c)]

/*
 * A variant tolower() implementation with no memory accesses,
 * for use when the compiler is able to autovectorize.
 */
static inline uint8_t
isc__ascii_tolower1(uint8_t c) {
	return c + ('a' - 'A') * ('A' <= c && c <= 'Z');
}

/*
 * Copy `len` bytes from `src` to `dst`, converting to lower case.
 */
static inline void
isc_ascii_lowercopy(uint8_t *dst, const uint8_t *src, unsigned int len) {
	while (len-- > 0) {
		*dst++ = isc__ascii_tolower1(*src++);
	}
}

/*
 * Convert a string to lower case in place
 */
static inline void
isc_ascii_strtolower(char *str) {
	isc_ascii_lowercopy((uint8_t *)str, (uint8_t *)str,
			    (unsigned int)strlen(str));
}

/*
 * Convert 8 bytes to lower case, using SWAR tricks (SIMD within a register).
 * Based on "Hacker's Delight" by Henry S. Warren, "searching for a value in a
 * given range", p. 95. Eight bytes is wider than many labels in DNS names, so
 * it does not seem worth dealing with the portability issues of wide vector
 * registers. If there was a vector string load instruction (analogous to
 * memove() below) the balance might be different.
 */
static inline uint64_t
isc_ascii_tolower8(uint64_t octets) {
	/*
	 * Multiply a single-byte constant by `all_bytes` to replicate
	 * it to all eight bytes in a word.
	 */
	uint64_t all_bytes = 0x0101010101010101;
	/*
	 * Clear the top bit of each byte to make space for a per-byte flag.
	 */
	uint64_t heptets = octets & (0x7F * all_bytes);
	/*
	 * We will need to avoid going wrong if our flag bits were originally
	 * set, and clear calculation leftovers in our non-flag bits
	 */
	uint64_t is_ascii = ~octets & (0x80 * all_bytes);
	/*
	 * To compare a heptet to `N`, we can add `0x7F - N` so that carry
	 * propagation will set the flag when our heptet is greater than `N`
	 */
	uint64_t is_gt_Z = heptets + (0x7F - 'Z') * all_bytes;
	/*
	 * Add one for greater-than-or-equal comparison
	 */
	uint64_t is_ge_A = heptets + (0x80 - 'A') * all_bytes;
	/*
	 * Now we have what we need to identify the ascii uppercase bytes
	 */
	uint64_t is_upper = (is_ge_A ^ is_gt_Z) & is_ascii;
	/*
	 * Move the is_upper flag bits to bit 0x20 (which is 'a' - 'A')
	 * and use them to adjust each byte as required
	 */
	return octets | (is_upper >> 2);
}

/*
 * Same, but 4 bytes at a time, used by isc_halfsiphash24()
 */
static inline uint32_t
isc_ascii_tolower4(uint32_t octets) {
	uint32_t all_bytes = 0x01010101;
	uint32_t heptets = octets & (0x7F * all_bytes);
	uint32_t is_ascii = ~octets & (0x80 * all_bytes);
	uint32_t is_gt_Z = heptets + (0x7F - 'Z') * all_bytes;
	uint32_t is_ge_A = heptets + (0x80 - 'A') * all_bytes;
	uint32_t is_upper = (is_ge_A ^ is_gt_Z) & is_ascii;
	return octets | (is_upper >> 2);
}

/*
 * Helper function to do an unaligned load of 8 bytes in host byte order
 */
static inline uint64_t
isc__ascii_load8(const uint8_t *ptr) {
	uint64_t bytes = 0;
	memmove(&bytes, ptr, sizeof(bytes));
	return bytes;
}

/*
 * Compare `len` bytes at `a` and `b` for case-insensitive equality
 */
static inline bool
isc_ascii_lowerequal(const uint8_t *a, const uint8_t *b, unsigned int len) {
	uint64_t a8 = 0, b8 = 0;
	while (len >= 8) {
		a8 = isc_ascii_tolower8(isc__ascii_load8(a));
		b8 = isc_ascii_tolower8(isc__ascii_load8(b));
		if (a8 != b8) {
			return false;
		}
		len -= 8;
		a += 8;
		b += 8;
	}
	while (len-- > 0) {
		if (isc_ascii_tolower(*a++) != isc_ascii_tolower(*b++)) {
			return false;
		}
	}
	return true;
}

/*
 * Compare `len` bytes at `a` and `b` for case-insensitive order.
 * Unlike the previous functions (which do not need to care about byte
 * order) here we need to ensure the comparisons are lexicographic,
 * i.e. they treat the strings as big-endian numbers.
 */
static inline int
isc_ascii_lowercmp(const uint8_t *a, const uint8_t *b, unsigned int len) {
	uint64_t a8 = 0, b8 = 0;
	while (len >= 8) {
		a8 = isc_ascii_tolower8(htobe64(isc__ascii_load8(a)));
		b8 = isc_ascii_tolower8(htobe64(isc__ascii_load8(b)));
		if (a8 != b8) {
			goto ret;
		}
		len -= 8;
		a += 8;
		b += 8;
	}
	while (len-- > 0) {
		a8 = isc_ascii_tolower(*a++);
		b8 = isc_ascii_tolower(*b++);
		if (a8 != b8) {
			goto ret;
		}
	}
ret:
	if (a8 < b8) {
		return -1;
	}
	if (a8 > b8) {
		return +1;
	}
	return 0;
}
