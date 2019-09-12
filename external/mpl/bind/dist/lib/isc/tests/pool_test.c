/*	$NetBSD: pool_test.c,v 1.4.4.1 2019/09/12 19:18:16 martin Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/mem.h>
#include <isc/pool.h>
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

static isc_result_t
poolinit(void **target, void *arg) {
	isc_result_t result;

	isc_taskmgr_t *mgr = (isc_taskmgr_t *) arg;
	isc_task_t *task = NULL;
	result = isc_task_create(mgr, 0, &task);
	if (result != ISC_R_SUCCESS)
		return (result);

	*target = (void *) task;
	return (ISC_R_SUCCESS);
}

static void
poolfree(void **target) {
	isc_task_t *task = *(isc_task_t **) target;
	isc_task_destroy(&task);
	*target = NULL;
}

/* Create a pool */
static void
create_pool(void **state) {
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
static void
expand_pool(void **state) {
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
static void
get_objects(void **state) {
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
	isc_task_attach((isc_task_t *) item, &task1);

	item = isc_pool_get(pool);
	assert_non_null(item);
	isc_task_attach((isc_task_t *) item, &task2);

	item = isc_pool_get(pool);
	assert_non_null(item);
	isc_task_attach((isc_task_t *) item, &task3);

	isc_task_detach(&task1);
	isc_task_detach(&task2);
	isc_task_detach(&task3);

	isc_pool_destroy(&pool);
	assert_null(pool);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(create_pool,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(expand_pool,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(get_objects,
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
