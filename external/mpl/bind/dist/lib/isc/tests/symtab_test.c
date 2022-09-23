/*	$NetBSD: symtab_test.c,v 1.8 2022/09/23 12:15:34 christos Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/print.h>
#include <isc/symtab.h>
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

static void
undefine(char *key, unsigned int type, isc_symvalue_t value, void *arg) {
	UNUSED(arg);

	assert_int_equal(type, 1);
	isc_mem_free(test_mctx, key);
	isc_mem_free(test_mctx, value.as_pointer);
}

/* test symbol table growth */
static void
symtab_grow(void **state) {
	isc_result_t result;
	isc_symtab_t *st = NULL;
	isc_symvalue_t value;
	isc_symexists_t policy = isc_symexists_reject;
	int i;

	UNUSED(state);

	result = isc_symtab_create(test_mctx, 3, undefine, NULL, false, &st);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(st);

	/* Nothing should be in the table yet */

	/*
	 * Put 1024 entries in the table (this should necessate
	 * regrowing the hash table several times
	 */
	for (i = 0; i < 1024; i++) {
		char str[16], *key;

		snprintf(str, sizeof(str), "%04x", i);
		key = isc_mem_strdup(test_mctx, str);
		assert_non_null(key);
		value.as_pointer = isc_mem_strdup(test_mctx, str);
		assert_non_null(value.as_pointer);
		result = isc_symtab_define(st, key, 1, value, policy);
		assert_int_equal(result, ISC_R_SUCCESS);
		if (result != ISC_R_SUCCESS) {
			undefine(key, 1, value, NULL);
		}
	}

	/*
	 * Try to put them in again; this should fail
	 */
	for (i = 0; i < 1024; i++) {
		char str[16], *key;

		snprintf(str, sizeof(str), "%04x", i);
		key = isc_mem_strdup(test_mctx, str);
		assert_non_null(key);
		value.as_pointer = isc_mem_strdup(test_mctx, str);
		assert_non_null(value.as_pointer);
		result = isc_symtab_define(st, key, 1, value, policy);
		assert_int_equal(result, ISC_R_EXISTS);
		undefine(key, 1, value, NULL);
	}

	/*
	 * Retrieve them; this should succeed
	 */
	for (i = 0; i < 1024; i++) {
		char str[16];

		snprintf(str, sizeof(str), "%04x", i);
		result = isc_symtab_lookup(st, str, 0, &value);
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_string_equal(str, (char *)value.as_pointer);
	}

	/*
	 * Undefine them
	 */
	for (i = 0; i < 1024; i++) {
		char str[16];

		snprintf(str, sizeof(str), "%04x", i);
		result = isc_symtab_undefine(st, str, 1);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	/*
	 * Retrieve them again; this should fail
	 */
	for (i = 0; i < 1024; i++) {
		char str[16];

		snprintf(str, sizeof(str), "%04x", i);
		result = isc_symtab_lookup(st, str, 0, &value);
		assert_int_equal(result, ISC_R_NOTFOUND);
	}

	isc_symtab_destroy(&st);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(symtab_grow, _setup, _teardown),
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
