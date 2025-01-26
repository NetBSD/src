/*	$NetBSD: ratelimiter_test.c,v 1.1.1.1 2025/01/26 16:12:36 christos Exp $	*/

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

#include <isc/job.h>
#include <isc/loop.h>
#include <isc/ratelimiter.h>
#include <isc/time.h>

#include "ratelimiter.c"

#include <tests/isc.h>

isc_ratelimiter_t *rl = NULL;

typedef struct rlstat {
	isc_rlevent_t *event;
} rlstat_t;

ISC_LOOP_TEST_IMPL(ratelimiter_create) {
	assert_null(rl);
	expect_assert_failure(isc_ratelimiter_create(NULL, &rl));
	expect_assert_failure(isc_ratelimiter_create(mainloop, NULL));
	assert_null(rl);

	isc_ratelimiter_create(mainloop, &rl);
	isc_ratelimiter_shutdown(rl);
	isc_ratelimiter_detach(&rl);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(ratelimiter_shutdown) {
	assert_null(rl);
	expect_assert_failure(isc_ratelimiter_shutdown(NULL));
	expect_assert_failure(isc_ratelimiter_shutdown(rl));

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_TEST_IMPL(ratelimiter_detach) {
	assert_null(rl);

	expect_assert_failure(isc_ratelimiter_detach(NULL));
	expect_assert_failure(isc_ratelimiter_detach(&rl));

	isc_loopmgr_shutdown(loopmgr);
}

static int ticks = 0;
static isc_time_t start_time;
static isc_time_t tick_time;

static void
tick(void *arg) {
	rlstat_t *rlstat = (rlstat_t *)arg;

	isc_rlevent_free(&rlstat->event);
	isc_mem_put(mctx, rlstat, sizeof(*rlstat));

	ticks++;

	tick_time = isc_time_now();

	isc_ratelimiter_shutdown(rl);
	isc_ratelimiter_detach(&rl);

	isc_loopmgr_shutdown(loopmgr);
}

ISC_LOOP_SETUP_IMPL(ratelimiter_common) {
	assert_null(rl);
	isc_time_set(&tick_time, 0, 0);
	start_time = isc_time_now();
	isc_ratelimiter_create(mainloop, &rl);
}

ISC_LOOP_SETUP_IMPL(ratelimiter_enqueue) {
	ticks = 0;
	setup_loop_ratelimiter_common(arg);
}

ISC_LOOP_TEARDOWN_IMPL(ratelimiter_enqueue) { assert_int_equal(ticks, 1); }

ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(ratelimiter_enqueue) {
	isc_result_t result;
	rlstat_t *rlstat = isc_mem_get(mctx, sizeof(*rlstat));
	*rlstat = (rlstat_t){ 0 };

	result = isc_ratelimiter_enqueue(rl, mainloop, tick, rlstat,
					 &rlstat->event);
	assert_int_equal(result, ISC_R_SUCCESS);
	assert_non_null(rlstat->event);
}

ISC_LOOP_SETUP_IMPL(ratelimiter_enqueue_shutdown) {
	ticks = 0;
	setup_loop_ratelimiter_common(arg);
}

ISC_LOOP_TEARDOWN_IMPL(ratelimiter_enqueue_shutdown) {
	assert_int_equal(ticks, 1);
}

ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(ratelimiter_enqueue_shutdown) {
	isc_rlevent_t *event = NULL;
	rlstat_t *rlstat = isc_mem_get(mctx, sizeof(*rlstat));
	*rlstat = (rlstat_t){ 0 };

	expect_assert_failure(
		isc_ratelimiter_enqueue(NULL, mainloop, tick, NULL, &event));
	expect_assert_failure(
		isc_ratelimiter_enqueue(rl, NULL, tick, NULL, &event));
	expect_assert_failure(
		isc_ratelimiter_enqueue(rl, mainloop, tick, NULL, NULL));

	assert_int_equal(isc_ratelimiter_enqueue(rl, mainloop, tick, rlstat,
						 &rlstat->event),
			 ISC_R_SUCCESS);
	assert_non_null(rlstat->event);

	isc_ratelimiter_shutdown(rl);

	assert_int_equal(
		isc_ratelimiter_enqueue(rl, mainloop, tick, NULL, &event),
		ISC_R_SHUTTINGDOWN);
	assert_null(event);
}

ISC_LOOP_SETUP_IMPL(ratelimiter_dequeue) {
	ticks = 0;
	setup_loop_ratelimiter_common(arg);
}

ISC_LOOP_TEARDOWN_IMPL(ratelimiter_dequeue) { /* */
	assert_int_equal(ticks, 0);
}

ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(ratelimiter_dequeue) {
	isc_rlevent_t *fake = isc_mem_get(mctx, sizeof(*fake));
	rlstat_t *rlstat = isc_mem_get(mctx, sizeof(*rlstat));
	*rlstat = (rlstat_t){ 0 };

	assert_int_equal(isc_ratelimiter_enqueue(rl, mainloop, tick, rlstat,
						 &rlstat->event),
			 ISC_R_SUCCESS);
	assert_int_equal(isc_ratelimiter_dequeue(rl, &rlstat->event),
			 ISC_R_SUCCESS);
	isc_mem_put(mctx, rlstat, sizeof(*rlstat));

	/* Set up a mock ratelimiter event that isn't actually scheduled */
	*fake = (isc_rlevent_t){ .link = ISC_LINK_INITIALIZER };
	isc_loop_attach(mainloop, &fake->loop);
	isc_ratelimiter_attach(rl, &fake->rl);
	assert_int_equal(isc_ratelimiter_dequeue(rl, &fake), ISC_R_NOTFOUND);
	isc_loop_detach(&fake->loop);
	isc_ratelimiter_detach(&fake->rl);
	isc_mem_put(mctx, fake, sizeof(*fake));

	isc_ratelimiter_shutdown(rl);
	isc_ratelimiter_detach(&rl);

	isc_loopmgr_shutdown(loopmgr);
}

static isc_time_t tock_time;

static void
tock(void *arg) {
	rlstat_t *rlstat = (rlstat_t *)arg;

	isc_rlevent_free(&rlstat->event);
	isc_mem_put(mctx, rlstat, sizeof(*rlstat));

	ticks++;
	tock_time = isc_time_now();
}

ISC_LOOP_SETUP_IMPL(ratelimiter_pertick_interval) {
	ticks = 0;
	isc_time_set(&tock_time, 0, 0);
	setup_loop_ratelimiter_common(arg);
}

ISC_LOOP_TEARDOWN_IMPL(ratelimiter_pertick_interval) {
	uint64_t t = isc_time_microdiff(&tick_time, &tock_time);
	assert_int_equal(ticks, 2);
	assert_true(t >= 1000000);

	t = isc_time_microdiff(&tock_time, &start_time);
	assert_true(t < 1000000);
}

ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(ratelimiter_pertick_interval) {
	rlstat_t *rlstat = NULL;
	isc_interval_t interval;

	isc_interval_set(&interval, 1, NS_PER_SEC / 10);

	expect_assert_failure(isc_ratelimiter_setinterval(NULL, &interval));
	expect_assert_failure(isc_ratelimiter_setinterval(rl, NULL));
	expect_assert_failure(isc_ratelimiter_setpertic(NULL, 1));
	expect_assert_failure(isc_ratelimiter_setpertic(rl, 0));
	expect_assert_failure(isc_ratelimiter_setpushpop(NULL, false));

	isc_ratelimiter_setinterval(rl, &interval);
	isc_ratelimiter_setpertic(rl, 1);
	isc_ratelimiter_setpushpop(rl, false);

	rlstat = isc_mem_get(mctx, sizeof(*rlstat));
	*rlstat = (rlstat_t){ 0 };
	assert_int_equal(isc_ratelimiter_enqueue(rl, mainloop, tock, rlstat,
						 &rlstat->event),
			 ISC_R_SUCCESS);

	rlstat = isc_mem_get(mctx, sizeof(*rlstat));
	*rlstat = (rlstat_t){ 0 };
	assert_int_equal(isc_ratelimiter_enqueue(rl, mainloop, tick, rlstat,
						 &rlstat->event),
			 ISC_R_SUCCESS);
}

ISC_LOOP_SETUP_IMPL(ratelimiter_pushpop) {
	ticks = 0;
	isc_time_set(&tock_time, 0, 0);
	setup_loop_ratelimiter_common(arg);
}

ISC_LOOP_TEARDOWN_IMPL(ratelimiter_pushpop) {
	uint64_t t = isc_time_microdiff(&tock_time, &tick_time);
	assert_int_equal(ticks, 2);
	assert_true(t < 1000000);
}

ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(ratelimiter_pushpop) {
	rlstat_t *rlstat = NULL;
	isc_interval_t interval;

	isc_interval_set(&interval, 1, NS_PER_SEC / 10);

	isc_ratelimiter_setinterval(rl, &interval);
	isc_ratelimiter_setpertic(rl, 2);
	isc_ratelimiter_setpushpop(rl, true);

	rlstat = isc_mem_get(mctx, sizeof(*rlstat));
	*rlstat = (rlstat_t){ 0 };
	assert_int_equal(isc_ratelimiter_enqueue(rl, mainloop, tock, rlstat,
						 &rlstat->event),
			 ISC_R_SUCCESS);

	rlstat = isc_mem_get(mctx, sizeof(*rlstat));
	*rlstat = (rlstat_t){ 0 };
	assert_int_equal(isc_ratelimiter_enqueue(rl, mainloop, tick, rlstat,
						 &rlstat->event),
			 ISC_R_SUCCESS);
}

static int
setup_test(void **state) {
	int r;

	r = setup_loopmgr(state);
	if (r != 0) {
		return r;
	}

	return 0;
}

static int
teardown_test(void **state) {
	int r;

	r = teardown_loopmgr(state);
	if (r != 0) {
		return r;
	}

	return 0;
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(ratelimiter_create, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ratelimiter_shutdown, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ratelimiter_detach, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ratelimiter_enqueue, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ratelimiter_enqueue_shutdown, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ratelimiter_dequeue, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ratelimiter_pertick_interval, setup_test, teardown_test)
ISC_TEST_ENTRY_CUSTOM(ratelimiter_pushpop, setup_test, teardown_test)

ISC_TEST_LIST_END

ISC_TEST_MAIN
