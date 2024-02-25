/*	$NetBSD: counter_test.c,v 1.2.2.2 2024/02/25 15:47:51 martin Exp $	*/

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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/counter.h>
#include <isc/result.h>
#include <isc/util.h>

#include <tests/isc.h>

/* test isc_counter object */
ISC_RUN_TEST_IMPL(isc_counter) {
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

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_counter)

ISC_TEST_LIST_END

ISC_TEST_MAIN
