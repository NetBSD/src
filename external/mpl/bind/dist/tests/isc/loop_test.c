/*	$NetBSD: loop_test.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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
#include <isc/util.h>

#include "loop.c"

#include <tests/isc.h>

static atomic_uint scheduled = 0;

static void
count(void *arg) {
	UNUSED(arg);

	atomic_fetch_add(&scheduled, 1);
}

static void
shutdown_loopmgr(void *arg) {
	UNUSED(arg);

	while (atomic_load(&scheduled) != loopmgr->nloops) {
		isc_thread_yield();
	}

	isc_loopmgr_shutdown(loopmgr);
}

ISC_RUN_TEST_IMPL(isc_loopmgr) {
	atomic_store(&scheduled, 0);

	isc_loopmgr_setup(loopmgr, count, loopmgr);
	isc_loop_setup(mainloop, shutdown_loopmgr, loopmgr);

	isc_loopmgr_run(loopmgr);

	assert_int_equal(atomic_load(&scheduled), loopmgr->nloops);
}

static void
runjob(void *arg ISC_ATTR_UNUSED) {
	isc_async_current(count, loopmgr);
	if (isc_tid() == 0) {
		isc_async_current(shutdown_loopmgr, loopmgr);
	}
}

ISC_RUN_TEST_IMPL(isc_loopmgr_runjob) {
	atomic_store(&scheduled, 0);

	isc_loopmgr_setup(loopmgr, runjob, loopmgr);
	isc_loopmgr_run(loopmgr);
	assert_int_equal(atomic_load(&scheduled), loopmgr->nloops);
}

static void
pause_loopmgr(void *arg) {
	UNUSED(arg);

	isc_loopmgr_pause(loopmgr);

	assert_true(atomic_load(&loopmgr->paused));

	for (size_t i = 0; i < loopmgr->nloops; i++) {
		isc_loop_t *loop = &loopmgr->loops[i];

		assert_true(loop->paused);
	}

	atomic_init(&scheduled, loopmgr->nloops);

	isc_loopmgr_resume(loopmgr);
}

ISC_RUN_TEST_IMPL(isc_loopmgr_pause) {
	isc_loop_setup(mainloop, pause_loopmgr, loopmgr);
	isc_loop_setup(mainloop, shutdown_loopmgr, loopmgr);
	isc_loopmgr_run(loopmgr);
}

static void
send_sigint(void *arg) {
	UNUSED(arg);

	kill(getpid(), SIGINT);
}

ISC_RUN_TEST_IMPL(isc_loopmgr_sigint) {
	isc_loop_setup(mainloop, send_sigint, loopmgr);
	isc_loopmgr_run(loopmgr);
}

static void
send_sigterm(void *arg) {
	UNUSED(arg);

	kill(getpid(), SIGINT);
}

ISC_RUN_TEST_IMPL(isc_loopmgr_sigterm) {
	isc_loop_setup(mainloop, send_sigterm, loopmgr);
	isc_loopmgr_run(loopmgr);
}

ISC_TEST_LIST_START
ISC_TEST_ENTRY_CUSTOM(isc_loopmgr, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(isc_loopmgr_pause, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(isc_loopmgr_runjob, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(isc_loopmgr_sigint, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(isc_loopmgr_sigterm, setup_loopmgr, teardown_loopmgr)
ISC_TEST_LIST_END

ISC_TEST_MAIN
