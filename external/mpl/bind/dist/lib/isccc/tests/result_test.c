/*	$NetBSD: result_test.c,v 1.9 2022/09/23 12:15:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#if HAVE_CMOCKA

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/result.h>
#include <isc/util.h>

#include <isccc/result.h>

/*
 * Check tables are populated.
 */
static void
tables(void **state) {
	const char *str;
	isc_result_t result;

	UNUSED(state);

	isccc_result_register();

	for (result = ISC_RESULTCLASS_ISCCC;
	     result < (ISC_RESULTCLASS_ISCCC + ISCCC_R_NRESULTS); result++)
	{
		str = isc_result_toid(result);
		assert_non_null(str);
		assert_string_not_equal(str, "(result code text not "
					     "available)");

		str = isc_result_totext(result);
		assert_non_null(str);
		assert_string_not_equal(str, "(result code text not "
					     "available)");
	}

	str = isc_result_toid(result);
	assert_non_null(str);
	assert_string_equal(str, "(result code text not available)");

	str = isc_result_totext(result);
	assert_non_null(str);
	assert_string_equal(str, "(result code text not available)");
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(tables),
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
