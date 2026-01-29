/*	$NetBSD: ede_test.c,v 1.3 2026/01/29 18:37:56 christos Exp $	*/

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

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/list.h>

#include <dns/ede.h>

#include "../../lib/dns/ede.c"

#include <tests/isc.h>

typedef struct {
	uint16_t code;
	const char *txt;
} ede_test_expected_t;

static void
dns_ede_test_equals(const ede_test_expected_t *expected, size_t expected_count,
		    dns_edectx_t *edectx) {
	size_t count = 0;

	for (size_t i = 0; i < DNS_EDE_MAX_ERRORS; i++) {
		dns_ednsopt_t *edns = edectx->ede[i];

		if (edns == NULL) {
			break;
		}

		uint16_t code;
		const unsigned char *txt;

		assert_uint_in_range(count, 0, expected_count);
		assert_int_equal(edns->code, DNS_OPT_EDE);

		code = ISC_U8TO16_BE(edns->value);
		assert_int_equal(code, expected[count].code);

		if (edns->length > sizeof(code)) {
			assert_non_null(expected[count].txt);
			txt = edns->value + sizeof(code);
			assert_memory_equal(expected[count].txt, txt,
					    edns->length - sizeof(code));
		} else {
			assert_null(expected[count].txt);
		}

		count++;
	}
	assert_int_equal(count, expected_count);
}

ISC_RUN_TEST_IMPL(dns_ede_test_text_max_count) {
	dns_edectx_t edectx;

	dns_ede_init(mctx, &edectx);

	const char *txt1 = "foobar";
	const char *txt2 = "It's been a long time since I rock-and-rolled"
			   "Ooh, let me get it back, let me get it back";

	dns_ede_add(&edectx, 2, txt1);
	dns_ede_add(&edectx, 22, NULL);
	dns_ede_add(&edectx, 3, txt2);

	const ede_test_expected_t expected[3] = {
		{ .code = 2, .txt = "foobar" },
		{ .code = 22, .txt = NULL },
		{ .code = 3,
		  .txt = "It's been a long time since I rock-and-rolledOoh, "
			 "let me get it " }
	};

	dns_ede_test_equals(expected, 3, &edectx);

	dns_ede_reset(&edectx);
}

ISC_RUN_TEST_IMPL(dns_ede_test_max_count) {
	dns_edectx_t edectx;

	dns_ede_init(mctx, &edectx);

	dns_ede_add(&edectx, 1, NULL);
	dns_ede_add(&edectx, 22, "two");
	dns_ede_add(&edectx, 3, "three");
	dns_ede_add(&edectx, 4, "four");
	dns_ede_add(&edectx, 5, "five");

	const ede_test_expected_t expected[3] = {
		{ .code = 1, .txt = NULL },
		{ .code = 22, .txt = "two" },
		{ .code = 3, .txt = "three" },
	};

	dns_ede_test_equals(expected, 3, &edectx);

	dns_ede_reset(&edectx);
}

ISC_RUN_TEST_IMPL(dns_ede_test_duplicates) {
	dns_edectx_t edectx;

	dns_ede_init(mctx, &edectx);

	dns_ede_add(&edectx, 1, NULL);
	dns_ede_add(&edectx, 1, "two");
	dns_ede_add(&edectx, 1, "three");

	const ede_test_expected_t expected[] = {
		{ .code = 1, .txt = NULL },
	};
	dns_ede_test_equals(expected, 1, &edectx);

	dns_ede_reset(&edectx);

	const ede_test_expected_t expectedempty[] = {};
	dns_ede_test_equals(expectedempty, 0, &edectx);
}

ISC_RUN_TEST_IMPL(dns_ede_test_infocode_range) {
	dns_edectx_t edectx;

	dns_ede_init(mctx, &edectx);

	dns_ede_add(&edectx, 1, NULL);
	expect_assert_failure(dns_ede_add(&edectx, 32, NULL));

	const ede_test_expected_t expected[] = {
		{ .code = 1, .txt = NULL },
	};
	dns_ede_test_equals(expected, 1, &edectx);

	dns_ede_reset(&edectx);
}

ISC_RUN_TEST_IMPL(dns_ede_test_copy) {
	dns_edectx_t edectx1;
	dns_edectx_t edectx2;
	dns_edectx_t edectx3;

	dns_ede_init(mctx, &edectx1);
	dns_ede_init(mctx, &edectx2);

	dns_ede_add(&edectx1, 1, NULL);
	dns_ede_add(&edectx1, 2, "two-the-first");
	dns_ede_add(&edectx1, 3, "three");

	const ede_test_expected_t expected[] = {
		{ .code = 1, .txt = NULL },
		{ .code = 2, .txt = "two-the-first" },
		{ .code = 3, .txt = "three" },
	};

	dns_ede_test_equals(expected, 3, &edectx1);
	dns_ede_copy(&edectx2, &edectx1);
	dns_ede_test_equals(expected, 3, &edectx2);
	dns_ede_test_equals(expected, 3, &edectx1);

	dns_ede_reset(&edectx2);
	dns_ede_add(&edectx2, 1, "one-the-first-with-txt");
	dns_ede_add(&edectx2, 2, "two-the-second");

	const ede_test_expected_t expected2[] = {
		{ .code = 1, .txt = "one-the-first-with-txt" },
		{ .code = 2, .txt = "two-the-second" },
		{ .code = 3, .txt = "three" }
	};

	dns_ede_copy(&edectx2, &edectx1);
	dns_ede_test_equals(expected2, 3, &edectx2);
	dns_ede_test_equals(expected, 3, &edectx1);

	dns_ede_init(mctx, &edectx3);
	dns_ede_add(&edectx3, 2, "two-the-third");
	dns_ede_copy(&edectx3, &edectx2);

	const ede_test_expected_t expected3[] = {
		{ .code = 2, .txt = "two-the-third" },
		{ .code = 1, .txt = "one-the-first-with-txt" },
		{ .code = 3, .txt = "three" }
	};
	dns_ede_test_equals(expected3, 3, &edectx3);

	dns_ede_reset(&edectx1);
	dns_ede_reset(&edectx2);
	dns_ede_reset(&edectx3);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(dns_ede_test_text_max_count)
ISC_TEST_ENTRY(dns_ede_test_max_count)
ISC_TEST_ENTRY(dns_ede_test_duplicates)
ISC_TEST_ENTRY(dns_ede_test_infocode_range)
ISC_TEST_ENTRY(dns_ede_test_copy)

ISC_TEST_LIST_END

ISC_TEST_MAIN
