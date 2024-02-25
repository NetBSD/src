/*	$NetBSD: result_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/result.h>
#include <isc/util.h>

#include <tests/isc.h>

/* convert result to identifier string */
ISC_RUN_TEST_IMPL(isc_result_toid) {
	const char *id;

	UNUSED(state);

	id = isc_result_toid(ISC_R_SUCCESS);
	assert_string_equal("ISC_R_SUCCESS", id);

	id = isc_result_toid(ISC_R_FAILURE);
	assert_string_equal("ISC_R_FAILURE", id);
}

/* convert result to description string */
ISC_RUN_TEST_IMPL(isc_result_totext) {
	const char *str;

	UNUSED(state);

	str = isc_result_totext(ISC_R_SUCCESS);
	assert_string_equal("success", str);

	str = isc_result_totext(ISC_R_FAILURE);
	assert_string_equal("failure", str);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_result_toid)
ISC_TEST_ENTRY(isc_result_totext)

ISC_TEST_LIST_END

ISC_TEST_MAIN
