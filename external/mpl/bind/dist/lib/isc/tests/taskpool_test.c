/*	$NetBSD: taskpool_test.c,v 1.10 2022/09/23 12:15:34 christos Exp $	*/

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

#include <isc/task.h>
#include <isc/taskpool.h>
#include <isc/util.h>

#include "isctest.h"

#define TASK_MAGIC    ISC_MAGIC('T', 'A', 'S', 'K')
#define VALID_TASK(t) ISC_MAGIC_VALID(t, TASK_MAGIC)

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

/* Create a taskpool */
static void
create_pool(void **state) {
	isc_result_t result;
	isc_taskpool_t *pool = NULL;

	UNUSED(state);

	result = isc_taskpool_create(taskmgr, test_mctx, 8, 2, false, &pool);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_taskpool_size(pool), 8);

	isc_taskpool_destroy(&pool);
	assert_null(pool);
}

/* Resize a taskpool */
static void
expand_pool(void **state) {
	isc_result_t result;
	isc_taskpool_t *pool1 = NULL, *pool2 = NULL, *hold = NULL;

	UNUSED(state);

	result = isc_taskpool_create(taskmgr, test_mctx, 10, 2, false, &pool1);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_taskpool_size(pool1), 10);

	/* resizing to a smaller size should have no effect */
	hold = pool1;
	result = isc_taskpool_expand(&pool1, 5, false, &pool2);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_taskpool_size(pool2), 10);
	assert_ptr_equal(pool2, hold);
	assert_null(pool1);
	pool1 = pool2;
	pool2 = NULL;

	/* resizing to the same size should have no effect */
	hold = pool1;
	result = isc_taskpool_expand(&pool1, 10, false, &pool2);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_taskpool_size(pool2), 10);
	assert_ptr_equal(pool2, hold);
	assert_null(pool1);
	pool1 = pool2;
	pool2 = NULL;

	/* resizing to larger size should make a new pool */
	hold = pool1;
	result = isc_taskpool_expand(&pool1, 20, false, &pool2);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_taskpool_size(pool2), 20);
	assert_ptr_not_equal(pool2, hold);
	assert_null(pool1);

	isc_taskpool_destroy(&pool2);
	assert_null(pool2);
}

/* Get tasks */
static void
get_tasks(void **state) {
	isc_result_t result;
	isc_taskpool_t *pool = NULL;
	isc_task_t *task1 = NULL, *task2 = NULL, *task3 = NULL;

	UNUSED(state);

	result = isc_taskpool_create(taskmgr, test_mctx, 2, 2, false, &pool);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_taskpool_size(pool), 2);

	/* two tasks in pool; make sure we can access them more than twice */
	isc_taskpool_gettask(pool, &task1);
	assert_true(VALID_TASK(task1));

	isc_taskpool_gettask(pool, &task2);
	assert_true(VALID_TASK(task2));

	isc_taskpool_gettask(pool, &task3);
	assert_true(VALID_TASK(task3));

	isc_task_destroy(&task1);
	isc_task_destroy(&task2);
	isc_task_destroy(&task3);

	isc_taskpool_destroy(&pool);
	assert_null(pool);
}

/* Set privileges */
static void
set_privilege(void **state) {
	isc_result_t result;
	isc_taskpool_t *pool = NULL;
	isc_task_t *task1 = NULL, *task2 = NULL, *task3 = NULL;

	UNUSED(state);

	result = isc_taskpool_create(taskmgr, test_mctx, 2, 2, true, &pool);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_int_equal(isc_taskpool_size(pool), 2);

	isc_taskpool_gettask(pool, &task1);
	isc_taskpool_gettask(pool, &task2);
	isc_taskpool_gettask(pool, &task3);

	assert_true(VALID_TASK(task1));
	assert_true(VALID_TASK(task2));
	assert_true(VALID_TASK(task3));

	assert_true(isc_task_getprivilege(task1));
	assert_true(isc_task_getprivilege(task2));
	assert_true(isc_task_getprivilege(task3));

	isc_task_destroy(&task1);
	isc_task_destroy(&task2);
	isc_task_destroy(&task3);

	isc_taskpool_destroy(&pool);
	assert_null(pool);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(create_pool, _setup, _teardown),
		cmocka_unit_test_setup_teardown(expand_pool, _setup, _teardown),
		cmocka_unit_test_setup_teardown(get_tasks, _setup, _teardown),
		cmocka_unit_test_setup_teardown(set_privilege, _setup,
						_teardown),
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
