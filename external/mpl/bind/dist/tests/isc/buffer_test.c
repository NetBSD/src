/*	$NetBSD: buffer_test.c,v 1.2.2.2 2024/02/25 15:47:51 martin Exp $	*/

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

#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/buffer.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <tests/isc.h>

/* reserve space in dynamic buffers */
ISC_RUN_TEST_IMPL(isc_buffer_reserve) {
	isc_result_t result;
	isc_buffer_t *b;

	UNUSED(state);

	b = NULL;
	isc_buffer_allocate(mctx, &b, 1024);
	assert_int_equal(b->length, 1024);

	/*
	 * 1024 bytes should already be available, so this call does
	 * nothing.
	 */
	result = isc_buffer_reserve(&b, 1024);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(ISC_BUFFER_VALID(b));
	assert_non_null(b);
	assert_int_equal(b->length, 1024);

	/*
	 * This call should grow it to 2048 bytes as only 1024 bytes are
	 * available in the buffer.
	 */
	result = isc_buffer_reserve(&b, 1025);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(ISC_BUFFER_VALID(b));
	assert_non_null(b);
	assert_int_equal(b->length, 2048);

	/*
	 * 2048 bytes should already be available, so this call does
	 * nothing.
	 */
	result = isc_buffer_reserve(&b, 2000);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(ISC_BUFFER_VALID(b));
	assert_non_null(b);
	assert_int_equal(b->length, 2048);

	/*
	 * This call should grow it to 4096 bytes as only 2048 bytes are
	 * available in the buffer.
	 */
	result = isc_buffer_reserve(&b, 3000);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_true(ISC_BUFFER_VALID(b));
	assert_non_null(b);
	assert_int_equal(b->length, 4096);

	/* Consume some of the buffer so we can run the next test. */
	isc_buffer_add(b, 4096);

	/*
	 * This call should fail and leave buffer untouched.
	 */
	result = isc_buffer_reserve(&b, UINT_MAX);
	assert_int_equal(result, ISC_R_NOMEMORY);
	assert_true(ISC_BUFFER_VALID(b));
	assert_non_null(b);
	assert_int_equal(b->length, 4096);

	isc_buffer_free(&b);
}

/* dynamic buffer automatic reallocation */
ISC_RUN_TEST_IMPL(isc_buffer_dynamic) {
	isc_buffer_t *b;
	size_t last_length = 10;
	int i;

	UNUSED(state);

	b = NULL;
	isc_buffer_allocate(mctx, &b, last_length);
	assert_non_null(b);
	assert_int_equal(b->length, last_length);

	isc_buffer_setautorealloc(b, true);

	isc_buffer_putuint8(b, 1);

	for (i = 0; i < 1000; i++) {
		isc_buffer_putstr(b, "thisisa24charslongstring");
	}
	assert_true(b->length - last_length >= 1000 * 24);
	last_length += 1000 * 24;

	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint8(b, 1);
	}

	assert_true(b->length - last_length >= 10000 * 1);
	last_length += 10000 * 1;

	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint16(b, 1);
	}

	assert_true(b->length - last_length >= 10000 * 2);

	last_length += 10000 * 2;
	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint24(b, 1);
	}
	assert_true(b->length - last_length >= 10000 * 3);

	last_length += 10000 * 3;

	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint32(b, 1);
	}
	assert_true(b->length - last_length >= 10000 * 4);

	isc_buffer_free(&b);
}

/* copy a region into a buffer */
ISC_RUN_TEST_IMPL(isc_buffer_copyregion) {
	unsigned char data[] = { 0x11, 0x22, 0x33, 0x44 };
	isc_buffer_t *b = NULL;
	isc_result_t result;

	isc_region_t r = {
		.base = data,
		.length = sizeof(data),
	};

	UNUSED(state);

	isc_buffer_allocate(mctx, &b, sizeof(data));

	/*
	 * Fill originally allocated buffer space.
	 */
	result = isc_buffer_copyregion(b, &r);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Appending more data to the buffer should fail.
	 */
	result = isc_buffer_copyregion(b, &r);
	assert_int_equal(result, ISC_R_NOSPACE);

	/*
	 * Enable auto reallocation and retry.  Appending should now succeed.
	 */
	isc_buffer_setautorealloc(b, true);
	result = isc_buffer_copyregion(b, &r);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_free(&b);
}

/* sprintf() into a buffer */
ISC_RUN_TEST_IMPL(isc_buffer_printf) {
	unsigned int used, prev_used;
	const char *empty_fmt;
	isc_result_t result;
	isc_buffer_t *b, sb;
	char buf[8];

	UNUSED(state);

	/*
	 * Prepare a buffer with auto-reallocation enabled.
	 */
	b = NULL;
	isc_buffer_allocate(mctx, &b, 0);
	isc_buffer_setautorealloc(b, true);

	/*
	 * Sanity check.
	 */
	result = isc_buffer_printf(b, "foo");
	assert_int_equal(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(b);
	assert_int_equal(used, 3);

	result = isc_buffer_printf(b, "bar");
	assert_int_equal(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(b);
	assert_int_equal(used, 3 + 3);

	/*
	 * Also check the terminating NULL byte is there, even though it is not
	 * part of the buffer's used region.
	 */
	assert_memory_equal(isc_buffer_current(b), "foobar", 7);

	/*
	 * Skip over data from previous check to prevent failures in previous
	 * check from affecting this one.
	 */
	prev_used = used;
	isc_buffer_forward(b, prev_used);

	/*
	 * Some standard usage checks.
	 */
	isc_buffer_printf(b, "%d", 42);
	used = isc_buffer_usedlength(b);
	assert_int_equal(used - prev_used, 2);

	isc_buffer_printf(b, "baz%1X", 42);
	used = isc_buffer_usedlength(b);
	assert_int_equal(used - prev_used, 2 + 5);

	isc_buffer_printf(b, "%6.1f", 42.42f);
	used = isc_buffer_usedlength(b);
	assert_int_equal(used - prev_used, 2 + 5 + 6);

	/*
	 * Also check the terminating NULL byte is there, even though it is not
	 * part of the buffer's used region.
	 */
	assert_memory_equal(isc_buffer_current(b), "42baz2A  42.4", 14);

	/*
	 * Check an empty format string is properly handled.
	 *
	 * Note: we don't use a string literal for the format string to
	 * avoid triggering [-Werror=format-zero-length].
	 * Note: we have a dummy third argument as some compilers complain
	 * without it.
	 */
	prev_used = used;
	empty_fmt = "";
	result = isc_buffer_printf(b, empty_fmt, "");
	assert_int_equal(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(b);
	assert_int_equal(prev_used, used);

	isc_buffer_free(&b);

	/*
	 * Check overflow on a static buffer.
	 */
	isc_buffer_init(&sb, buf, sizeof(buf));
	result = isc_buffer_printf(&sb, "123456");
	assert_int_equal(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(&sb);
	assert_int_equal(used, 6);

	result = isc_buffer_printf(&sb, "789");
	assert_int_equal(result, ISC_R_NOSPACE);
	used = isc_buffer_usedlength(&sb);
	assert_int_equal(used, 6);

	result = isc_buffer_printf(&sb, "78");
	assert_int_equal(result, ISC_R_NOSPACE);
	used = isc_buffer_usedlength(&sb);
	assert_int_equal(used, 6);

	result = isc_buffer_printf(&sb, "7");
	assert_int_equal(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(&sb);
	assert_int_equal(used, 7);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_buffer_reserve)
ISC_TEST_ENTRY(isc_buffer_dynamic)
ISC_TEST_ENTRY(isc_buffer_copyregion)
ISC_TEST_ENTRY(isc_buffer_printf)

ISC_TEST_LIST_END

ISC_TEST_MAIN
