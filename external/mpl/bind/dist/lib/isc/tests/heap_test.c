/*	$NetBSD: heap_test.c,v 1.4.2.3 2020/04/13 08:02:59 martin Exp $	*/

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

/* ! \file */

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

#include <isc/heap.h>
#include <isc/mem.h>
#include <isc/util.h>

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
	isc_mem_t *mctx = NULL;
	isc_heap_t *heap = NULL;
	isc_result_t result;
	struct e e1 = { 100, 0 };

	UNUSED(state);

	result = isc_mem_create(0, 0, &mctx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_heap_create(mctx, compare, idx, 0, &heap);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(heap);

	isc_heap_insert(heap, &e1);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(e1.index, 1);

	isc_heap_delete(heap, e1.index);
	assert_int_equal(e1.index, 0);

	isc_heap_destroy(&heap);
	assert_int_equal(heap, NULL);

	isc_mem_detach(&mctx);
	assert_int_equal(mctx, NULL);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_heap_delete_test),
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
