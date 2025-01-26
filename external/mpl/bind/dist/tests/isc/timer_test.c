/*	$NetBSD: timer_test.c,v 1.3 2025/01/26 16:25:50 christos Exp $	*/

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

#include <isc/atomic.h>
#include <isc/commandline.h>
#include <isc/condition.h>
#include <isc/job.h>
#include <isc/loop.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include "timer.c"

#include <tests/isc.h>

/* Set to true (or use -v option) for verbose output */
static bool verbose = true;

#define FUDGE_SECONDS	  0	    /* in absence of clock_getres() */
#define FUDGE_NANOSECONDS 500000000 /* in absence of clock_getres() */

static isc_timer_t *timer = NULL;
static isc_time_t endtime;
static isc_mutex_t lasttime_mx;
static isc_time_t lasttime;
static int seconds;
static int nanoseconds;
static atomic_int_fast32_t eventcnt;
static atomic_uint_fast32_t errcnt;
static int nevents;

typedef struct setup_test_arg {
	isc_timertype_t timertype;
	isc_interval_t *interval;
	isc_job_cb action;
} setup_test_arg_t;

static void
setup_test_run(void *data) {
	isc_timertype_t timertype = ((setup_test_arg_t *)data)->timertype;
	isc_interval_t *interval = ((setup_test_arg_t *)data)->interval;
	isc_job_cb action = ((setup_test_arg_t *)data)->action;

	isc_mutex_lock(&lasttime_mx);
	lasttime = isc_time_now();
	UNLOCK(&lasttime_mx);

	isc_timer_create(mainloop, action, (void *)timertype, &timer);
	isc_timer_start(timer, timertype, interval);
}

static void
setup_test(isc_timertype_t timertype, isc_interval_t *interval,
	   isc_job_cb action) {
	setup_test_arg_t arg = { .timertype = timertype,
				 .interval = interval,
				 .action = action };

	isc_time_settoepoch(&endtime);
	atomic_init(&eventcnt, 0);

	isc_mutex_init(&lasttime_mx);

	atomic_store(&errcnt, ISC_R_SUCCESS);

	isc_loop_setup(mainloop, setup_test_run, &arg);
	isc_loopmgr_run(loopmgr);

	assert_int_equal(atomic_load(&errcnt), ISC_R_SUCCESS);

	isc_mutex_destroy(&lasttime_mx);
}

static void
set_global_error(isc_result_t result) {
	(void)atomic_compare_exchange_strong(
		&errcnt, &(uint_fast32_t){ ISC_R_SUCCESS }, result);
}

static void
subthread_assert_true(bool expected, const char *file, unsigned int line) {
	if (!expected) {
		printf("# %s:%u subthread_assert_true\n", file, line);
		set_global_error(ISC_R_UNEXPECTED);
	}
}
#define subthread_assert_true(expected) \
	subthread_assert_true(expected, __FILE__, __LINE__)

static void
subthread_assert_result_equal(isc_result_t result, isc_result_t expected,
			      const char *file, unsigned int line) {
	if (result != expected) {
		printf("# %s:%u subthread_assert_result_equal(%u != %u)\n",
		       file, line, (unsigned int)result,
		       (unsigned int)expected);
		set_global_error(result);
	}
}
#define subthread_assert_result_equal(observed, expected) \
	subthread_assert_result_equal(observed, expected, __FILE__, __LINE__)

static void
ticktock(void *arg) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_interval_t interval;
	int tick = atomic_fetch_add(&eventcnt, 1);

	UNUSED(arg);

	if (verbose) {
		print_message("# tick %d\n", tick);
	}

	now = isc_time_now();

	isc_interval_set(&interval, seconds, nanoseconds);
	isc_mutex_lock(&lasttime_mx);
	result = isc_time_add(&lasttime, &interval, &base);
	isc_mutex_unlock(&lasttime_mx);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, FUDGE_SECONDS, FUDGE_NANOSECONDS);
	result = isc_time_add(&base, &interval, &ulim);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	result = isc_time_subtract(&base, &interval, &llim);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	subthread_assert_true(isc_time_compare(&llim, &now) <= 0);
	subthread_assert_true(isc_time_compare(&ulim, &now) >= 0);

	isc_interval_set(&interval, 0, 0);
	isc_mutex_lock(&lasttime_mx);
	result = isc_time_add(&now, &interval, &lasttime);
	isc_mutex_unlock(&lasttime_mx);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	if (atomic_load(&eventcnt) == nevents) {
		endtime = isc_time_now();
		isc_timer_destroy(&timer);
		isc_loopmgr_shutdown(loopmgr);
	}
}

/*
 * Individual unit tests
 */

/* timer type ticker */
ISC_RUN_TEST_IMPL(ticker) {
	isc_interval_t interval;

	UNUSED(state);

	nevents = 12;
	seconds = 0;
	nanoseconds = 500000000;

	isc_interval_set(&interval, seconds, nanoseconds);

	setup_test(isc_timertype_ticker, &interval, ticktock);
}

static void
test_idle(void *arg) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_interval_t interval;
	int tick = atomic_fetch_add(&eventcnt, 1);

	UNUSED(arg);

	if (verbose) {
		print_message("# tick %d\n", tick);
	}

	now = isc_time_now();

	isc_interval_set(&interval, seconds, nanoseconds);
	isc_mutex_lock(&lasttime_mx);
	result = isc_time_add(&lasttime, &interval, &base);
	isc_mutex_unlock(&lasttime_mx);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, FUDGE_SECONDS, FUDGE_NANOSECONDS);
	result = isc_time_add(&base, &interval, &ulim);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	result = isc_time_subtract(&base, &interval, &llim);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	subthread_assert_true(isc_time_compare(&llim, &now) <= 0);
	subthread_assert_true(isc_time_compare(&ulim, &now) >= 0);

	isc_interval_set(&interval, 0, 0);
	isc_mutex_lock(&lasttime_mx);
	isc_time_add(&now, &interval, &lasttime);
	isc_mutex_unlock(&lasttime_mx);

	isc_timer_destroy(&timer);
	isc_loopmgr_shutdown(loopmgr);
}

/* timer type once idles out */
ISC_RUN_TEST_IMPL(once_idle) {
	isc_interval_t interval;

	UNUSED(state);

	nevents = 1;
	seconds = 1;
	nanoseconds = 200000000;

	isc_interval_set(&interval, seconds, nanoseconds);

	setup_test(isc_timertype_once, &interval, test_idle);
}

/* timer reset */
static void
test_reset(void *arg) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_interval_t interval;
	int tick = atomic_fetch_add(&eventcnt, 1);

	UNUSED(arg);

	if (verbose) {
		print_message("# tick %d\n", tick);
	}

	/*
	 * Check expired time.
	 */

	now = isc_time_now();

	isc_interval_set(&interval, seconds, nanoseconds);
	isc_mutex_lock(&lasttime_mx);
	result = isc_time_add(&lasttime, &interval, &base);
	isc_mutex_unlock(&lasttime_mx);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, FUDGE_SECONDS, FUDGE_NANOSECONDS);
	result = isc_time_add(&base, &interval, &ulim);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	result = isc_time_subtract(&base, &interval, &llim);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

	subthread_assert_true(isc_time_compare(&llim, &now) <= 0);
	subthread_assert_true(isc_time_compare(&ulim, &now) >= 0);

	isc_interval_set(&interval, 0, 0);
	isc_mutex_lock(&lasttime_mx);
	isc_time_add(&now, &interval, &lasttime);
	isc_mutex_unlock(&lasttime_mx);

	if (tick < 2) {
		if (tick == 1) {
			isc_interval_set(&interval, seconds, nanoseconds);
			isc_timer_start(timer, isc_timertype_once, &interval);
		}
	} else {
		isc_timer_destroy(&timer);
		isc_loopmgr_shutdown(loopmgr);
	}
}

ISC_RUN_TEST_IMPL(reset) {
	isc_interval_t interval;

	UNUSED(state);

	nevents = 3;
	seconds = 0;
	nanoseconds = 750000000;

	isc_interval_set(&interval, seconds, nanoseconds);

	setup_test(isc_timertype_ticker, &interval, test_reset);
}

static atomic_bool startflag;
static isc_timer_t *tickertimer = NULL;
static isc_timer_t *oncetimer = NULL;

static void
tick_event(void *arg) {
	int tick;

	UNUSED(arg);

	if (!atomic_load(&startflag)) {
		if (verbose) {
			print_message("# tick_event %d\n", -1);
		}
		return;
	}

	tick = atomic_fetch_add(&eventcnt, 1);
	if (verbose) {
		print_message("# tick_event %d\n", tick);
	}

	/*
	 * On the first tick, purge all remaining tick events.
	 */
	if (tick == 0) {
		isc_timer_destroy(&tickertimer);
		isc_loopmgr_shutdown(loopmgr);
	}
}

static void
once_event(void *arg) {
	UNUSED(arg);

	if (verbose) {
		print_message("# once_event\n");
	}

	/*
	 * Allow task1 to start processing events.
	 */
	atomic_store(&startflag, true);

	isc_timer_destroy(&oncetimer);
}

ISC_LOOP_SETUP_IMPL(purge) {
	atomic_init(&eventcnt, 0);
	atomic_store(&errcnt, ISC_R_SUCCESS);
}

ISC_LOOP_TEARDOWN_IMPL(purge) {
	assert_int_equal(atomic_load(&errcnt), ISC_R_SUCCESS);
	assert_int_equal(atomic_load(&eventcnt), 1);
}

/* timer events purged */
ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(purge) {
	isc_interval_t interval;

	UNUSED(arg);

	if (verbose) {
		print_message("# purge_run\n");
	}

	atomic_init(&startflag, 0);
	seconds = 1;
	nanoseconds = 0;

	isc_interval_set(&interval, seconds, 0);

	tickertimer = NULL;
	isc_timer_create(mainloop, tick_event, NULL, &tickertimer);
	isc_timer_start(tickertimer, isc_timertype_ticker, &interval);

	oncetimer = NULL;

	isc_interval_set(&interval, (seconds * 2) + 1, 0);

	isc_timer_create(mainloop, once_event, NULL, &oncetimer);
	isc_timer_start(oncetimer, isc_timertype_once, &interval);
}

/*
 * Set of tests that check whether the rescheduling works as expected.
 */

isc_time_t timer_start;
isc_time_t timer_stop;
uint64_t timer_expect;
uint64_t timer_ticks;
isc_interval_t timer_interval;
isc_timertype_t timer_type;

ISC_LOOP_TEARDOWN_IMPL(timer_expect) {
	uint64_t diff = isc_time_microdiff(&timer_stop, &timer_start) / 1000000;
	assert_true(diff == timer_expect);
}

static void
timer_event(void *arg ISC_ATTR_UNUSED) {
	if (--timer_ticks == 0) {
		isc_timer_destroy(&timer);
		isc_loopmgr_shutdown(loopmgr);
		timer_stop = isc_loop_now(isc_loop());
	} else {
		isc_timer_start(timer, timer_type, &timer_interval);
	}
}

ISC_LOOP_SETUP_IMPL(reschedule_up) {
	timer_start = isc_loop_now(isc_loop());
	timer_expect = 1;
	timer_ticks = 1;
	timer_type = isc_timertype_once;
}

ISC_LOOP_TEST_CUSTOM_IMPL(reschedule_up, setup_loop_reschedule_up,
			  teardown_loop_timer_expect) {
	isc_timer_create(mainloop, timer_event, NULL, &timer);

	/* Schedule the timer to fire immediately */
	isc_interval_set(&timer_interval, 0, 0);
	isc_timer_start(timer, timer_type, &timer_interval);

	/* And then reschedule it to 1 second */
	isc_interval_set(&timer_interval, 1, 0);
	isc_timer_start(timer, timer_type, &timer_interval);
}

ISC_LOOP_SETUP_IMPL(reschedule_down) {
	timer_start = isc_loop_now(isc_loop());
	timer_expect = 0;
	timer_ticks = 1;
	timer_type = isc_timertype_once;
}

ISC_LOOP_TEST_CUSTOM_IMPL(reschedule_down, setup_loop_reschedule_down,
			  teardown_loop_timer_expect) {
	isc_timer_create(mainloop, timer_event, NULL, &timer);

	/* Schedule the timer to fire at 10 seconds */
	isc_interval_set(&timer_interval, 10, 0);
	isc_timer_start(timer, timer_type, &timer_interval);

	/* And then reschedule it fire immediately */
	isc_interval_set(&timer_interval, 0, 0);
	isc_timer_start(timer, timer_type, &timer_interval);
}

ISC_LOOP_SETUP_IMPL(reschedule_from_callback) {
	timer_start = isc_loop_now(isc_loop());
	timer_expect = 1;
	timer_ticks = 2;
	timer_type = isc_timertype_once;
}

ISC_LOOP_TEST_CUSTOM_IMPL(reschedule_from_callback,
			  setup_loop_reschedule_from_callback,
			  teardown_loop_timer_expect) {
	isc_timer_create(mainloop, timer_event, NULL, &timer);

	isc_interval_set(&timer_interval, 0, NS_PER_SEC / 2);
	isc_timer_start(timer, timer_type, &timer_interval);
}

ISC_LOOP_SETUP_IMPL(zero) {
	timer_start = isc_loop_now(isc_loop());
	timer_expect = 0;
	timer_ticks = 1;
	timer_type = isc_timertype_once;
}

ISC_LOOP_TEST_CUSTOM_IMPL(zero, setup_loop_zero, teardown_loop_timer_expect) {
	isc_timer_create(mainloop, timer_event, NULL, &timer);

	/* Schedule the timer to fire immediately (in the next event loop) */
	isc_interval_set(&timer_interval, 0, 0);
	isc_timer_start(timer, timer_type, &timer_interval);
}

ISC_LOOP_SETUP_IMPL(reschedule_ticker) {
	timer_start = isc_loop_now(isc_loop());
	timer_expect = 1;
	timer_ticks = 5;
	timer_type = isc_timertype_ticker;
}

ISC_LOOP_TEST_CUSTOM_IMPL(reschedule_ticker, setup_loop_reschedule_ticker,
			  teardown_loop_timer_expect) {
	isc_timer_create(mainloop, timer_event, NULL, &timer);

	/* Schedule the timer to fire immediately (in the next event loop) */
	isc_interval_set(&timer_interval, 0, 0);
	isc_timer_start(timer, timer_type, &timer_interval);

	/* Then fire every 1/4 second */
	isc_interval_set(&timer_interval, 0, NS_PER_SEC / 4);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(ticker, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(once_idle, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(reset, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(purge, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(reschedule_up, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(reschedule_down, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(reschedule_from_callback, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(zero, setup_loopmgr, teardown_loopmgr)
ISC_TEST_ENTRY_CUSTOM(reschedule_ticker, setup_loopmgr, teardown_loopmgr)

ISC_TEST_LIST_END

ISC_TEST_MAIN
