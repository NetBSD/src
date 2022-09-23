/*	$NetBSD: heap_test.c,v 1.9 2022/09/23 12:15:34 christos Exp $	*/

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

/* ! \file */

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/heap.h>
#include <isc/mem.h>
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

struct e {
	unsigned int value;
	unsigned int index;
};

static bool
compare(void *p1, void *p2) {
	struct e *e1 = p1;
	struct e *e2 = p2;

	return (e1->value < e2->value);
}

static void
idx(void *p, unsigned int i) {
	struct e *e = p;

	e->index = i;
}

/* test isc_heap_delete() */
static void
isc_heap_delete_test(void **state) {
	isc_heap_t *heap = NULL;
	struct e e1 = { 100, 0 };

	UNUSED(state);

	isc_heap_create(test_mctx, compare, idx, 0, &heap);
	assert_non_null(heap);

	isc_heap_insert(heap, &e1);
	assert_int_equal(e1.index, 1);

	isc_heap_delete(heap, e1.index);
	assert_int_equal(e1.index, 0);

	isc_heap_destroy(&heap);
	assert_null(heap);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_heap_delete_test),
	};

	return (cmocka_run_group_tests(tests, _setup, _teardown));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
