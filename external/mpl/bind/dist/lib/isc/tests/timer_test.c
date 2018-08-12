/*	$NetBSD: timer_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <isc/condition.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>
#include <isc/util.h>

#include "isctest.h"

/*
 * This entire test requires threads.
 */
#ifdef ISC_PLATFORM_USETHREADS

/*
 * Helper functions
 */
#define	FUDGE_SECONDS	0	     /* in absence of clock_getres() */
#define	FUDGE_NANOSECONDS	500000000    /* in absence of clock_getres() */

static	isc_timer_t *timer = NULL;
static	isc_condition_t cv;
static	isc_mutex_t mx;
static	isc_time_t endtime;
static	isc_time_t lasttime;
static	int seconds;
static	int nanoseconds;
static	int eventcnt;
static	int nevents;

static void
shutdown(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;

	UNUSED(task);

	/*
	 * Signal shutdown processing complete.
	 */
	result = isc_mutex_lock(&mx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_signal(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mutex_unlock(&mx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_event_free(&event);
}

static void
setup_test(isc_timertype_t timertype, isc_time_t *expires,
	   isc_interval_t *interval,
	   void (*action)(isc_task_t *, isc_event_t *))
{
	isc_result_t result;
	isc_task_t *task = NULL;
	isc_time_settoepoch(&endtime);
	eventcnt = 0;

	result = isc_mutex_init(&mx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_init(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	LOCK(&mx);

	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, shutdown, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_time_now(&lasttime);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_timer_create(timermgr, timertype, expires, interval,
				  task, action, (void *)timertype,
				  &timer);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (eventcnt != nevents) {
		result = isc_condition_wait(&cv, &mx);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	UNLOCK(&mx);

	isc_task_detach(&task);
	DESTROYLOCK(&mx);
	(void) isc_condition_destroy(&cv);
}

static void
ticktock(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_interval_t interval;
	isc_eventtype_t expected_event_type;

	++eventcnt;

	printf("tick %d\n", eventcnt);

	expected_event_type = ISC_TIMEREVENT_LIFE;
	if ((isc_timertype_t) event->ev_arg == isc_timertype_ticker) {
		expected_event_type = ISC_TIMEREVENT_TICK;
	}

	if (event->ev_type != expected_event_type) {
		printf("expected event type %u, got %u\n",
		       expected_event_type, event->ev_type);
	}

	result = isc_time_now(&now);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds, nanoseconds);
	result = isc_time_add(&lasttime, &interval, &base);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, FUDGE_SECONDS, FUDGE_NANOSECONDS);
	result = isc_time_add(&base, &interval, &ulim);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_time_subtract(&base, &interval, &llim);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_CHECK(isc_time_compare(&llim, &now) <= 0);
	ATF_CHECK(isc_time_compare(&ulim, &now) >= 0);
	lasttime = now;

	if (eventcnt == nevents) {
		result = isc_time_now(&endtime);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		isc_timer_detach(&timer);
		isc_task_shutdown(task);
	}

	isc_event_free(&event);
}

/*
 * Individual unit tests
 */

ATF_TC(ticker);
ATF_TC_HEAD(ticker, tc) {
	atf_tc_set_md_var(tc, "descr", "timer type ticker");
}
ATF_TC_BODY(ticker, tc) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(tc);

	nevents = 12;
	seconds = 0;
	nanoseconds = 500000000;

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds, nanoseconds);
	isc_time_settoepoch(&expires);

	setup_test(isc_timertype_ticker, &expires, &interval, ticktock);

	isc_test_end();
}

ATF_TC(once_life);
ATF_TC_HEAD(once_life, tc) {
	atf_tc_set_md_var(tc, "descr", "timer type once reaches lifetime");
}
ATF_TC_BODY(once_life, tc) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(tc);

	nevents = 1;
	seconds = 1;
	nanoseconds = 100000000;

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds, nanoseconds);
	result = isc_time_nowplusinterval(&expires, &interval);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, 0, 0);

	setup_test(isc_timertype_once, &expires, &interval, ticktock);

	isc_test_end();
}


static void
test_idle(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_interval_t interval;

	++eventcnt;

	printf("tick %d\n", eventcnt);

	result = isc_time_now(&now);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds, nanoseconds);
	result = isc_time_add(&lasttime, &interval, &base);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, FUDGE_SECONDS, FUDGE_NANOSECONDS);
	result = isc_time_add(&base, &interval, &ulim);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_time_subtract(&base, &interval, &llim);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_CHECK(isc_time_compare(&llim, &now) <= 0);
	ATF_CHECK(isc_time_compare(&ulim, &now) >= 0);
	lasttime = now;

	ATF_CHECK_EQ(event->ev_type, ISC_TIMEREVENT_IDLE);

	isc_timer_detach(&timer);
	isc_task_shutdown(task);
	isc_event_free(&event);
}

ATF_TC(once_idle);
ATF_TC_HEAD(once_idle, tc) {
	atf_tc_set_md_var(tc, "descr", "timer type once idles out");
}
ATF_TC_BODY(once_idle, tc) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(tc);

	nevents = 1;
	seconds = 1;
	nanoseconds = 200000000;

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds + 1, nanoseconds);
	result = isc_time_nowplusinterval(&expires, &interval);
	ATF_CHECK_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds, nanoseconds);

	setup_test(isc_timertype_once, &expires, &interval, test_idle);

	isc_test_end();
}

static void
test_reset(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t now;
	isc_time_t base;
	isc_time_t ulim;
	isc_time_t llim;
	isc_time_t expires;
	isc_interval_t interval;

	++eventcnt;

	printf("tick %d\n", eventcnt);

	/*
	 * Check expired time.
	 */

	result = isc_time_now(&now);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, seconds, nanoseconds);
	result = isc_time_add(&lasttime, &interval, &base);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, FUDGE_SECONDS, FUDGE_NANOSECONDS);
	result = isc_time_add(&base, &interval, &ulim);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_time_subtract(&base, &interval, &llim);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ATF_CHECK(isc_time_compare(&llim, &now) <= 0);
	ATF_CHECK(isc_time_compare(&ulim, &now) >= 0);
	lasttime = now;

	if (eventcnt < 3) {
		ATF_CHECK_EQ(event->ev_type, ISC_TIMEREVENT_TICK);

		if (eventcnt == 2) {
			isc_interval_set(&interval, seconds, nanoseconds);
			result = isc_time_nowplusinterval(&expires, &interval);
			ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

			isc_interval_set(&interval, 0, 0);
			result = isc_timer_reset(timer, isc_timertype_once,
						 &expires, &interval,
						 ISC_FALSE);
			ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
		}
	} else {
		ATF_CHECK_EQ(event->ev_type, ISC_TIMEREVENT_LIFE);

		isc_timer_detach(&timer);
		isc_task_shutdown(task);
	}

	isc_event_free(&event);
}

ATF_TC(reset);
ATF_TC_HEAD(reset, tc) {
	atf_tc_set_md_var(tc, "descr", "timer reset");
}
ATF_TC_BODY(reset, tc) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	nevents = 3;
	seconds = 0;
	nanoseconds = 750000000;

	isc_interval_set(&interval, seconds, nanoseconds);
	isc_time_settoepoch(&expires);

	setup_test(isc_timertype_ticker, &expires, &interval, test_reset);

	isc_test_end();
}

static int startflag;
static int shutdownflag;
static isc_timer_t *tickertimer = NULL;
static isc_timer_t *oncetimer = NULL;
static isc_task_t *task1 = NULL;
static isc_task_t *task2 = NULL;

/*
 * task1 blocks on mx while events accumulate
 * in its queue, until signaled by task2.
 */

static void
start_event(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	printf("start_event\n");

	LOCK(&mx);
	while (! startflag) {
		(void) isc_condition_wait(&cv, &mx);
	}
	UNLOCK(&mx);

	isc_event_free(&event);
}

static void
tick_event(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(task);

	++eventcnt;
	printf("tick_event %d\n", eventcnt);

	/*
	 * On the first tick, purge all remaining tick events
	 * and then shut down the task.
	 */
	if (eventcnt == 1) {
		isc_time_settoepoch(&expires);
		isc_interval_set(&interval, seconds, 0);
		result = isc_timer_reset(tickertimer, isc_timertype_ticker,
					 &expires, &interval, ISC_TRUE);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		isc_task_shutdown(task);
	}

	isc_event_free(&event);
}

static void
once_event(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;

	printf("once_event\n");

	/*
	 * Allow task1 to start processing events.
	 */
	LOCK(&mx);
	startflag = 1;

	result = isc_condition_broadcast(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	UNLOCK(&mx);

	isc_event_free(&event);
	isc_task_shutdown(task);
}

static void
shutdown_purge(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;

	UNUSED(task);
	UNUSED(event);

	printf("shutdown_event\n");

	/*
	 * Signal shutdown processing complete.
	 */
	LOCK(&mx);
	shutdownflag = 1;

	result = isc_condition_signal(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	UNLOCK(&mx);

	isc_event_free(&event);
}

ATF_TC(purge);
ATF_TC_HEAD(purge, tc) {
	atf_tc_set_md_var(tc, "descr", "timer events purged");
}
ATF_TC_BODY(purge, tc) {
	isc_result_t result;
	isc_event_t *event = NULL;
	isc_time_t expires;
	isc_interval_t interval;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	startflag = 0;
	shutdownflag = 0;
	eventcnt = 0;
	seconds = 1;
	nanoseconds = 0;

	result = isc_mutex_init(&mx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_init(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task1, shutdown_purge, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	LOCK(&mx);

	event = isc_event_allocate(mctx, (void *)1 , (isc_eventtype_t)1,
				   start_event, NULL, sizeof(*event));
	ATF_REQUIRE(event != NULL);
	isc_task_send(task1, &event);

	isc_time_settoepoch(&expires);
	isc_interval_set(&interval, seconds, 0);

	tickertimer = NULL;
	result = isc_timer_create(timermgr, isc_timertype_ticker,
				  &expires, &interval, task1,
				  tick_event, NULL, &tickertimer);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	oncetimer = NULL;

	isc_interval_set(&interval, (seconds * 2) + 1, 0);
	result = isc_time_nowplusinterval(&expires, &interval);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_interval_set(&interval, 0, 0);
	result = isc_timer_create(timermgr, isc_timertype_once,
				      &expires, &interval, task2,
				      once_event, NULL, &oncetimer);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (! shutdownflag) {
		result = isc_condition_wait(&cv, &mx);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	}

	UNLOCK(&mx);

	ATF_CHECK_EQ(eventcnt, 1);

	isc_timer_detach(&tickertimer);
	isc_timer_detach(&oncetimer);
	isc_task_destroy(&task1);
	isc_task_destroy(&task2);
	DESTROYLOCK(&mx);

	isc_test_end();
}
#else
ATF_TC(untested);
ATF_TC_HEAD(untested, tc) {
	atf_tc_set_md_var(tc, "descr", "skipping nsec3 test");
}
ATF_TC_BODY(untested, tc) {
	UNUSED(tc);
	atf_tc_skip("DNSSEC not available");
}
#endif

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
#ifdef ISC_PLATFORM_USETHREADS
	ATF_TP_ADD_TC(tp, ticker);
	ATF_TP_ADD_TC(tp, once_life);
	ATF_TP_ADD_TC(tp, once_idle);
	ATF_TP_ADD_TC(tp, reset);
	ATF_TP_ADD_TC(tp, purge);
#else
	ATF_TP_ADD_TC(tp, untested);
#endif

	return (atf_no_error());
}
