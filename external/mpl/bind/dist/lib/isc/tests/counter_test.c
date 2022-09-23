/*	$NetBSD: counter_test.c,v 1.9 2022/09/23 12:15:34 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/counter.h>
#include <isc/result.h>
#include <isc/util.h>

#include "isctest.h"

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = isc_test_begin(NULL, true, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	isc_test_end();

	return (0);
}

/* test isc_counter object */
static void
isc_counter_test(void **state) {
	isc_result_t result;
	isc_counter_t *counter = NULL;
	int i;

	UNUSED(state);

	result = isc_counter_create(test_mctx, 0, &counter);
	assert_int_equal(result, ISC_R_SUCCESS);

	for (i = 0; i < 10; i++) {
		result = isc_counter_increment(counter);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	assert_int_equal(isc_counter_used(counter), 10);

	isc_counter_setlimit(counter, 15);
	for (i = 0; i < 10; i++) {
		result = isc_counter_increment(counter);
		if (result != ISC_R_SUCCESS) {
			break;
		}
	}

	assert_int_equal(isc_counter_used(counter), 15);

	isc_counter_detach(&counter);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(isc_counter_test, _setup,
						_teardown),
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
