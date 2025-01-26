/*	$NetBSD: ascii.c,v 1.1.1.1 2025/01/26 16:12:38 christos Exp $	*/

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

#include <isc/ascii.h>
#include <isc/random.h>
#include <isc/time.h>

#define SIZE (1024 * 1024)

typedef void
copy_fn(void *a, void *b, unsigned int len);

static void
time_it(copy_fn *copier, void *a, void *b, const char *name) {
	isc_time_t start;
	start = isc_time_now_hires();

	copier(a, b, SIZE);

	isc_time_t finish;
	finish = isc_time_now_hires();

	uint64_t microseconds = isc_time_microdiff(&finish, &start);
	printf("%f for %s\n", (double)microseconds / 1000000.0, name);
}

static void
copy_raw(void *a, void *b, unsigned int size) {
	memmove(a, b, size);
}

static void
copy_toupper(void *va, void *vb, unsigned int size) {
	uint8_t *a = va, *b = vb;
	while (size-- > 0) {
		*a++ = isc_ascii_toupper(*b++);
	}
}

static void
copy_tolower8(void *a, void *b, unsigned int size) {
	isc_ascii_lowercopy(a, b, size);
}

#define TOLOWER(c) ((c) + ('a' - 'A') * (((c) >= 'A') ^ ((c) > 'Z')))

static void
copy_tolower1(void *va, void *vb, unsigned int size) {
	for (uint8_t *a = va, *b = vb; size-- > 0; a++, b++) {
		*a = TOLOWER(*b);
	}
}

static bool
cmp_tolower1(void *va, void *vb, unsigned int size) {
	for (uint8_t *a = va, *b = vb; size-- > 0; a++, b++) {
		if (TOLOWER(*a) != TOLOWER(*b)) {
			return false;
		}
	}
	return true;
}

static bool oldskool_result;

static void
cmp_oldskool(void *va, void *vb, unsigned int size) {
	uint8_t *a = va, *b = vb, c;

	while (size > 3) {
		c = isc_ascii_tolower(a[0]);
		if (c != isc_ascii_tolower(b[0])) {
			goto diff;
		}
		c = isc_ascii_tolower(a[1]);
		if (c != isc_ascii_tolower(b[1])) {
			goto diff;
		}
		c = isc_ascii_tolower(a[2]);
		if (c != isc_ascii_tolower(b[2])) {
			goto diff;
		}
		c = isc_ascii_tolower(a[3]);
		if (c != isc_ascii_tolower(b[3])) {
			goto diff;
		}
		size -= 4;
		a += 4;
		b += 4;
	}
	while (size-- > 0) {
		c = isc_ascii_tolower(*a++);
		if (c != isc_ascii_tolower(*b++)) {
			goto diff;
		}
	}
	oldskool_result = true;
	return;
diff:
	oldskool_result = false;
	return;
}

static bool tolower1_result;

static void
vcmp_tolower1(void *a, void *b, unsigned int size) {
	tolower1_result = cmp_tolower1(a, b, size);
}

static bool swar_result;

static void
cmp_swar(void *a, void *b, unsigned int size) {
	swar_result = isc_ascii_lowerequal(a, b, size);
}

static bool chunk_result;
static unsigned int chunk_size;

static void
cmp_chunks1(void *va, void *vb, unsigned int size) {
	uint8_t *a = va, *b = vb;

	chunk_result = false;
	while (size >= chunk_size) {
		if (!cmp_tolower1(a, b, chunk_size)) {
			return;
		}
		size -= chunk_size;
		a += chunk_size;
		b += chunk_size;
	}
	chunk_result = cmp_tolower1(a, b, size);
}

static void
cmp_chunks8(void *va, void *vb, unsigned int size) {
	uint8_t *a = va, *b = vb;

	while (size >= chunk_size) {
		if (!isc_ascii_lowerequal(a, b, chunk_size)) {
			goto diff;
		}
		size -= chunk_size;
		a += chunk_size;
		b += chunk_size;
	}
	chunk_result = isc_ascii_lowerequal(a, b, size);
	return;
diff:
	chunk_result = false;
	return;
}

static void
cmp_oldchunks(void *va, void *vb, unsigned int size) {
	uint8_t *a = va, *b = vb;

	while (size >= chunk_size) {
		cmp_oldskool(a, b, chunk_size);
		if (!oldskool_result) {
			return;
		}
		size -= chunk_size;
		a += chunk_size;
		b += chunk_size;
	}
	cmp_oldskool(a, b, size);
}

int
main(void) {
	static uint8_t bytes[SIZE];

	isc_random_buf(bytes, SIZE);

	static uint8_t raw_dest[SIZE];
	time_it(copy_raw, raw_dest, bytes, "memmove");

	static uint8_t toupper_dest[SIZE];
	time_it(copy_toupper, toupper_dest, bytes, "toupper");

	static uint8_t tolower1_dest[SIZE];
	time_it(copy_tolower1, tolower1_dest, bytes, "tolower1");

	static uint8_t tolower8_dest[SIZE];
	time_it(copy_tolower8, tolower8_dest, bytes, "tolower8");

	time_it(cmp_oldskool, toupper_dest, tolower1_dest, "oldskool");
	printf("-> %s\n", oldskool_result ? "same" : "WAT");

	time_it(vcmp_tolower1, tolower1_dest, tolower8_dest, "tolower1");
	printf("-> %s\n", tolower1_result ? "same" : "WAT");

	time_it(cmp_swar, toupper_dest, tolower8_dest, "swar");
	printf("-> %s\n", swar_result ? "same" : "WAT");

	for (chunk_size = 3; chunk_size <= 15; chunk_size += 2) {
		time_it(cmp_chunks1, toupper_dest, raw_dest, "chunks1");
		printf("%u -> %s\n", chunk_size, chunk_result ? "same" : "WAT");
		time_it(cmp_chunks8, toupper_dest, raw_dest, "chunks8");
		printf("%u -> %s\n", chunk_size, chunk_result ? "same" : "WAT");
		time_it(cmp_oldchunks, toupper_dest, raw_dest, "oldchunks");
		printf("%u -> %s\n", chunk_size,
		       oldskool_result ? "same" : "WAT");
	}
}
