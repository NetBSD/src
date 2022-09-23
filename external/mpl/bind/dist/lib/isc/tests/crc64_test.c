/*	$NetBSD: crc64_test.c,v 1.7 2022/09/23 12:15:34 christos Exp $	*/

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

/* ! \file */

#if HAVE_CMOCKA

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/crc64.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/util.h>

#define TEST_INPUT(x) (x), sizeof(x) - 1

typedef struct hash_testcase {
	const char *input;
	size_t input_len;
	const char *result;
	int repeats;
} hash_testcase_t;

static void
isc_crc64_init_test(void **state) {
	uint64_t crc;

	UNUSED(state);

	isc_crc64_init(&crc);
	assert_int_equal(crc, 0xffffffffffffffffUL);
}

static void
_crc64(const char *buf, size_t buflen, const char *result, const int repeats) {
	uint64_t crc;

	isc_crc64_init(&crc);
	assert_int_equal(crc, 0xffffffffffffffffUL);

	for (int i = 0; i < repeats; i++) {
		isc_crc64_update(&crc, buf, buflen);
	}

	isc_crc64_final(&crc);

	char hex[16 + 1];
	snprintf(hex, sizeof(hex), "%016" PRIX64, crc);

	assert_memory_equal(hex, result, (result ? strlen(result) : 0));
}

/* 64-bit cyclic redundancy check */
static void
isc_crc64_test(void **state) {
	UNUSED(state);

	_crc64(TEST_INPUT(""), "0000000000000000", 1);
	_crc64(TEST_INPUT("a"), "CE73F427ACC0A99A", 1);
	_crc64(TEST_INPUT("abc"), "048B813AF9F49702", 1);
	_crc64(TEST_INPUT("message digest"), "5273F9EA7A357BF4", 1);
	_crc64(TEST_INPUT("abcdefghijklmnopqrstuvwxyz"), "59F079F9218BAAA1", 1);
	_crc64(TEST_INPUT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklm"
			  "nopqrstuvwxyz0123456789"),
	       "A36DA8F71E78B6FB", 1);
	_crc64(TEST_INPUT("123456789012345678901234567890123456789"
			  "01234567890123456789012345678901234567890"),
	       "81E5EB73C8E7874A", 1);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_crc64_init_test),
		cmocka_unit_test(isc_crc64_test),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
