/*	$NetBSD: task_test.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#ifdef HAVE_INTTYPES_H
#include <inttypes.h> 	/* uintptr_t */
#endif
#include <stdlib.h>
#include <unistd.h>

#include <isc/condition.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include "isctest.h"

/*
 * Helper functions
 */

static isc_mutex_t lock;
int counter = 0;
static int active[10];
static isc_boolean_t done = ISC_FALSE;

#ifdef ISC_PLATFORM_USETHREADS
static isc_condition_t cv;
#endif

static void
set(isc_task_t *task, isc_event_t *event) {
	int *value = (int *) event->ev_arg;

	UNUSED(task);

	isc_event_free(&event);
	LOCK(&lock);
	*value = counter++;
	UNLOCK(&lock);
}

static void
set_and_drop(isc_task_t *task, isc_event_t *event) {
	int *value = (int *) event->ev_arg;

	UNUSED(task);

	isc_event_free(&event);
	LOCK(&lock);
	*value = (int) isc_taskmgr_mode(taskmgr);
	counter++;
	UNLOCK(&lock);
	isc_taskmgr_setmode(taskmgr, isc_taskmgrmode_normal);
}

/*
 * Individual unit tests
 */

/* Create a task */
ATF_TC(create_task);
ATF_TC_HEAD(create_task, tc) {
	atf_tc_set_md_var(tc, "descr", "create and destroy a task");
}
ATF_TC_BODY(create_task, tc) {
	isc_result_t result;
	isc_task_t *task = NULL;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_task_destroy(&task);
	ATF_REQUIRE_EQ(task, NULL);

	isc_test_end();
}

/* Process events */
ATF_TC(all_events);
ATF_TC_HEAD(all_events, tc) {
	atf_tc_set_md_var(tc, "descr", "process task events");
}
ATF_TC_BODY(all_events, tc) {
	isc_result_t result;
	isc_task_t *task = NULL;
	isc_event_t *event = NULL;
	int a = 0, b = 0;
	int i = 0;

	UNUSED(tc);

	counter = 1;

	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/* First event */
	event = isc_event_allocate(mctx, task, ISC_TASKEVENT_TEST,
				   set, &a, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(a, 0);
	isc_task_send(task, &event);

	event = isc_event_allocate(mctx, task, ISC_TASKEVENT_TEST,
				   set, &b, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(b, 0);
	isc_task_send(task, &event);

	while ((a == 0 || b == 0) && i++ < 5000) {
#ifndef ISC_PLATFORM_USETHREADS
		while (isc__taskmgr_ready(taskmgr))
			isc__taskmgr_dispatch(taskmgr);
#endif
		isc_test_nap(1000);
	}

	ATF_CHECK(a != 0);
	ATF_CHECK(b != 0);

	isc_task_destroy(&task);
	ATF_REQUIRE_EQ(task, NULL);

	isc_test_end();
}

/* Privileged events */
ATF_TC(privileged_events);
ATF_TC_HEAD(privileged_events, tc) {
	atf_tc_set_md_var(tc, "descr", "process privileged events");
}
ATF_TC_BODY(privileged_events, tc) {
	isc_result_t result;
	isc_task_t *task1 = NULL, *task2 = NULL;
	isc_event_t *event = NULL;
	int a = 0, b = 0, c = 0, d = 0, e = 0;
	int i = 0;

	UNUSED(tc);

	counter = 1;
	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

#ifdef ISC_PLATFORM_USETHREADS
	/*
	 * Pause the task manager so we can fill up the work queue
	 * without things happening while we do it.
	 */
	isc__taskmgr_pause(taskmgr);
#endif

	result = isc_task_create(taskmgr, 0, &task1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_task_setname(task1, "privileged", NULL);
	ATF_CHECK(!isc_task_privilege(task1));
	isc_task_setprivilege(task1, ISC_TRUE);
	ATF_CHECK(isc_task_privilege(task1));

	result = isc_task_create(taskmgr, 0, &task2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_task_setname(task2, "normal", NULL);
	ATF_CHECK(!isc_task_privilege(task2));

	/* First event: privileged */
	event = isc_event_allocate(mctx, task1, ISC_TASKEVENT_TEST,
				   set, &a, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(a, 0);
	isc_task_send(task1, &event);

	/* Second event: not privileged */
	event = isc_event_allocate(mctx, task2, ISC_TASKEVENT_TEST,
				   set, &b, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(b, 0);
	isc_task_send(task2, &event);

	/* Third event: privileged */
	event = isc_event_allocate(mctx, task1, ISC_TASKEVENT_TEST,
				   set, &c, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(c, 0);
	isc_task_send(task1, &event);

	/* Fourth event: privileged */
	event = isc_event_allocate(mctx, task1, ISC_TASKEVENT_TEST,
				   set, &d, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(d, 0);
	isc_task_send(task1, &event);

	/* Fifth event: not privileged */
	event = isc_event_allocate(mctx, task2, ISC_TASKEVENT_TEST,
				   set, &e, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(e, 0);
	isc_task_send(task2, &event);

	ATF_CHECK_EQ(isc_taskmgr_mode(taskmgr), isc_taskmgrmode_normal);
	isc_taskmgr_setmode(taskmgr, isc_taskmgrmode_privileged);
	ATF_CHECK_EQ(isc_taskmgr_mode(taskmgr), isc_taskmgrmode_privileged);

#ifdef ISC_PLATFORM_USETHREADS
	isc__taskmgr_resume(taskmgr);
#endif

	/* We're waiting for *all* variables to be set */
	while ((a == 0 || b == 0 || c == 0 || d == 0 || e == 0) && i++ < 5000) {
#ifndef ISC_PLATFORM_USETHREADS
		while (isc__taskmgr_ready(taskmgr))
			isc__taskmgr_dispatch(taskmgr);
#endif
		isc_test_nap(1000);
	}

	/*
	 * We can't guarantee what order the events fire, but
	 * we do know the privileged tasks that set a, c, and d
	 * would have fired first.
	 */
	ATF_CHECK(a <= 3);
	ATF_CHECK(c <= 3);
	ATF_CHECK(d <= 3);

	/* ...and the non-privileged tasks that set b and e, last */
	ATF_CHECK(b >= 4);
	ATF_CHECK(e >= 4);

	ATF_CHECK_EQ(counter, 6);

	isc_task_setprivilege(task1, ISC_FALSE);
	ATF_CHECK(!isc_task_privilege(task1));

	ATF_CHECK_EQ(isc_taskmgr_mode(taskmgr), isc_taskmgrmode_normal);

	isc_task_destroy(&task1);
	ATF_REQUIRE_EQ(task1, NULL);
	isc_task_destroy(&task2);
	ATF_REQUIRE_EQ(task2, NULL);

	isc_test_end();
}

/*
 * Edge case: this tests that the task manager behaves as expected when
 * we explicitly set it into normal mode *while* running privileged.
 */
ATF_TC(privilege_drop);
ATF_TC_HEAD(privilege_drop, tc) {
	atf_tc_set_md_var(tc, "descr", "process privileged events");
}
ATF_TC_BODY(privilege_drop, tc) {
	isc_result_t result;
	isc_task_t *task1 = NULL, *task2 = NULL;
	isc_event_t *event = NULL;
	int a = -1, b = -1, c = -1, d = -1, e = -1;	/* non valid states */
	int i = 0;

	UNUSED(tc);

	counter = 1;
	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_test_begin(NULL, ISC_TRUE, 0);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

#ifdef ISC_PLATFORM_USETHREADS
	/*
	 * Pause the task manager so we can fill up the work queue
	 * without things happening while we do it.
	 */
	isc__taskmgr_pause(taskmgr);
#endif

	result = isc_task_create(taskmgr, 0, &task1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_task_setname(task1, "privileged", NULL);
	ATF_CHECK(!isc_task_privilege(task1));
	isc_task_setprivilege(task1, ISC_TRUE);
	ATF_CHECK(isc_task_privilege(task1));

	result = isc_task_create(taskmgr, 0, &task2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_task_setname(task2, "normal", NULL);
	ATF_CHECK(!isc_task_privilege(task2));

	/* First event: privileged */
	event = isc_event_allocate(mctx, task1, ISC_TASKEVENT_TEST,
				   set_and_drop, &a, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(a, -1);
	isc_task_send(task1, &event);

	/* Second event: not privileged */
	event = isc_event_allocate(mctx, task2, ISC_TASKEVENT_TEST,
				   set_and_drop, &b, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(b, -1);
	isc_task_send(task2, &event);

	/* Third event: privileged */
	event = isc_event_allocate(mctx, task1, ISC_TASKEVENT_TEST,
				   set_and_drop, &c, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(c, -1);
	isc_task_send(task1, &event);

	/* Fourth event: privileged */
	event = isc_event_allocate(mctx, task1, ISC_TASKEVENT_TEST,
				   set_and_drop, &d, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(d, -1);
	isc_task_send(task1, &event);

	/* Fifth event: not privileged */
	event = isc_event_allocate(mctx, task2, ISC_TASKEVENT_TEST,
				   set_and_drop, &e, sizeof (isc_event_t));
	ATF_REQUIRE(event != NULL);

	ATF_CHECK_EQ(e, -1);
	isc_task_send(task2, &event);

	ATF_CHECK_EQ(isc_taskmgr_mode(taskmgr), isc_taskmgrmode_normal);
	isc_taskmgr_setmode(taskmgr, isc_taskmgrmode_privileged);
	ATF_CHECK_EQ(isc_taskmgr_mode(taskmgr), isc_taskmgrmode_privileged);

#ifdef ISC_PLATFORM_USETHREADS
	isc__taskmgr_resume(taskmgr);
#endif

	/* We're waiting for all variables to be set. */
	while ((a == -1 || b == -1 || c == -1 || d == -1 || e == -1) &&
	       i++ < 5000) {
#ifndef ISC_PLATFORM_USETHREADS
		while (isc__taskmgr_ready(taskmgr))
			isc__taskmgr_dispatch(taskmgr);
#endif
		isc_test_nap(1000);
	}

	/*
	 * We can't guarantee what order the events fire, but
	 * we do know *exactly one* of the privileged tasks will
	 * have run in privileged mode...
	 */
	ATF_CHECK(a == isc_taskmgrmode_privileged ||
		  c == isc_taskmgrmode_privileged ||
		  d == isc_taskmgrmode_privileged);
	ATF_CHECK(a + c + d == isc_taskmgrmode_privileged);

	/* ...and neither of the non-privileged tasks did... */
	ATF_CHECK(b == isc_taskmgrmode_normal || e == isc_taskmgrmode_normal);

	/* ...but all five of them did run. */
	ATF_CHECK_EQ(counter, 6);

	ATF_CHECK_EQ(isc_taskmgr_mode(taskmgr), isc_taskmgrmode_normal);

	isc_task_destroy(&task1);
	ATF_REQUIRE_EQ(task1, NULL);
	isc_task_destroy(&task2);
	ATF_REQUIRE_EQ(task2, NULL);

	isc_test_end();
}

/*
 * Basic task functions:
 */
static void
basic_cb(isc_task_t *task, isc_event_t *event) {
	int i;
	int j;

	UNUSED(task);

	j = 0;
	for (i = 0; i < 1000000; i++) {
		j += 100;
	}

	printf("task %s\n", (char *)event->ev_arg);
	isc_event_free(&event);
}

static void
basic_shutdown(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	printf("shutdown %s\n", (char *)event->ev_arg);
	isc_event_free(&event);
}

static void
basic_tick(isc_task_t *task, isc_event_t *event) {

	UNUSED(task);

	printf("%s\n", (char *)event->ev_arg);
	isc_event_free(&event);
}

static char one[] = "1";
static char two[] = "2";
static char three[] = "3";
static char four[] = "4";
static char tick[] = "tick";
static char tock[] = "tock";


ATF_TC(basic);
ATF_TC_HEAD(basic, tc) {
	atf_tc_set_md_var(tc, "descr", "basic task system check");
}
ATF_TC_BODY(basic, tc) {
	isc_result_t result;
	isc_task_t *task1 = NULL;
	isc_task_t *task2 = NULL;
	isc_task_t *task3 = NULL;
	isc_task_t *task4 = NULL;
	isc_event_t *event = NULL;
	isc_timer_t *ti1 = NULL;
	isc_timer_t *ti2 = NULL;
	isc_time_t absolute;
	isc_interval_t interval;
	char *testarray[] = {
		one, one, one, one, one, one, one, one, one,
		two, three, four, two, three, four, NULL
	};
	int i;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_task_create(taskmgr, 0, &task2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_task_create(taskmgr, 0, &task3);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_task_create(taskmgr, 0, &task4);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task1, basic_shutdown, one);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_task_onshutdown(task2, basic_shutdown, two);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_task_onshutdown(task3, basic_shutdown, three);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	result = isc_task_onshutdown(task4, basic_shutdown, four);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_time_settoepoch(&absolute);
	isc_interval_set(&interval, 1, 0);
	result = isc_timer_create(timermgr, isc_timertype_ticker,
				&absolute, &interval,
				task1, basic_tick, tick, &ti1);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	ti2 = NULL;
	isc_time_settoepoch(&absolute);
	isc_interval_set(&interval, 1, 0);
	result = isc_timer_create(timermgr, isc_timertype_ticker,
				  &absolute, &interval,
				  task2, basic_tick, tock, &ti2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

#ifndef WIN32
	sleep(2);
#else
	Sleep(2000);
#endif

	for (i = 0; testarray[i] != NULL; i++) {
		/*
		 * Note:  (void *)1 is used as a sender here, since some
		 * compilers don't like casting a function pointer to a
		 * (void *).
		 *
		 * In a real use, it is more likely the sender would be a
		 * structure (socket, timer, task, etc) but this is just a
		 * test program.
		 */
		event = isc_event_allocate(mctx, (void *)1, 1, basic_cb,
					   testarray[i], sizeof(*event));
		ATF_REQUIRE(event != NULL);
		isc_task_send(task1, &event);
	}

	(void)isc_task_purge(task3, NULL, 0, 0);

	isc_task_detach(&task1);
	isc_task_detach(&task2);
	isc_task_detach(&task3);
	isc_task_detach(&task4);

#ifndef WIN32
	sleep(10);
#else
	Sleep(10000);
#endif
	isc_timer_detach(&ti1);
	isc_timer_detach(&ti2);

	isc_test_end();
}

/*
 * Exclusive mode test:
 * When one task enters exclusive mode, all other active
 * tasks complete first.
 */
static
int spin(int n) {
	int i;
	int r = 0;
	for (i = 0; i < n; i++) {
		r += i;
		if (r > 1000000)
			r = 0;
	}
	return (r);
}

static void
exclusive_cb(isc_task_t *task, isc_event_t *event) {
	int taskno = *(int *)(event->ev_arg);

	printf("task enter %d\n", taskno);

	/* task chosen from the middle of the range */
	if (taskno == 6) {
		isc_result_t result;
		int i;

		result = isc_task_beginexclusive(task);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		for (i = 0; i < 10; i++) {
			ATF_CHECK(active[i] == 0);
		}

		isc_task_endexclusive(task);
		done = ISC_TRUE;
	} else {
		active[taskno]++;
		(void) spin(10000000);
		active[taskno]--;
	}

	printf("task exit %d\n", taskno);

	if (done) {
		isc_mem_put(event->ev_destroy_arg, event->ev_arg, sizeof (int));
		isc_event_free(&event);
	} else {
		isc_task_send(task, &event);
	}
}

ATF_TC(task_exclusive);
ATF_TC_HEAD(task_exclusive, tc) {
	atf_tc_set_md_var(tc, "descr", "test exclusive mode");
}
ATF_TC_BODY(task_exclusive, tc) {
	isc_task_t *tasks[10];
	isc_result_t result;
	int i;

	UNUSED(tc);

	result = isc_test_begin(NULL, ISC_TRUE, 4);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	for (i = 0; i < 10; i++) {
		isc_event_t *event = NULL;
		int *v;

		tasks[i] = NULL;

		result = isc_task_create(taskmgr, 0, &tasks[i]);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		v = isc_mem_get(mctx, sizeof *v);
		ATF_REQUIRE(v != NULL);

		*v = i;

		event = isc_event_allocate(mctx, NULL, 1, exclusive_cb,
					   v, sizeof(*event));
		ATF_REQUIRE(event != NULL);

		isc_task_send(tasks[i], &event);
	}

	for (i = 0; i < 10; i++) {
		isc_task_detach(&tasks[i]);
	}
	isc_test_end();
}

/*
 * The remainder of these tests require threads
 */
#ifdef ISC_PLATFORM_USETHREADS
/*
 * Max tasks test:
 * The task system can create and execute many tasks. Tests with 10000.
 */
static void
maxtask_shutdown(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	if (event->ev_arg != NULL) {
		isc_task_destroy((isc_task_t**) &event->ev_arg);
	} else {
		LOCK(&lock);
		done = ISC_TRUE;
		SIGNAL(&cv);
		UNLOCK(&lock);

		isc_event_free(&event);
		isc_taskmgr_destroy(&taskmgr);
		isc_mem_destroy(&mctx);

		isc_condition_destroy(&cv);
		DESTROYLOCK(&lock);
	}
}

static void
maxtask_cb(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;

	if (event->ev_arg != NULL) {
		isc_task_t *newtask = NULL;

		event->ev_arg = (void *)(((uintptr_t) event->ev_arg) - 1);

		/*
		 * Create a new task and forward the message.
		 */
		result = isc_task_create(taskmgr, 0, &newtask);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		result = isc_task_onshutdown(newtask, maxtask_shutdown,
						 (void *)task);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		isc_task_send(newtask, &event);
	} else if (task != NULL) {
		isc_task_destroy(&task);
	}
}

ATF_TC(manytasks);
ATF_TC_HEAD(manytasks, tc) {
	atf_tc_set_md_var(tc, "descr", "many tasks");
}
ATF_TC_BODY(manytasks, tc) {
	isc_result_t result;
	isc_event_t *event = NULL;
	uintptr_t ntasks = 10000;

	UNUSED(tc);

	printf("Testing with %lu tasks\n", (unsigned long)ntasks);

	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_init(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mem_create(0, 0, &mctx);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_taskmgr_create(mctx, 4, 0, &taskmgr);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	event = isc_event_allocate(mctx, (void *)1, 1, maxtask_cb,
				   (void *)ntasks, sizeof(*event));
	ATF_REQUIRE(event != NULL);

	LOCK(&lock);
	maxtask_cb(NULL, event);
	while (!done) {
		WAIT(&cv, &lock);
	}
}


/*
 * Shutdown test:
 * When isc_task_shutdown() is called, shutdown events are posted
 * in LIFO order.
 */

static int senders[4];
static int nevents = 0;
static int nsdevents = 0;

static void
sd_sde1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	ATF_CHECK_EQ(nevents, 256);
	ATF_REQUIRE_EQ(nsdevents, 1);
	++nsdevents;
	printf("shutdown 1\n");

	isc_event_free(&event);
}

static void
sd_sde2(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	ATF_CHECK_EQ(nevents, 256);
	ATF_REQUIRE_EQ(nsdevents, 0);
	++nsdevents;
	printf("shutdown 2\n");

	isc_event_free(&event);
}

static void
sd_event1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	while (!done) {
		WAIT(&cv, &lock);
	}

	printf("event 1\n");

	isc_event_free(&event);
}

static void
sd_event2(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	++nevents;

	printf("event 2\n");

	isc_event_free(&event);
}

ATF_TC(shutdown);
ATF_TC_HEAD(shutdown, tc) {
	atf_tc_set_md_var(tc, "descr", "task shutdown");
}
ATF_TC_BODY(shutdown, tc) {
	isc_result_t result;
	isc_eventtype_t event_type;
	isc_event_t *event = NULL;
	isc_task_t *task = NULL;
	int i;

	nevents = nsdevents = 0;
	done = ISC_FALSE;

	event_type = 3;

	result = isc_test_begin(NULL, ISC_TRUE, 4);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_init(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	LOCK(&lock);

	task = NULL;
	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * This event causes the task to wait on cv.
	 */
	event = isc_event_allocate(mctx, &senders[1], event_type, sd_event1,
				   NULL, sizeof(*event));
	ATF_REQUIRE(event != NULL);
	isc_task_send(task, &event);

	/*
	 * Now we fill up the task's event queue with some events.
	 */
	for (i = 0; i < 256; ++i) {
		event = isc_event_allocate(mctx, &senders[1], event_type,
					   sd_event2, NULL, sizeof(*event));
		ATF_REQUIRE(event != NULL);
		isc_task_send(task, &event);
	}

	/*
	 * Now we register two shutdown events.
	 */
	result = isc_task_onshutdown(task, sd_sde1, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, sd_sde2, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_task_shutdown(task);

	/*
	 * Now we free the task by signaling cv.
	 */
	done = ISC_TRUE;
	SIGNAL(&cv);
	UNLOCK(&lock);

	isc_task_detach(&task);

	isc_test_end();

	ATF_REQUIRE_EQ(nsdevents, 2);
}

/*
 * Post-shutdown test:
 * After isc_task_shutdown() has been called, any call to
 * isc_task_onshutdown() will return ISC_R_SHUTTINGDOWN.
 */
static void
psd_event1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);

	while (!done) {
		WAIT(&cv, &lock);
	}

	UNLOCK(&lock);

	isc_event_free(&event);
}

static void
psd_sde(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	isc_event_free(&event);
}

ATF_TC(post_shutdown);
ATF_TC_HEAD(post_shutdown, tc) {
	atf_tc_set_md_var(tc, "descr", "post-shutdown");
}
ATF_TC_BODY(post_shutdown, tc) {
	isc_result_t result;
	isc_eventtype_t event_type;
	isc_event_t *event;
	isc_task_t *task;

	done = ISC_FALSE;
	event_type = 4;

	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_init(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	LOCK(&lock);

	task = NULL;
	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * This event causes the task to wait on cv.
	 */
	event = isc_event_allocate(mctx, &senders[1], event_type, psd_event1,
				   NULL, sizeof(*event));
	ATF_REQUIRE(event != NULL);
	isc_task_send(task, &event);

	isc_task_shutdown(task);

	result = isc_task_onshutdown(task, psd_sde, NULL);
	ATF_CHECK_EQ(result, ISC_R_SHUTTINGDOWN);

	/*
	 * Release the task.
	 */
	done = ISC_TRUE;

	SIGNAL(&cv);
	UNLOCK(&lock);

	isc_task_detach(&task);
	isc_test_end();

	(void) isc_condition_destroy(&cv);
	DESTROYLOCK(&lock);
}

/*
 * Helper for the purge tests below:
 */

#define	SENDERCNT 3
#define	TYPECNT 4
#define	TAGCNT 5
#define	NEVENTS	(SENDERCNT * TYPECNT * TAGCNT)

static isc_boolean_t testrange;
static void *purge_sender;
static isc_eventtype_t purge_type_first;
static isc_eventtype_t purge_type_last;
static void *purge_tag;
static int eventcnt;

isc_boolean_t started = ISC_FALSE;

static void
pg_event1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	while (!started) {
		WAIT(&cv, &lock);
	}
	UNLOCK(&lock);

	isc_event_free(&event);
}

static void
pg_event2(isc_task_t *task, isc_event_t *event) {
	isc_boolean_t sender_match = ISC_FALSE;
	isc_boolean_t type_match = ISC_FALSE;
	isc_boolean_t tag_match = ISC_FALSE;

	UNUSED(task);

	if ((purge_sender == NULL) || (purge_sender == event->ev_sender)) {
		sender_match = ISC_TRUE;
	}

	if (testrange) {
		if ((purge_type_first <= event->ev_type) &&
		    (event->ev_type <= purge_type_last))
		{
			type_match = ISC_TRUE;
		}
	} else {
		if (purge_type_first == event->ev_type) {
			type_match = ISC_TRUE;
		}
	}

	if ((purge_tag == NULL) || (purge_tag == event->ev_tag)) {
		tag_match = ISC_TRUE;
	}

	if (sender_match && type_match && tag_match) {
		if (event->ev_attributes & ISC_EVENTATTR_NOPURGE) {
			printf("event %p,%d,%p matched but was not purgeable\n",
			       event->ev_sender, (int)event->ev_type,
			       event->ev_tag);
			++eventcnt;
		} else {
			printf("*** event %p,%d,%p not purged\n",
			       event->ev_sender, (int)event->ev_type,
			       event->ev_tag);
		}
	} else {
		++eventcnt;
	}

	isc_event_free(&event);
}

static void
pg_sde(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	done = ISC_TRUE;
	SIGNAL(&cv);
	UNLOCK(&lock);

	isc_event_free(&event);
}

static void
test_purge(int sender, int type, int tag, int exp_purged) {
	isc_result_t result;
	isc_task_t *task = NULL;
	isc_event_t *eventtab[NEVENTS];
	isc_event_t *event = NULL;
	isc_interval_t interval;
	isc_time_t now;
	int sender_cnt, type_cnt, tag_cnt, event_cnt, i;
	int purged = 0;

	started = ISC_FALSE;
	done = ISC_FALSE;
	eventcnt = 0;

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_init(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, pg_sde, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Block the task on cv.
	 */
	event = isc_event_allocate(mctx, (void *)1, 9999,
				   pg_event1, NULL, sizeof(*event));

	ATF_REQUIRE(event != NULL);
	isc_task_send(task, &event);

	/*
	 * Fill the task's queue with some messages with varying
	 * sender, type, tag, and purgeable attribute values.
	 */
	event_cnt = 0;
	for (sender_cnt = 0; sender_cnt < SENDERCNT; ++sender_cnt) {
		for (type_cnt = 0; type_cnt < TYPECNT; ++type_cnt) {
			for (tag_cnt = 0; tag_cnt < TAGCNT; ++tag_cnt) {
				eventtab[event_cnt] =
					isc_event_allocate(mctx,
					    &senders[sender + sender_cnt],
					    (isc_eventtype_t)(type + type_cnt),
					    pg_event2, NULL, sizeof(*event));

				ATF_REQUIRE(eventtab[event_cnt] != NULL);

				eventtab[event_cnt]->ev_tag =
					(void *)((uintptr_t)tag + tag_cnt);

				/*
				 * Mark events as non-purgeable if
				 * sender, type and tag are all
				 * odd-numbered. (There should be 4
				 * of these out of 60 events total.)
				 */
				if (((sender_cnt % 2) != 0) &&
				    ((type_cnt % 2) != 0) &&
				    ((tag_cnt % 2) != 0))
				{
					eventtab[event_cnt]->ev_attributes |=
						ISC_EVENTATTR_NOPURGE;
				}
				++event_cnt;
			}
		}
	}

	for (i = 0; i < event_cnt; ++i) {
		isc_task_send(task, &eventtab[i]);
	}

	if (testrange) {
		/*
		 * We're testing isc_task_purgerange.
		 */
		purged  = isc_task_purgerange(task, purge_sender,
					      (isc_eventtype_t)purge_type_first,
					      (isc_eventtype_t)purge_type_last,
					      purge_tag);
		ATF_CHECK_EQ(purged, exp_purged);
	} else {
		/*
		 * We're testing isc_task_purge.
		 */
		printf("purge events %p,%u,%p\n",
		       purge_sender, purge_type_first, purge_tag);
		purged = isc_task_purge(task, purge_sender,
					(isc_eventtype_t)purge_type_first,
					purge_tag);
		printf("purged %d expected %d\n", purged, exp_purged);
		ATF_CHECK_EQ(purged, exp_purged);
	}

	/*
	 * Unblock the task, allowing event processing.
	 */
	LOCK(&lock);
	started = ISC_TRUE;
	SIGNAL(&cv);

	isc_task_shutdown(task);

	isc_interval_set(&interval, 5, 0);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (!done) {
		result = isc_time_nowplusinterval(&now, &interval);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		WAITUNTIL(&cv, &lock, &now);
	}

	UNLOCK(&lock);

	isc_task_detach(&task);

	isc_test_end();
	DESTROYLOCK(&lock);
	(void) isc_condition_destroy(&cv);

	ATF_CHECK_EQ(eventcnt, event_cnt - exp_purged);
}

/*
 * Purge test:
 * A call to isc_task_purge(task, sender, type, tag) purges all events of
 * type 'type' and with tag 'tag' not marked as unpurgeable from sender
 * from the task's " queue and returns the number of events purged.
 */
ATF_TC(purge);
ATF_TC_HEAD(purge, tc) {
	atf_tc_set_md_var(tc, "descr", "purge");
}
ATF_TC_BODY(purge, tc) {
	/* Try purging on a specific sender. */
	printf("testing purge on 2,4,8 expecting 1\n");
	purge_sender = &senders[2];
	purge_type_first = 4;
	purge_type_last = 4;
	purge_tag = (void *)8;
	testrange = ISC_FALSE;
	test_purge(1, 4, 7, 1);

	/* Try purging on all senders. */
	printf("testing purge on 0,4,8 expecting 3\n");
	purge_sender = NULL;
	purge_type_first = 4;
	purge_type_last = 4;
	purge_tag = (void *)8;
	testrange = ISC_FALSE;
	test_purge(1, 4, 7, 3);

	/* Try purging on all senders, specified type, all tags. */
	printf("testing purge on 0,4,0 expecting 15\n");
	purge_sender = NULL;
	purge_type_first = 4;
	purge_type_last = 4;
	purge_tag = NULL;
	testrange = ISC_FALSE;
	test_purge(1, 4, 7, 15);

	/* Try purging on a specified tag, no such type. */
	printf("testing purge on 0,99,8 expecting 0\n");
	purge_sender = NULL;
	purge_type_first = 99;
	purge_type_last = 99;
	purge_tag = (void *)8;
	testrange = ISC_FALSE;
	test_purge(1, 4, 7, 0);

	/*
	 * Try purging on specified sender, type, all tags.
	 */
	printf("testing purge on 3,5,0 expecting 5\n");
	purge_sender = &senders[3];
	purge_type_first = 5;
	purge_type_last = 5;
	purge_tag = NULL;
	testrange = ISC_FALSE;
	test_purge(1, 4, 7, 5);
}

/*
 * Purge range test:
 * A call to isc_event_purgerange(task, sender, first, last, tag) purges
 * all events not marked unpurgeable from sender 'sender' and of type within
 * the range 'first' to 'last' inclusive from the task's event queue and
 * returns the number of tasks purged.
 */

ATF_TC(purgerange);
ATF_TC_HEAD(purgerange, tc) {
	atf_tc_set_md_var(tc, "descr", "purge-range");
}
ATF_TC_BODY(purgerange, tc) {
	/* Now let's try some ranges. */
	printf("testing purgerange on 2,4-5,8 expecting 1\n");
	purge_sender = &senders[2];
	purge_type_first = 4;
	purge_type_last = 5;
	purge_tag = (void *)8;
	testrange = ISC_TRUE;
	test_purge(1, 4, 7, 1);

	/* Try purging on all senders. */
	printf("testing purge on 0,4-5,8 expecting 5\n");
	purge_sender = NULL;
	purge_type_first = 4;
	purge_type_last = 5;
	purge_tag = (void *)8;
	testrange = ISC_TRUE;
	test_purge(1, 4, 7, 5);

	/* Try purging on all senders, specified type, all tags. */
	printf("testing purge on 0,5-6,0 expecting 28\n");
	purge_sender = NULL;
	purge_type_first = 5;
	purge_type_last = 6;
	purge_tag = NULL;
	testrange = ISC_TRUE;
	test_purge(1, 4, 7, 28);

	/* Try purging on a specified tag, no such type. */
	printf("testing purge on 0,99-101,8 expecting 0\n");
	purge_sender = NULL;
	purge_type_first = 99;
	purge_type_last = 101;
	purge_tag = (void *)8;
	testrange = ISC_TRUE;
	test_purge(1, 4, 7, 0);

	/* Try purging on specified sender, type, all tags. */
	printf("testing purge on 3,5-6,0 expecting 10\n");
	purge_sender = &senders[3];
	purge_type_first = 5;
	purge_type_last = 6;
	purge_tag = NULL;
	testrange = ISC_TRUE;
	test_purge(1, 4, 7, 10);
}

/*
 * Helpers for purge event tests
 */
static void
pge_event1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	while (!started) {
		WAIT(&cv, &lock);
	}
	UNLOCK(&lock);

	isc_event_free(&event);
}

static void
pge_event2(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	++eventcnt;
	isc_event_free(&event);
}


static void
pge_sde(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	done = ISC_TRUE;
	SIGNAL(&cv);
	UNLOCK(&lock);

	isc_event_free(&event);
}

static void
try_purgeevent(isc_boolean_t purgeable) {
	isc_result_t	result;
	isc_task_t *task = NULL;
	isc_boolean_t purged;
	isc_event_t *event1 = NULL;
	isc_event_t *event2 = NULL;
	isc_event_t *event2_clone = NULL;;
	isc_time_t now;
	isc_interval_t interval;

	started = ISC_FALSE;
	done = ISC_FALSE;
	eventcnt = 0;

	result = isc_mutex_init(&lock);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_condition_init(&cv);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_test_begin(NULL, ISC_TRUE, 2);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_create(taskmgr, 0, &task);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, pge_sde, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	/*
	 * Block the task on cv.
	 */
	event1 = isc_event_allocate(mctx, (void *)1, (isc_eventtype_t)1,
				    pge_event1, NULL, sizeof(*event1));
	ATF_REQUIRE(event1 != NULL);
	isc_task_send(task, &event1);

	event2 = isc_event_allocate(mctx, (void *)1, (isc_eventtype_t)1,
				    pge_event2, NULL, sizeof(*event2));
	ATF_REQUIRE(event2 != NULL);

	event2_clone = event2;

	if (purgeable) {
		event2->ev_attributes &= ~ISC_EVENTATTR_NOPURGE;
	} else {
		event2->ev_attributes |= ISC_EVENTATTR_NOPURGE;
	}

	isc_task_send(task, &event2);

	purged = isc_task_purgeevent(task, event2_clone);
	ATF_CHECK_EQ(purgeable, purged);

	/*
	 * Unblock the task, allowing event processing.
	 */
	LOCK(&lock);
	started = ISC_TRUE;
	SIGNAL(&cv);

	isc_task_shutdown(task);

	isc_interval_set(&interval, 5, 0);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (!done) {
		result = isc_time_nowplusinterval(&now, &interval);
		ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

		WAITUNTIL(&cv, &lock, &now);
	}

	UNLOCK(&lock);

	isc_task_detach(&task);

	isc_test_end();
	DESTROYLOCK(&lock);
	(void) isc_condition_destroy(&cv);

	ATF_REQUIRE_EQ(eventcnt, (purgeable ? 0 : 1));
}

/*
 * Purge event test:
 * When the event is marked as purgeable, a call to
 * isc_task_purgeevent(task, event) purges the event 'event' from the
 * task's queue and returns ISC_TRUE.
 */

ATF_TC(purgeevent);
ATF_TC_HEAD(purgeevent, tc) {
	atf_tc_set_md_var(tc, "descr", "purge-event");
}
ATF_TC_BODY(purgeevent, tc) {
	try_purgeevent(ISC_TRUE);
}

/*
 * Purge event not purgeable test:
 * When the event is not marked as purgable, a call to
 * isc_task_purgeevent(task, event) does not purge the event
 * 'event' from the task's queue and returns ISC_FALSE.
 */

ATF_TC(purgeevent_notpurge);
ATF_TC_HEAD(purgeevent_notpurge, tc) {
	atf_tc_set_md_var(tc, "descr", "purge-event");
}
ATF_TC_BODY(purgeevent_notpurge, tc) {
	try_purgeevent(ISC_FALSE);
}
#endif

/*
 * Main
 */
ATF_TP_ADD_TCS(tp) {
	ATF_TP_ADD_TC(tp, create_task);
	ATF_TP_ADD_TC(tp, all_events);
	ATF_TP_ADD_TC(tp, privileged_events);
	ATF_TP_ADD_TC(tp, privilege_drop);
	ATF_TP_ADD_TC(tp, basic);
	ATF_TP_ADD_TC(tp, task_exclusive);

#ifdef ISC_PLATFORM_USETHREADS
	ATF_TP_ADD_TC(tp, manytasks);
	ATF_TP_ADD_TC(tp, shutdown);
	ATF_TP_ADD_TC(tp, post_shutdown);
	ATF_TP_ADD_TC(tp, purge);
	ATF_TP_ADD_TC(tp, purgerange);
	ATF_TP_ADD_TC(tp, purgeevent);
	ATF_TP_ADD_TC(tp, purgeevent_notpurge);
#endif

	return (atf_no_error());
}

