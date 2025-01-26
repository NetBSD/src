/*	$NetBSD: job_test.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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
#include <sys/types.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/atomic.h>
#include <isc/job.h>
#include <isc/loop.h>
#include <isc/os.h>
#include <isc/result.h>
#include <isc/util.h>

#include <tests/isc.h>

static atomic_uint scheduled;
static atomic_uint executed;

#define MAX_EXECUTED 1000000

struct test_arg {
	isc_job_t job;
	union {
		int n;
		void *ptr;
	} arg;
};

static void
shutdown_cb(void *arg) {
	struct test_arg *ta = arg;

	isc_mem_put(mctx, ta, sizeof(*ta));

	isc_loopmgr_shutdown(loopmgr);
}

static void
job_cb(void *arg) {
	struct test_arg *ta = arg;
	unsigned int n = atomic_fetch_add(&executed, 1);

	if (n <= MAX_EXECUTED) {
		atomic_fetch_add(&scheduled, 1);
		isc_job_run(isc_loop(), &ta->job, job_cb, ta);
	} else {
		isc_job_run(isc_loop(), &ta->job, shutdown_cb, ta);
	}
}

static void
job_run_cb(void *arg) {
	struct test_arg *ta = arg;
	atomic_fetch_add(&scheduled, 1);

	if (arg == NULL) {
		ta = isc_mem_get(mctx, sizeof(*ta));
		*ta = (struct test_arg){ .job = ISC_JOB_INITIALIZER };
	}

	isc_job_run(isc_loop(), &ta->job, job_cb, ta);
}

ISC_RUN_TEST_IMPL(isc_job_run) {
	atomic_init(&scheduled, 0);
	atomic_init(&executed, 0);

	isc_loopmgr_setup(loopmgr, job_run_cb, NULL);

	isc_loopmgr_run(loopmgr);

	assert_int_equal(atomic_load(&scheduled), atomic_load(&executed));
}

static char string[32] = "";
struct test_arg n1 = { .job = ISC_JOB_INITIALIZER, .arg.n = 1 };
struct test_arg n2 = { .job = ISC_JOB_INITIALIZER, .arg.n = 2 };
struct test_arg n3 = { .job = ISC_JOB_INITIALIZER, .arg.n = 3 };
struct test_arg n4 = { .job = ISC_JOB_INITIALIZER, .arg.n = 4 };
struct test_arg n5 = { .job = ISC_JOB_INITIALIZER, .arg.n = 5 };

static void
append(void *arg) {
	struct test_arg *ta = arg;

	char value[32];
	sprintf(value, "%d", ta->arg.n);
	strlcat(string, value, 10);
}

static void
job_multiple(void *arg) {
	UNUSED(arg);

	/* These will be processed in normal order */
	isc_job_run(mainloop, &n1.job, append, &n1);
	isc_job_run(mainloop, &n2.job, append, &n2);
	isc_job_run(mainloop, &n3.job, append, &n3);
	isc_job_run(mainloop, &n4.job, append, &n4);
	isc_job_run(mainloop, &n5.job, append, &n5);
	isc_loopmgr_shutdown(loopmgr);
}

ISC_RUN_TEST_IMPL(isc_job_multiple) {
	string[0] = '\0';
	isc_loop_setup(isc_loop_main(loopmgr), job_multiple, loopmgr);
	isc_loopmgr_run(loopmgr);
	assert_string_equal(string, "12345");
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(isc_job_run, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(isc_job_multiple, setup_loopmgr, teardown_loopmgr)
ISC_TEST_LIST_END

ISC_TEST_MAIN
