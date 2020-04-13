/*	$NetBSD: counter_test.c,v 1.4.2.3 2020/04/13 08:02:59 martin Exp $	*/

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

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <sched.h> /* IWYU pragma: keep */
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

	result = isc_counter_create(mctx, 0, &counter);
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
		cmocka_unit_test_setup_teardown(isc_counter_test,
						_setup, _teardown),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
