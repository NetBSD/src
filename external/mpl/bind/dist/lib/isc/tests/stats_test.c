/*	$NetBSD: stats_test.c,v 1.2 2022/09/23 12:15:34 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/stats.h>
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

/* test stats */
static void
isc_stats_basic_test(void **state) {
	isc_stats_t *stats = NULL;
	isc_result_t result;

	UNUSED(state);

	result = isc_stats_create(test_mctx, &stats, 4);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_stats_ncounters(stats), 4);

	/* Default all 0. */
	for (int i = 0; i < isc_stats_ncounters(stats); i++) {
		assert_int_equal(isc_stats_get_counter(stats, i), 0);
	}

	/* Test increment. */
	for (int i = 0; i < isc_stats_ncounters(stats); i++) {
		isc_stats_increment(stats, i);
		assert_int_equal(isc_stats_get_counter(stats, i), 1);
		isc_stats_increment(stats, i);
		assert_int_equal(isc_stats_get_counter(stats, i), 2);
	}

	/* Test decrement. */
	for (int i = 0; i < isc_stats_ncounters(stats); i++) {
		isc_stats_decrement(stats, i);
		assert_int_equal(isc_stats_get_counter(stats, i), 1);
		isc_stats_decrement(stats, i);
		assert_int_equal(isc_stats_get_counter(stats, i), 0);
	}

	/* Test set. */
	for (int i = 0; i < isc_stats_ncounters(stats); i++) {
		isc_stats_set(stats, i, i);
		assert_int_equal(isc_stats_get_counter(stats, i), i);
	}

	/* Test update if greater. */
	for (int i = 0; i < isc_stats_ncounters(stats); i++) {
		isc_stats_update_if_greater(stats, i, i);
		assert_int_equal(isc_stats_get_counter(stats, i), i);
		isc_stats_update_if_greater(stats, i, i + 1);
		assert_int_equal(isc_stats_get_counter(stats, i), i + 1);
	}

	/* Test resize. */
	isc_stats_resize(&stats, 3);
	assert_int_equal(isc_stats_ncounters(stats), 4);
	isc_stats_resize(&stats, 4);
	assert_int_equal(isc_stats_ncounters(stats), 4);
	isc_stats_resize(&stats, 5);
	assert_int_equal(isc_stats_ncounters(stats), 5);

	/* Existing counters are retained */
	for (int i = 0; i < isc_stats_ncounters(stats); i++) {
		uint32_t expect = i + 1;
		if (i == 4) {
			expect = 0;
		}
		assert_int_equal(isc_stats_get_counter(stats, i), expect);
	}

	isc_stats_detach(&stats);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(isc_stats_basic_test, _setup,
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
