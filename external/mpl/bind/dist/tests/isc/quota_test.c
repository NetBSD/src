/*	$NetBSD: quota_test.c,v 1.3 2025/01/26 16:25:50 christos Exp $	*/

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
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/job.h>
#include <isc/quota.h>
#include <isc/result.h>
#include <isc/thread.h>
#include <isc/util.h>
#include <isc/uv.h>

#include <tests/isc.h>

isc_quota_t quota;

ISC_RUN_TEST_IMPL(isc_quota_get_set) {
	UNUSED(state);
	isc_quota_init(&quota, 100);

	assert_int_equal(isc_quota_getmax(&quota), 100);
	assert_int_equal(isc_quota_getsoft(&quota), 0);

	isc_quota_max(&quota, 50);
	isc_quota_soft(&quota, 30);

	assert_int_equal(isc_quota_getmax(&quota), 50);
	assert_int_equal(isc_quota_getsoft(&quota), 30);

	assert_int_equal(isc_quota_getused(&quota), 0);
	isc_quota_acquire(&quota);
	assert_int_equal(isc_quota_getused(&quota), 1);
	isc_quota_release(&quota);
	assert_int_equal(isc_quota_getused(&quota), 0);

	/* Unlimited */
	isc_quota_max(&quota, 0);
	isc_quota_soft(&quota, 0);

	isc_quota_destroy(&quota);
}

static void
add_quota(isc_quota_t *source, isc_result_t expected_result,
	  int expected_used) {
	isc_result_t result;

	result = isc_quota_acquire(source);
	assert_int_equal(result, expected_result);

	switch (expected_result) {
	case ISC_R_SUCCESS:
	case ISC_R_SOFTQUOTA:
		break;
	default:
		break;
	}

	assert_int_equal(isc_quota_getused(source), expected_used);
}

ISC_RUN_TEST_IMPL(isc_quota_hard) {
	int i;
	UNUSED(state);

	isc_quota_init(&quota, 100);

	for (i = 0; i < 100; i++) {
		add_quota(&quota, ISC_R_SUCCESS, i + 1);
	}

	add_quota(&quota, ISC_R_QUOTA, 100);

	assert_int_equal(isc_quota_getused(&quota), 100);

	isc_quota_release(&quota);

	add_quota(&quota, ISC_R_SUCCESS, 100);
	add_quota(&quota, ISC_R_QUOTA, 100);

	for (i = 100; i > 0; i--) {
		isc_quota_release(&quota);
		assert_int_equal(isc_quota_getused(&quota), i - 1);
	}
	assert_int_equal(isc_quota_getused(&quota), 0);
	isc_quota_destroy(&quota);
}

ISC_RUN_TEST_IMPL(isc_quota_soft) {
	int i;
	UNUSED(state);

	isc_quota_init(&quota, 100);
	isc_quota_soft(&quota, 50);

	for (i = 0; i < 50; i++) {
		add_quota(&quota, ISC_R_SUCCESS, i + 1);
	}
	for (i = 50; i < 100; i++) {
		add_quota(&quota, ISC_R_SOFTQUOTA, i + 1);
	}

	add_quota(&quota, ISC_R_QUOTA, 100);

	for (i = 99; i >= 0; i--) {
		isc_quota_release(&quota);
		assert_int_equal(isc_quota_getused(&quota), i);
	}
	assert_int_equal(isc_quota_getused(&quota), 0);
	isc_quota_destroy(&quota);
}

static atomic_uint_fast32_t cb_calls = 0;
static isc_job_t cbs[30];

static void
callback(void *data) {
	int val = *(int *)data;
	/* Callback is not called if we get the quota directly */
	assert_int_not_equal(val, -1);

	/* Verify that the callbacks are called in order */
	int v = atomic_fetch_add_relaxed(&cb_calls, 1);
	assert_int_equal(v, val);

	/*
	 * First 5 will be detached by the test function,
	 * for the last 5 - do a 'chain detach'.
	 */
	if (v >= 5) {
		isc_quota_release(&quota);
	}
}

ISC_RUN_TEST_IMPL(isc_quota_callback) {
	isc_result_t result;
	/*
	 * - 10 calls that end with SUCCESS
	 * - 10 calls that end with SOFTQUOTA
	 * - 10 callbacks
	 */
	int ints[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		       0,  1,  2,  3,  4,  5,  6,  7,  8,  9 };
	int i;
	UNUSED(state);

	isc_quota_init(&quota, 20);
	isc_quota_soft(&quota, 10);

	for (i = 0; i < 10; i++) {
		cbs[i] = (isc_job_t)ISC_JOB_INITIALIZER;
		result = isc_quota_acquire_cb(&quota, &cbs[i], callback,
					      &ints[i]);
		assert_int_equal(result, ISC_R_SUCCESS);
		assert_int_equal(isc_quota_getused(&quota), i + 1);
	}
	for (i = 10; i < 20; i++) {
		cbs[i] = (isc_job_t)ISC_JOB_INITIALIZER;
		result = isc_quota_acquire_cb(&quota, &cbs[i], callback,
					      &ints[i]);
		assert_int_equal(result, ISC_R_SOFTQUOTA);
		assert_int_equal(isc_quota_getused(&quota), i + 1);
	}

	for (i = 20; i < 30; i++) {
		cbs[i] = (isc_job_t)ISC_JOB_INITIALIZER;
		result = isc_quota_acquire_cb(&quota, &cbs[i], callback,
					      &ints[i]);
		assert_int_equal(result, ISC_R_QUOTA);
		assert_int_equal(isc_quota_getused(&quota), 20);
	}
	assert_int_equal(atomic_load(&cb_calls), 0);

	for (i = 0; i < 5; i++) {
		isc_quota_release(&quota);
		assert_int_equal(isc_quota_getused(&quota), 20);
		assert_int_equal(atomic_load(&cb_calls), i + 1);
	}
	/* That should cause a chain reaction */
	isc_quota_release(&quota);
	assert_int_equal(atomic_load(&cb_calls), 10);

	/* Release the quotas that we did not released in the callback */
	for (i = 0; i < 5; i++) {
		isc_quota_release(&quota);
	}

	for (i = 6; i < 20; i++) {
		isc_quota_release(&quota);
		assert_int_equal(isc_quota_getused(&quota), 19 - i);
	}
	assert_int_equal(atomic_load(&cb_calls), 10);

	assert_int_equal(isc_quota_getused(&quota), 0);
	isc_quota_destroy(&quota);
}

/*
 * Multithreaded quota callback test:
 * - quota set to 100
 * - 10 threads, each trying to get 100 quotas.
 * - creates a separate thread to release it after 10ms
 */

typedef struct qthreadinfo {
	atomic_uint_fast32_t direct;
	atomic_uint_fast32_t callback;
	isc_job_t callbacks[100];
} qthreadinfo_t;

static atomic_uint_fast32_t g_tnum = 0;
/* at most 10 * 100 quota_detach threads */
isc_thread_t g_threads[10 * 100];

static void *
quota_release(void *arg) {
	uv_sleep(10);
	isc_quota_release((isc_quota_t *)arg);
	return NULL;
}

static void
quota_callback(void *data) {
	qthreadinfo_t *qti = (qthreadinfo_t *)data;
	atomic_fetch_add_relaxed(&qti->callback, 1);
	int tnum = atomic_fetch_add_relaxed(&g_tnum, 1);
	isc_thread_create(quota_release, &quota, &g_threads[tnum]);
}

static void *
quota_thread(void *qtip) {
	qthreadinfo_t *qti = (qthreadinfo_t *)qtip;
	for (int i = 0; i < 100; i++) {
		qti->callbacks[i] = (isc_job_t)ISC_JOB_INITIALIZER;
		isc_result_t result = isc_quota_acquire_cb(
			&quota, &qti->callbacks[i], quota_callback, qti);
		if (result == ISC_R_SUCCESS) {
			atomic_fetch_add_relaxed(&qti->direct, 1);
			int tnum = atomic_fetch_add_relaxed(&g_tnum, 1);
			isc_thread_create(quota_release, &quota,
					  &g_threads[tnum]);
		}
	}
	return NULL;
}

ISC_RUN_TEST_IMPL(isc_quota_callback_mt) {
	UNUSED(state);
	int i;

	isc_quota_init(&quota, 100);
	static qthreadinfo_t qtis[10];
	isc_thread_t threads[10];
	for (i = 0; i < 10; i++) {
		atomic_init(&qtis[i].direct, 0);
		atomic_init(&qtis[i].callback, 0);
		isc_thread_create(quota_thread, &qtis[i], &threads[i]);
	}
	for (i = 0; i < 10; i++) {
		isc_thread_join(threads[i], NULL);
	}

	for (i = 0; i < (int)atomic_load(&g_tnum); i++) {
		isc_thread_join(g_threads[i], NULL);
	}
	int direct = 0, ncallback = 0;

	for (i = 0; i < 10; i++) {
		direct += atomic_load(&qtis[i].direct);
		ncallback += atomic_load(&qtis[i].callback);
	}
	/* Total quota gained must be 10 threads * 100 tries */
	assert_int_equal(direct + ncallback, 10 * 100);
	/*
	 * At least 100 must be direct, the rest is virtually random:
	 * - in a regular run I'm constantly getting 100:900 ratio
	 * - under rr - usually around ~120:880
	 * - under rr -h - 1000:0
	 */
	assert_true(direct >= 100);

	isc_quota_destroy(&quota);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY(isc_quota_get_set)
ISC_TEST_ENTRY(isc_quota_hard)
ISC_TEST_ENTRY(isc_quota_soft)
ISC_TEST_ENTRY(isc_quota_callback)
ISC_TEST_ENTRY(isc_quota_callback_mt)

ISC_TEST_LIST_END

ISC_TEST_MAIN
