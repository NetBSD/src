/*	$NetBSD: work_test.c,v 1.2 2025/01/26 16:25:50 christos Exp $	*/

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
#include <isc/loop.h>
#include <isc/os.h>
#include <isc/result.h>
#include <isc/tid.h>
#include <isc/util.h>
#include <isc/work.h>

#include "work.c"

#include <tests/isc.h>

static atomic_uint scheduled = 0;

static void
work_cb(void *arg) {
	UNUSED(arg);

	atomic_fetch_add(&scheduled, 1);

	assert_int_equal(isc_tid(), UINT32_MAX);
}

static void
after_work_cb(void *arg) {
	UNUSED(arg);

	assert_int_equal(atomic_load(&scheduled), 1);
	isc_loopmgr_shutdown(loopmgr);
}

static void
work_enqueue_cb(void *arg) {
	UNUSED(arg);
	uint32_t tid = isc_loopmgr_nloops(loopmgr) - 1;

	isc_loop_t *loop = isc_loop_get(loopmgr, tid);

	isc_work_enqueue(loop, work_cb, after_work_cb, loopmgr);
}

ISC_RUN_TEST_IMPL(isc_work_enqueue) {
	atomic_init(&scheduled, 0);

	isc_loop_setup(isc_loop_main(loopmgr), work_enqueue_cb, loopmgr);

	isc_loopmgr_run(loopmgr);

	assert_int_equal(atomic_load(&scheduled), 1);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(isc_work_enqueue, setup_loopmgr, teardown_loopmgr)
ISC_TEST_LIST_END

ISC_TEST_MAIN
