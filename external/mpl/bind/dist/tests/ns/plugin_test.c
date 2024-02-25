/*	$NetBSD: plugin_test.c,v 1.2.2.2 2024/02/25 15:47:54 martin Exp $	*/

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
#include <limits.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/attributes.h>
#include <isc/dir.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

noreturn void
_fail(const char *const file, const int line);

#include <ns/hooks.h>

#include <tests/ns.h>

/*%
 * Structure containing parameters for run_full_path_test().
 */
typedef struct {
	const ns_test_id_t id; /* libns test identifier */
	const char *input;     /* source string - plugin name or path
				* */
	size_t output_size;    /* size of target char array to
				* allocate */
	isc_result_t result;   /* expected return value */
	const char *output;    /* expected output string */
} ns_plugin_expandpath_test_params_t;

/*%
 * Perform a single ns_plugin_expandpath() check using given parameters.
 */
static void
run_full_path_test(const ns_plugin_expandpath_test_params_t *test,
		   void **state) {
	char **target = (char **)state;
	isc_result_t result;

	REQUIRE(test != NULL);
	REQUIRE(test->id.description != NULL);
	REQUIRE(test->input != NULL);
	REQUIRE(test->result != ISC_R_SUCCESS || test->output != NULL);

	/*
	 * Prepare a target buffer of given size.  Store it in 'state' so that
	 * it can get cleaned up by _teardown() if the test fails.
	 */
	*target = isc_mem_allocate(mctx, test->output_size);

	/*
	 * Call ns_plugin_expandpath().
	 */
	result = ns_plugin_expandpath(test->input, *target, test->output_size);

	/*
	 * Check return value.
	 */
	if (result != test->result) {
		fail_msg("# test \"%s\" on line %d: "
			 "expected result %d (%s), got %d (%s)",
			 test->id.description, test->id.lineno, test->result,
			 isc_result_totext(test->result), result,
			 isc_result_totext(result));
	}

	/*
	 * Check output string if return value indicates success.
	 */
	if (result == ISC_R_SUCCESS && strcmp(*target, test->output) != 0) {
		fail_msg("# test \"%s\" on line %d: "
			 "expected output \"%s\", got \"%s\"",
			 test->id.description, test->id.lineno, test->output,
			 *target);
	}

	isc_mem_free(mctx, *target);
}

/* test ns_plugin_expandpath() */
ISC_RUN_TEST_IMPL(ns_plugin_expandpath) {
	size_t i;

	const ns_plugin_expandpath_test_params_t tests[] = {
		{
			NS_TEST_ID("correct use with an absolute path"),
			.input = "/usr/lib/named/foo.so",
			.output_size = PATH_MAX,
			.result = ISC_R_SUCCESS,
			.output = "/usr/lib/named/foo.so",
		},
		{
			NS_TEST_ID("correct use with a relative path"),
			.input = "../../foo.so",
			.output_size = PATH_MAX,
			.result = ISC_R_SUCCESS,
			.output = "../../foo.so",
		},
		{
			NS_TEST_ID("correct use with a filename"),
			.input = "foo.so",
			.output_size = PATH_MAX,
			.result = ISC_R_SUCCESS,
			.output = NAMED_PLUGINDIR "/foo.so",
		},
		{
			NS_TEST_ID("no space at all in target buffer"),
			.input = "/usr/lib/named/foo.so",
			.output_size = 0,
			.result = ISC_R_NOSPACE,
		},
		{
			NS_TEST_ID("target buffer too small to fit input"),
			.input = "/usr/lib/named/foo.so",
			.output_size = 1,
			.result = ISC_R_NOSPACE,
		},
		{
			NS_TEST_ID("target buffer too small to fit NULL byte"),
			.input = "/foo.so",
			.output_size = 7,
			.result = ISC_R_NOSPACE,
		},
		{
			NS_TEST_ID("target buffer too small to fit full path"),
			.input = "foo.so",
			.output_size = 7,
			.result = ISC_R_NOSPACE,
		},
	};

	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		run_full_path_test(&tests[i], state);
	}
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(ns_plugin_expandpath, setup_managers, teardown_managers)

ISC_TEST_LIST_END

ISC_TEST_MAIN
