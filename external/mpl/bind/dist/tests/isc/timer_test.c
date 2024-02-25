/*	$NetBSD: timer_test.c,v 1.2.2.2 2024/02/25 15:47:53 martin Exp $	*/

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
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include "netmgr/uv-compat.h"
#include "timer.c"

#include <tests/isc.h>

/* Set to true (or use -v option) for verbose output */
static bool verbose = false;

#define FUDGE_SECONDS	  0	    /* in absence of clock_getres() */
#define FUDGE_NANOSECONDS 500000000 /* in absence of clock_getres() */

static isc_timer_t *timer = NULL;
static isc_condition_t cv;
static isc_mutex_t mx;
static isc_time_t endtime;
static isc_mutex_t lasttime_mx;
static isc_time_t lasttime;
static int seconds;
static int nanoseconds;
static atomic_int_fast32_t eventcnt;
static atomic_uint_fast32_t errcnt;
static int nevents;

static int
_setup(void **state) {
	atomic_init(&errcnt, ISC_R_SUCCESS);

	setup_managers(state);

	return (0);
}

static int
_teardown(void **state) {
	teardown_managers(state);

	return (0);
}

static void
test_shutdown(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;

	UNUSED(task);

	/*
	 * Signal shutdown processing complete.
	 */
	result = isc_mutex_lock(&mx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_condition_signal(&cv);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_mutex_unlock(&mx);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_event_free(&event);
}

static void
setup_test(isc_timertype_t timertype, isc_time_t *expires,
	   isc_interval_t *interval,
	   void (*action)(isc_task_t *, isc_event_t *)) {
	isc_result_t result;
	isc_task_t *task = NULL;
	isc_time_settoepoch(&endtime);
	atomic_init(&eventcnt, 0);

	isc_mutex_init(&mx);
	isc_mutex_init(&lasttime_mx);

	isc_condition_init(&cv);

	atomic_store(&errcnt, ISC_R_SUCCESS);

	LOCK(&mx);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, test_shutdown, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_mutex_lock(&lasttime_mx);
	result = isc_time_now(&lasttime);
	isc_mutex_unlock(&lasttime_mx);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_timer_create(timermgr, timertype, expires, interval, task,
				  action, (void *)timertype, &timer);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (atomic_load(&eventcnt) != nevents) {
		result = isc_condition_wait(&cv, &mx);
		assert_int_equal(result, ISC_R_SUCCESS);
	}

	UNLOCK(&mx);

	assert_int_equal(atomic_load(&errcnt), ISC_R_SUCCESS);

	isc_task_detach(&task);
	isc_mutex_destroy(&mx);
	isc_mutex_destroy(&lasttime_mx);
	(void)isc_condition_destroy(&cv);
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
subthread_assert_int_equal(int observed, int expected, const char *file,
			   unsigned int line) {
	if (observed != expected) {
		printf("# %s:%u subthread_assert_int_equal(%d != %d)\n", file,
		       line, observed, expected);
		set_global_error(ISC_R_UNEXPECTED);
	}
}
#define subthread_assert_int_equal(observed, expected) \
	subthread_assert_int_equal(observed, expected, __FILE__, __LINE__)

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
ticktock(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_interval_t interval;
	isc_eventtype_t expected_event_type;

	int tick = atomic_fetch_add(&eventcnt, 1);

	if (verbose) {
		print_message("# tick %d\n", tick);
	}

	expected_event_type = ISC_TIMEREVENT_LIFE;
	if ((uintptr_t)event->ev_arg == isc_timertype_ticker) {
		expected_event_type = ISC_TIMEREVENT_TICK;
	}

	if (event->ev_type != expected_event_type) {
		print_error("# expected event type %u, got %u\n",
			    expected_event_type, event->ev_type);
	}

	result = isc_time_now(&now);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

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

	isc_event_free(&event);

	if (atomic_load(&eventcnt) == nevents) {
		result = isc_time_now(&endtime);
		subthread_assert_result_equal(result, ISC_R_SUCCESS);
		isc_timer_destroy(&timer);
		isc_task_shutdown(task);
	}
}

/*
 * Individual unit tests
 */

/* timer type ticker */
ISC_RUN_TEST_IMPL(ticker) {
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(state);

	nevents = 12;
	seconds = 0;
	nanoseconds = 500000000;

	isc_interval_set(&interval, seconds, nanoseconds);
	isc_time_settoepoch(&expires);

	setup_test(isc_timertype_ticker, &expires, &interval, ticktock);
}

/* timer type once reaches lifetime */
ISC_RUN_TEST_IMPL(once_life) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(state);

	nevents = 1;
	seconds = 1;
	nanoseconds = 100000000;

	isc_interval_set(&interval, seconds, nanoseconds);
	result = isc_time_nowplusinterval(&expires, &interval);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, 0, 0);

	setup_test(isc_timertype_once, &expires, &interval, ticktock);
}

static void
test_idle(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_interval_t interval;

	int tick = atomic_fetch_add(&eventcnt, 1);

	if (verbose) {
		print_message("# tick %d\n", tick);
	}

	result = isc_time_now(&now);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

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

	subthread_assert_int_equal(event->ev_type, ISC_TIMEREVENT_IDLE);

	isc_event_free(&event);

	isc_timer_destroy(&timer);
	isc_task_shutdown(task);
}

/* timer type once idles out */
ISC_RUN_TEST_IMPL(once_idle) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(state);

	nevents = 1;
	seconds = 1;
	nanoseconds = 200000000;

	isc_interval_set(&interval, seconds + 1, nanoseconds);
	result = isc_time_nowplusinterval(&expires, &interval);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds, nanoseconds);

	setup_test(isc_timertype_once, &expires, &interval, test_idle);
}

/* timer reset */
static void
test_reset(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_time_t expires;
	isc_interval_t interval;

	int tick = atomic_fetch_add(&eventcnt, 1);

	if (verbose) {
		print_message("# tick %d\n", tick);
	}

	/*
	 * Check expired time.
	 */

	result = isc_time_now(&now);
	subthread_assert_result_equal(result, ISC_R_SUCCESS);

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

	int _eventcnt = atomic_load(&eventcnt);

	if (_eventcnt < 3) {
		subthread_assert_int_equal(event->ev_type, ISC_TIMEREVENT_TICK);

		if (_eventcnt == 2) {
			isc_interval_set(&interval, seconds, nanoseconds);
			result = isc_time_nowplusinterval(&expires, &interval);
			subthread_assert_result_equal(result, ISC_R_SUCCESS);

			isc_interval_set(&interval, 0, 0);
			result = isc_timer_reset(timer, isc_timertype_once,
						 &expires, &interval, false);
			subthread_assert_result_equal(result, ISC_R_SUCCESS);
		}

		isc_event_free(&event);
	} else {
		subthread_assert_int_equal(event->ev_type, ISC_TIMEREVENT_LIFE);

		isc_event_free(&event);
		isc_timer_destroy(&timer);
		isc_task_shutdown(task);
	}
}

ISC_RUN_TEST_IMPL(reset) {
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(state);

	nevents = 3;
	seconds = 0;
	nanoseconds = 750000000;

	isc_interval_set(&interval, seconds, nanoseconds);
	isc_time_settoepoch(&expires);

	setup_test(isc_timertype_ticker, &expires, &interval, test_reset);
}

static atomic_bool startflag;
static atomic_bool shutdownflag;
static isc_timer_t *tickertimer = NULL;
static isc_timer_t *oncetimer = NULL;
static isc_task_t *task1 = NULL;
static isc_task_t *task2 = NULL;

/*
 * task1 blocks on mx while events accumulate
 * in its queue, until signaled by task2.
 */

static void
tick_event(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(task);

	if (!atomic_load(&startflag)) {
		if (verbose) {
			print_message("# tick_event %d\n", -1);
		}
		isc_event_free(&event);
		return;
	}

	int tick = atomic_fetch_add(&eventcnt, 1);
	if (verbose) {
		print_message("# tick_event %d\n", tick);
	}

	/*
	 * On the first tick, purge all remaining tick events
	 * and then shut down the task.
	 */
	if (tick == 0) {
		isc_time_settoepoch(&expires);
		isc_interval_set(&interval, seconds, 0);
		result = isc_timer_reset(tickertimer, isc_timertype_ticker,
					 &expires, &interval, true);
		subthread_assert_result_equal(result, ISC_R_SUCCESS);

		isc_task_shutdown(task);
	}

	isc_event_free(&event);
}

static void
once_event(isc_task_t *task, isc_event_t *event) {
	if (verbose) {
		print_message("# once_event\n");
	}

	/*
	 * Allow task1 to start processing events.
	 */
	atomic_store(&startflag, true);

	isc_event_free(&event);
	isc_task_shutdown(task);
}

static void
shutdown_purge(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);
	UNUSED(event);

	if (verbose) {
		print_message("# shutdown_event\n");
	}

	/*
	 * Signal shutdown processing complete.
	 */
	atomic_store(&shutdownflag, 1);

	isc_event_free(&event);
}

/* timer events purged */
ISC_RUN_TEST_IMPL(purge) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(state);

	atomic_init(&startflag, 0);
	atomic_init(&shutdownflag, 0);
	atomic_init(&eventcnt, 0);
	seconds = 1;
	nanoseconds = 0;

	result = isc_task_create(taskmgr, 0, &task1);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task1, shutdown_purge, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task2);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_time_settoepoch(&expires);
	isc_interval_set(&interval, seconds, 0);

	tickertimer = NULL;
	result = isc_timer_create(timermgr, isc_timertype_ticker, &expires,
				  &interval, task1, tick_event, NULL,
				  &tickertimer);
	assert_int_equal(result, ISC_R_SUCCESS);

	oncetimer = NULL;

	isc_interval_set(&interval, (seconds * 2) + 1, 0);
	result = isc_time_nowplusinterval(&expires, &interval);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, 0, 0);
	result = isc_timer_create(timermgr, isc_timertype_once, &expires,
				  &interval, task2, once_event, NULL,
				  &oncetimer);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (!atomic_load(&shutdownflag)) {
		uv_sleep(1);
	}

	assert_int_equal(atomic_load(&errcnt), ISC_R_SUCCESS);

	assert_int_equal(atomic_load(&eventcnt), 1);

	isc_timer_destroy(&tickertimer);
	isc_timer_destroy(&oncetimer);
	isc_task_destroy(&task1);
	isc_task_destroy(&task2);
}

ISC_TEST_LIST_START

ISC_TEST_ENTRY_CUSTOM(ticker, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(once_life, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(once_idle, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(reset, _setup, _teardown)
ISC_TEST_ENTRY_CUSTOM(purge, _setup, _teardown)

ISC_TEST_LIST_END

ISC_TEST_MAIN
