/*	$NetBSD: heap_test.c,v 1.2.2.2 2024/02/25 15:47:51 martin Exp $	*/

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

#include <inttypes.h>
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

#include <tests/isc.h>

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
ISC_RUN_TEST_IMPL(isc_heap_delete) {
	isc_heap_t *heap = NULL;
	struct e e1 = { 100, 0 };

	UNUSED(state);

	isc_heap_create(mctx, compare, idx, 0, &heap);
	assert_non_null(heap);

	isc_heap_insert(heap, &e1);
	assert_int_equal(e1.index, 1);

	isc_heap_delete(heap, e1.index);
	assert_int_equal(e1.index, 0);

	isc_heap_destroy(&heap);
	assert_null(heap);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_heap_delete)

ISC_TEST_LIST_END

ISC_TEST_MAIN
