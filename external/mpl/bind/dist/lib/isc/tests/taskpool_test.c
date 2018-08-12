/*	$NetBSD: taskpool_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <unistd.h>

#include <isc/task.h>
#include <isc/taskpool.h>

#include "isctest.h"

/*
 * Individual unit tests
 */

/* Create a taskpool */
ATF_TC(create_pool);
ATF_TC_HEAD(create_pool, tc) {
	atf_tc_set_md_var(tc, "descr", "create a taskpool");
}
ATF_TC_BODY(create_pool, tc) {
	isc_result_t result;
	isc_taskpool_t *pool = NULL;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_taskpool_create(taskmgr, mctx, 8, 2, &pool);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_taskpool_size(pool), 8);

	isc_taskpool_destroy(&pool);
	ATF_REQUIRE_EQ(pool, NULL);

	isc_test_end();
}

/* Resize a taskpool */
ATF_TC(expand_pool);
ATF_TC_HEAD(expand_pool, tc) {
	atf_tc_set_md_var(tc, "descr", "expand a taskpool");
}
ATF_TC_BODY(expand_pool, tc) {
	isc_result_t result;
	isc_taskpool_t *pool1 = NULL, *pool2 = NULL, *hold = NULL;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_taskpool_create(taskmgr, mctx, 10, 2, &pool1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_taskpool_size(pool1), 10);

	/* resizing to a smaller size should have no effect */
	hold = pool1;
	result = isc_taskpool_expand(&pool1, 5, &pool2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_taskpool_size(pool2), 10);
	ATF_REQUIRE_EQ(pool2, hold);
	ATF_REQUIRE_EQ(pool1, NULL);
	pool1 = pool2;
	pool2 = NULL;

	/* resizing to the same size should have no effect */
	hold = pool1;
	result = isc_taskpool_expand(&pool1, 10, &pool2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_taskpool_size(pool2), 10);
	ATF_REQUIRE_EQ(pool2, hold);
	ATF_REQUIRE_EQ(pool1, NULL);
	pool1 = pool2;
	pool2 = NULL;

	/* resizing to larger size should make a new pool */
	hold = pool1;
	result = isc_taskpool_expand(&pool1, 20, &pool2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_taskpool_size(pool2), 20);
	ATF_REQUIRE(pool2 != hold);
	ATF_REQUIRE_EQ(pool1, NULL);

	isc_taskpool_destroy(&pool2);
	ATF_REQUIRE_EQ(pool2, NULL);

	isc_test_end();
}

/* Get tasks */
ATF_TC(get_tasks);
ATF_TC_HEAD(get_tasks, tc) {
	atf_tc_set_md_var(tc, "descr", "create a taskpool");
}
ATF_TC_BODY(get_tasks, tc) {
	isc_result_t result;
	isc_taskpool_t *pool = NULL;
	isc_task_t *task1 = NULL, *task2 = NULL, *task3 = NULL;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_taskpool_create(taskmgr, mctx, 2, 2, &pool);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_taskpool_size(pool), 2);

	/* two tasks in pool; make sure we can access them more than twice */
	isc_taskpool_gettask(pool, &task1);
	ATF_REQUIRE(ISCAPI_TASK_VALID(task1));

	isc_taskpool_gettask(pool, &task2);
	ATF_REQUIRE(ISCAPI_TASK_VALID(task2));

	isc_taskpool_gettask(pool, &task3);
	ATF_REQUIRE(ISCAPI_TASK_VALID(task3));

	isc_task_destroy(&task1);
	isc_task_destroy(&task2);
	isc_task_destroy(&task3);

	isc_taskpool_destroy(&pool);
	ATF_REQUIRE_EQ(pool, NULL);

	isc_test_end();
}

/* Get tasks */
ATF_TC(set_privilege);
ATF_TC_HEAD(set_privilege, tc) {
	atf_tc_set_md_var(tc, "descr", "create a taskpool");
}
ATF_TC_BODY(set_privilege, tc) {
	isc_result_t result;
	isc_taskpool_t *pool = NULL;
	isc_task_t *task1 = NULL, *task2 = NULL, *task3 = NULL;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_taskpool_create(taskmgr, mctx, 2, 2, &pool);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	ATF_REQUIRE_EQ(isc_taskpool_size(pool), 2);

	isc_taskpool_setprivilege(pool, ISC_TRUE);

	isc_taskpool_gettask(pool, &task1);
	isc_taskpool_gettask(pool, &task2);
	isc_taskpool_gettask(pool, &task3);

	ATF_CHECK(ISCAPI_TASK_VALID(task1));
	ATF_CHECK(ISCAPI_TASK_VALID(task2));
	ATF_CHECK(ISCAPI_TASK_VALID(task3));

	ATF_CHECK(isc_task_privilege(task1));
	ATF_CHECK(isc_task_privilege(task2));
	ATF_CHECK(isc_task_privilege(task3));

	isc_taskpool_setprivilege(pool, ISC_FALSE);

	ATF_CHECK(!isc_task_privilege(task1));
	ATF_CHECK(!isc_task_privilege(task2));
	ATF_CHECK(!isc_task_privilege(task3));

	isc_task_destroy(&task1);
	isc_task_destroy(&task2);
	isc_task_destroy(&task3);

	isc_taskpool_destroy(&pool);
	ATF_REQUIRE_EQ(pool, NULL);

	isc_test_end();
}

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, create_pool);
	ATF_TP_ADD_TC(tp, expand_pool);
	ATF_TP_ADD_TC(tp, get_tasks);
	ATF_TP_ADD_TC(tp, set_privilege);

	return (atf_no_error());
}

