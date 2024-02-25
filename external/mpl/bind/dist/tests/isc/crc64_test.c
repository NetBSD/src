/*	$NetBSD: crc64_test.c,v 1.2.2.2 2024/02/25 15:47:51 martin Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
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

#include <tests/isc.h>

#define TEST_INPUT(x) (x), sizeof(x) - 1

ISC_RUN_TEST_IMPL(isc_crc64_init) {
	uint64_t crc;

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
ISC_RUN_TEST_IMPL(isc_crc64) {
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

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_crc64_init)
ISC_TEST_ENTRY(isc_crc64)

ISC_TEST_LIST_END

ISC_TEST_MAIN
