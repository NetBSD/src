/*	$NetBSD: ascii_test.c,v 1.1.1.1 2025/01/26 16:12:37 christos Exp $	*/

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

#include <ctype.h>
#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/ascii.h>

#include <tests/isc.h>

const char *same[][2] = {
	{
		"AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
	},
	{
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
		"AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSSTTUUVVWWXXYYZZ",
	},
	{
		"AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSSTTUUVVWWXXYYZZ",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
	},
	{
		"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",
	},
	{
		"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVxXyYzZ",
		"aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvxxyyzz",
	},
	{
		"WwW.ExAmPlE.OrG",
		"wWw.eXaMpLe.oRg",
	},
	{
		"_SIP.tcp.example.org",
		"_sip.TCP.example.org",
	},
	{
		"bind-USERS.lists.example.org",
		"bind-users.lists.example.org",
	},
	{
		"a0123456789.example.org",
		"A0123456789.example.org",
	},
	{
		"\\000.example.org",
		"\\000.example.org",
	},
	{
		"wWw.\\000.isc.org",
		"www.\\000.isc.org",
	},
	{
		"\255.example.org",
		"\255.example.ORG",
	}
};

struct {
	const char *a, *b;
	int cmp;
} diff[] = {
	{ "foo", "bar", +1 },
	{ "bar", "foo", -1 },
	{ "foosuffix", "barsuffix", +1 },
	{ "barsuffix", "foosuffix", -1 },
	{ "prefixfoo", "prefixbar", +1 },
	{ "prefixbar", "prefixfoo", -1 },
};

ISC_RUN_TEST_IMPL(upperlower) {
	UNUSED(state);

	for (size_t n = 0; n < ARRAY_SIZE(same); n++) {
		const char *a = same[n][0];
		const char *b = same[n][1];
		for (size_t i = 0; a[i] != '\0' && b[i] != '\0'; i++) {
			assert_true(isc_ascii_toupper(a[i]) == (uint8_t)a[i] ||
				    isc_ascii_tolower(a[i]) == (uint8_t)a[i]);
			assert_true(isc_ascii_toupper(b[i]) == (uint8_t)b[i] ||
				    isc_ascii_tolower(b[i]) == (uint8_t)b[i]);
			assert_true(isc_ascii_toupper(a[i]) ==
				    isc_ascii_toupper(b[i]));
			assert_true(isc_ascii_tolower(a[i]) ==
				    isc_ascii_tolower(b[i]));
		}
	}
}

ISC_RUN_TEST_IMPL(lowerequal) {
	for (size_t n = 0; n < ARRAY_SIZE(same); n++) {
		const uint8_t *a = (void *)same[n][0];
		const uint8_t *b = (void *)same[n][1];
		unsigned int len = (unsigned int)strlen(same[n][0]);
		assert_true(isc_ascii_lowerequal(a, b, len));
	}
	for (size_t n = 0; n < ARRAY_SIZE(diff); n++) {
		const uint8_t *a = (void *)diff[n].a;
		const uint8_t *b = (void *)diff[n].b;
		unsigned int len = (unsigned int)strlen(diff[n].a);
		assert_true(!isc_ascii_lowerequal(a, b, len));
	}
}

ISC_RUN_TEST_IMPL(lowercmp) {
	for (size_t n = 0; n < ARRAY_SIZE(same); n++) {
		const uint8_t *a = (void *)same[n][0];
		const uint8_t *b = (void *)same[n][1];
		unsigned int len = (unsigned int)strlen(same[n][0]);
		assert_true(isc_ascii_lowercmp(a, b, len) == 0);
	}
	for (size_t n = 0; n < ARRAY_SIZE(diff); n++) {
		const uint8_t *a = (void *)diff[n].a;
		const uint8_t *b = (void *)diff[n].b;
		unsigned int len = (unsigned int)strlen(diff[n].a);
		assert_true(isc_ascii_lowercmp(a, b, len) == diff[n].cmp);
	}
}

ISC_RUN_TEST_IMPL(exhaustive) {
	for (uint64_t ab = 0; ab < (1 << 16); ab++) {
		uint8_t a = ab >> 8;
		uint8_t b = ab & 0xFF;
		uint64_t abc = tolower(a) << 8 | tolower(b);
		uint64_t abi = isc_ascii_tolower(a) << 8 | isc_ascii_tolower(b);
		uint64_t ab1 = isc__ascii_tolower1(a) << 8 |
			       isc__ascii_tolower1(b);
		uint64_t ab8 = isc_ascii_tolower8(ab);
		/* each byte individually matches ctype.h */
		assert_int_equal(tolower(a), isc_ascii_tolower(a));
		assert_int_equal(tolower(a), isc__ascii_tolower1(a));
		assert_int_equal(tolower(a), isc_ascii_tolower8(a));
		assert_int_equal(tolower(b), isc_ascii_tolower(b));
		assert_int_equal(tolower(b), isc__ascii_tolower1(b));
		assert_int_equal(tolower(b), isc_ascii_tolower8(b));
		/* two lanes of SWAR match other implementations */
		assert_int_equal(ab8, abc);
		assert_int_equal(ab8, abi);
		assert_int_equal(ab8, ab1);
		/* check lack of overflow */
		assert_int_equal(ab8 >> 16, 0);
		/* all lanes of SWAR work */
		assert_int_equal(isc_ascii_tolower8(ab << 8), abc << 8);
		assert_int_equal(isc_ascii_tolower8(ab << 16), abc << 16);
		assert_int_equal(isc_ascii_tolower8(ab << 24), abc << 24);
		assert_int_equal(isc_ascii_tolower8(ab << 32), abc << 32);
		assert_int_equal(isc_ascii_tolower8(ab << 40), abc << 40);
		assert_int_equal(isc_ascii_tolower8(ab << 48), abc << 48);
	}
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY(upperlower)
ISC_TEST_ENTRY(lowerequal)
ISC_TEST_ENTRY(lowercmp)
ISC_TEST_ENTRY(exhaustive)
ISC_TEST_LIST_END

ISC_TEST_MAIN
