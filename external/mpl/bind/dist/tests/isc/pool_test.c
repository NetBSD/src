/*	$NetBSD: pool_test.c,v 1.2.2.2 2024/02/25 15:47:52 martin Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/mem.h>
#include <isc/pool.h>
#include <isc/util.h>

#include <tests/isc.h>

static isc_result_t
poolinit(void **target, void *arg) {
	isc_result_t result;

	isc_taskmgr_t *mgr = (isc_taskmgr_t *)arg;
	isc_task_t *task = NULL;
	result = isc_task_create(mgr, 0, &task);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	*target = (void *)task;
	return (ISC_R_SUCCESS);
}

static void
poolfree(void **target) {
	isc_task_t *task = *(isc_task_t **)target;
	isc_task_destroy(&task);
	*target = NULL;
}

/* Create a pool */
ISC_RUN_TEST_IMPL(create_pool) {
	isc_result_t result;
	isc_pool_t *pool = NULL;

	UNUSED(state);

	result = isc_pool_create(mctx, 8, poolfree, poolinit, taskmgr, &pool);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_pool_count(pool), 8);

	isc_pool_destroy(&pool);
	assert_null(pool);
}

/* Resize a pool */
ISC_RUN_TEST_IMPL(expand_pool) {
	isc_result_t result;
	isc_pool_t *pool1 = NULL, *pool2 = NULL, *hold = NULL;

	UNUSED(state);

	result = isc_pool_create(mctx, 10, poolfree, poolinit, taskmgr, &pool1);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_pool_count(pool1), 10);

	/* resizing to a smaller size should have no effect */
	hold = pool1;
	result = isc_pool_expand(&pool1, 5, &pool2);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_pool_count(pool2), 10);
	assert_ptr_equal(pool2, hold);
	assert_null(pool1);
	pool1 = pool2;
	pool2 = NULL;

	/* resizing to the same size should have no effect */
	hold = pool1;
	result = isc_pool_expand(&pool1, 10, &pool2);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_pool_count(pool2), 10);
	assert_ptr_equal(pool2, hold);
	assert_null(pool1);
	pool1 = pool2;
	pool2 = NULL;

	/* resizing to larger size should make a new pool */
	hold = pool1;
	result = isc_pool_expand(&pool1, 20, &pool2);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_pool_count(pool2), 20);
	assert_ptr_not_equal(pool2, hold);
	assert_null(pool1);

	isc_pool_destroy(&pool2);
	assert_null(pool2);
}

/* Get objects */
ISC_RUN_TEST_IMPL(get_objects) {
	isc_result_t result;
	isc_pool_t *pool = NULL;
	void *item;
	isc_task_t *task1 = NULL, *task2 = NULL, *task3 = NULL;

	UNUSED(state);

	result = isc_pool_create(mctx, 2, poolfree, poolinit, taskmgr, &pool);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_pool_count(pool), 2);

	item = isc_pool_get(pool);
	assert_non_null(item);
	isc_task_attach((isc_task_t *)item, &task1);

	item = isc_pool_get(pool);
	assert_non_null(item);
	isc_task_attach((isc_task_t *)item, &task2);

	item = isc_pool_get(pool);
	assert_non_null(item);
	isc_task_attach((isc_task_t *)item, &task3);

	isc_task_detach(&task1);
	isc_task_detach(&task2);
	isc_task_detach(&task3);

	isc_pool_destroy(&pool);
	assert_null(pool);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(create_pool, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(expand_pool, setup_managers, teardown_managers)
ISC_TEST_ENTRY_CUSTOM(get_objects, setup_managers, teardown_managers)

ISC_TEST_LIST_END

ISC_TEST_MAIN
