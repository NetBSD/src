/*	$NetBSD: buffer_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <config.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <atf-c.h>

#include "isctest.h"

#include <isc/buffer.h>
#include <isc/print.h>
#include <isc/result.h>

ATF_TC(isc_buffer_reserve);
ATF_TC_HEAD(isc_buffer_reserve, tc) {
	atf_tc_set_md_var(tc, "descr", "reserve space in dynamic buffers");
}

ATF_TC_BODY(isc_buffer_reserve, tc) {
	isc_result_t result;
	isc_buffer_t *b;

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	b = NULL;
	result = isc_buffer_allocate(mctx, &b, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK_EQ(b->length, 1024);

	/*
	 * 1024 bytes should already be available, so this call does
	 * nothing.
	 */
	result = isc_buffer_reserve(&b, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(ISC_BUFFER_VALID(b));
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 1024);

	/*
	 * This call should grow it to 2048 bytes as only 1024 bytes are
	 * available in the buffer.
	 */
	result = isc_buffer_reserve(&b, 1025);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(ISC_BUFFER_VALID(b));
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 2048);

	/*
	 * 2048 bytes should already be available, so this call does
	 * nothing.
	 */
	result = isc_buffer_reserve(&b, 2000);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(ISC_BUFFER_VALID(b));
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 2048);

	/*
	 * This call should grow it to 4096 bytes as only 2048 bytes are
	 * available in the buffer.
	 */
	result = isc_buffer_reserve(&b, 3000);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(ISC_BUFFER_VALID(b));
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 4096);

	/* Consume some of the buffer so we can run the next test. */
	isc_buffer_add(b, 4096);

	/*
	 * This call should fail and leave buffer untouched.
	 */
	result = isc_buffer_reserve(&b, UINT_MAX);
	ATF_CHECK_EQ(result, ISC_R_NOMEMORY);
	ATF_CHECK(ISC_BUFFER_VALID(b));
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 4096);

	isc_buffer_free(&b);

	isc_test_end();
}

ATF_TC(isc_buffer_reallocate);
ATF_TC_HEAD(isc_buffer_reallocate, tc) {
	atf_tc_set_md_var(tc, "descr", "reallocate dynamic buffers");
}

ATF_TC_BODY(isc_buffer_reallocate, tc) {
	isc_result_t result;
	isc_buffer_t *b;

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	b = NULL;
	result = isc_buffer_allocate(mctx, &b, 1024);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 1024);

	result = isc_buffer_reallocate(&b, 512);
	ATF_CHECK_EQ(result, ISC_R_NOSPACE);
	ATF_CHECK(ISC_BUFFER_VALID(b));
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 1024);

	result = isc_buffer_reallocate(&b, 1536);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_CHECK(ISC_BUFFER_VALID(b));
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, 1536);

	isc_buffer_free(&b);

	isc_test_end();
}

ATF_TC(isc_buffer_dynamic);
ATF_TC_HEAD(isc_buffer_dynamic, tc) {
	atf_tc_set_md_var(tc, "descr", "dynamic buffer automatic reallocation");
}

ATF_TC_BODY(isc_buffer_dynamic, tc) {
	isc_result_t result;
	isc_buffer_t *b;
	size_t last_length = 10;
	int i;

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	b = NULL;
	result = isc_buffer_allocate(mctx, &b, last_length);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE(b != NULL);
	ATF_CHECK_EQ(b->length, last_length);

	isc_buffer_setautorealloc(b, ISC_TRUE);

	isc_buffer_putuint8(b, 1);

	for (i = 0; i < 1000; i++) {
		isc_buffer_putstr(b, "thisisa24charslongstring");
	}
	ATF_CHECK(b->length-last_length >= 1000*24);
	last_length+=1000*24;

	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint8(b, 1);
	}

	ATF_CHECK(b->length-last_length >= 10000*1);
	last_length += 10000*1;

	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint16(b, 1);
	}

	ATF_CHECK(b->length-last_length >= 10000*2);

	last_length += 10000*2;
	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint24(b, 1);
	}
	ATF_CHECK(b->length-last_length >= 10000*3);

	last_length+=10000*3;

	for (i = 0; i < 10000; i++) {
		isc_buffer_putuint32(b, 1);
	}
	ATF_CHECK(b->length-last_length >= 10000*4);


	isc_buffer_free(&b);

	isc_test_end();
}

ATF_TC(isc_buffer_printf);
ATF_TC_HEAD(isc_buffer_printf, tc) {
	atf_tc_set_md_var(tc, "descr", "printf() into a buffer");
}

ATF_TC_BODY(isc_buffer_printf, tc) {
	unsigned int used, prev_used;
	const char *empty_fmt;
	isc_result_t result;
	isc_buffer_t *b, sb;
	char buf[8];

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Prepare a buffer with auto-reallocation enabled.
	 */
	b = NULL;
	result = isc_buffer_allocate(mctx, &b, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_buffer_setautorealloc(b, ISC_TRUE);

	/*
	 * Sanity check.
	 */
	result = isc_buffer_printf(b, "foo");
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(b);
	ATF_CHECK_EQ(used, 3);

	result = isc_buffer_printf(b, "bar");
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(b);
	ATF_CHECK_EQ(used, 3 + 3);

	/*
	 * Also check the terminating NULL byte is there, even though it is not
	 * part of the buffer's used region.
	 */
	ATF_CHECK_EQ(memcmp(isc_buffer_current(b), "foobar", 7), 0);

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
	ATF_CHECK_EQ(used - prev_used, 2);

	isc_buffer_printf(b, "baz%1X", 42);
	used = isc_buffer_usedlength(b);
	ATF_CHECK_EQ(used - prev_used, 2 + 5);

	isc_buffer_printf(b, "%6.1f", 42.42f);
	used = isc_buffer_usedlength(b);
	ATF_CHECK_EQ(used - prev_used, 2 + 5 + 6);

	/*
	 * Also check the terminating NULL byte is there, even though it is not
	 * part of the buffer's used region.
	 */
	ATF_CHECK_EQ(memcmp(isc_buffer_current(b), "42baz2A  42.4", 14), 0);

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
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(b);
	ATF_CHECK_EQ(prev_used, used);

	isc_buffer_free(&b);

	/*
	 * Check overflow on a static buffer.
	 */
	isc_buffer_init(&sb, buf, sizeof(buf));
	result = isc_buffer_printf(&sb, "123456");
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(&sb);
	ATF_CHECK_EQ(used, 6);

	result = isc_buffer_printf(&sb, "789");
	ATF_CHECK_EQ(result, ISC_R_NOSPACE);
	used = isc_buffer_usedlength(&sb);
	ATF_CHECK_EQ(used, 6);

	result = isc_buffer_printf(&sb, "78");
	ATF_CHECK_EQ(result, ISC_R_NOSPACE);
	used = isc_buffer_usedlength(&sb);
	ATF_CHECK_EQ(used, 6);

	result = isc_buffer_printf(&sb, "7");
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);
	used = isc_buffer_usedlength(&sb);
	ATF_CHECK_EQ(used, 7);

	isc_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, isc_buffer_reserve);
	ATF_TP_ADD_TC(tp, isc_buffer_reallocate);
	ATF_TP_ADD_TC(tp, isc_buffer_dynamic);
	ATF_TP_ADD_TC(tp, isc_buffer_printf);
	return (atf_no_error());
}
