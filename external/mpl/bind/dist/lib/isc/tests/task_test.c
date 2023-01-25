/*	$NetBSD: task_test.c,v 1.11 2023/01/25 21:43:31 christos Exp $	*/

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

#if HAVE_CMOCKA

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING

#include <cmocka.h>

#include <isc/atomic.h>
#include <isc/cmocka.h>
#include <isc/commandline.h>
#include <isc/condition.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/platform.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

#include "isctest.h"

/* Set to true (or use -v option) for verbose output */
static bool verbose = false;

static isc_mutex_t lock;
static isc_condition_t cv;

atomic_int_fast32_t counter;
static int active[10];
static atomic_bool done, done2;

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	isc_mutex_init(&lock);

	isc_condition_init(&cv);

	result = isc_test_begin(NULL, true, 0);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_setup2(void **state) {
	isc_result_t result;

	UNUSED(state);

	isc_mutex_init(&lock);

	isc_condition_init(&cv);

	/* Two worker threads */
	result = isc_test_begin(NULL, true, 2);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_setup4(void **state) {
	isc_result_t result;

	UNUSED(state);

	isc_mutex_init(&lock);

	isc_condition_init(&cv);

	/* Four worker threads */
	result = isc_test_begin(NULL, true, 4);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	isc_test_end();
	isc_condition_destroy(&cv);

	return (0);
}

static void
set(isc_task_t *task, isc_event_t *event) {
	atomic_int_fast32_t *value = (atomic_int_fast32_t *)event->ev_arg;

	UNUSED(task);

	isc_event_free(&event);
	atomic_store(value, atomic_fetch_add(&counter, 1));
}

#include <isc/thread.h>

static void
set_and_drop(isc_task_t *task, isc_event_t *event) {
	atomic_int_fast32_t *value = (atomic_int_fast32_t *)event->ev_arg;

	UNUSED(task);

	isc_event_free(&event);
	LOCK(&lock);
	atomic_store(value, atomic_fetch_add(&counter, 1));
	UNLOCK(&lock);
}

/* Create a task */
static void
create_task(void **state) {
	isc_result_t result;
	isc_task_t *task = NULL;

	UNUSED(state);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_task_destroy(&task);
	assert_null(task);
}

/* Process events */
static void
all_events(void **state) {
	isc_result_t result;
	isc_task_t *task = NULL;
	isc_event_t *event = NULL;
	atomic_int_fast32_t a, b;
	int i = 0;

	UNUSED(state);

	atomic_init(&counter, 1);
	atomic_init(&a, 0);
	atomic_init(&b, 0);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* First event */
	event = isc_event_allocate(test_mctx, task, ISC_TASKEVENT_TEST, set, &a,
				   sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&a), 0);
	isc_task_send(task, &event);

	event = isc_event_allocate(test_mctx, task, ISC_TASKEVENT_TEST, set, &b,
				   sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&b), 0);
	isc_task_send(task, &event);

	while ((atomic_load(&a) == 0 || atomic_load(&b) == 0) && i++ < 5000) {
		isc_test_nap(1000);
	}

	assert_int_not_equal(atomic_load(&a), 0);
	assert_int_not_equal(atomic_load(&b), 0);

	isc_task_destroy(&task);
	assert_null(task);
}

/* Privileged events */
static void
privileged_events(void **state) {
	isc_result_t result;
	isc_task_t *task1 = NULL, *task2 = NULL;
	isc_event_t *event = NULL;
	atomic_int_fast32_t a, b, c, d, e;
	int i = 0;

	UNUSED(state);

	atomic_init(&counter, 1);
	atomic_init(&a, -1);
	atomic_init(&b, -1);
	atomic_init(&c, -1);
	atomic_init(&d, -1);
	atomic_init(&e, -1);

	/*
	 * Pause the net/task manager so we can fill up the work
	 * queue without things happening while we do it.
	 */
	isc_nm_pause(netmgr);
	isc_taskmgr_setmode(taskmgr, isc_taskmgrmode_privileged);

	result = isc_task_create(taskmgr, 0, &task1);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_task_setname(task1, "privileged", NULL);
	assert_false(isc_task_getprivilege(task1));
	isc_task_setprivilege(task1, true);
	assert_true(isc_task_getprivilege(task1));

	result = isc_task_create(taskmgr, 0, &task2);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_task_setname(task2, "normal", NULL);
	assert_false(isc_task_getprivilege(task2));

	/* First event: privileged */
	event = isc_event_allocate(test_mctx, task1, ISC_TASKEVENT_TEST, set,
				   &a, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&a), -1);
	isc_task_send(task1, &event);

	/* Second event: not privileged */
	event = isc_event_allocate(test_mctx, task2, ISC_TASKEVENT_TEST, set,
				   &b, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&b), -1);
	isc_task_send(task2, &event);

	/* Third event: privileged */
	event = isc_event_allocate(test_mctx, task1, ISC_TASKEVENT_TEST, set,
				   &c, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&c), -1);
	isc_task_send(task1, &event);

	/* Fourth event: privileged */
	event = isc_event_allocate(test_mctx, task1, ISC_TASKEVENT_TEST, set,
				   &d, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&d), -1);
	isc_task_send(task1, &event);

	/* Fifth event: not privileged */
	event = isc_event_allocate(test_mctx, task2, ISC_TASKEVENT_TEST, set,
				   &e, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&e), -1);
	isc_task_send(task2, &event);

	isc_nm_resume(netmgr);

	/* We're waiting for *all* variables to be set */
	while ((atomic_load(&a) < 0 || atomic_load(&b) < 0 ||
		atomic_load(&c) < 0 || atomic_load(&d) < 0 ||
		atomic_load(&e) < 0) &&
	       i++ < 5000)
	{
		isc_test_nap(1000);
	}

	/*
	 * We can't guarantee what order the events fire, but
	 * we do know the privileged tasks that set a, c, and d
	 * would have fired first.
	 */
	assert_true(atomic_load(&a) <= 3);
	assert_true(atomic_load(&c) <= 3);
	assert_true(atomic_load(&d) <= 3);

	/* ...and the non-privileged tasks that set b and e, last */
	assert_true(atomic_load(&b) > 3);
	assert_true(atomic_load(&e) > 3);

	assert_int_equal(atomic_load(&counter), 6);

	isc_task_setprivilege(task1, false);
	assert_false(isc_task_getprivilege(task1));

	isc_task_destroy(&task1);
	assert_null(task1);
	isc_task_destroy(&task2);
	assert_null(task2);
}

/*
 * Edge case: this tests that the task manager behaves as expected when
 * we explicitly set it into normal mode *while* running privileged.
 */
static void
privilege_drop(void **state) {
	isc_result_t result;
	isc_task_t *task1 = NULL, *task2 = NULL;
	isc_event_t *event = NULL;
	atomic_int_fast32_t a, b, c, d, e; /* non valid states */
	int i = 0;

	UNUSED(state);

	atomic_init(&counter, 1);
	atomic_init(&a, -1);
	atomic_init(&b, -1);
	atomic_init(&c, -1);
	atomic_init(&d, -1);
	atomic_init(&e, -1);

	/*
	 * Pause the net/task manager so we can fill up the work queue
	 * without things happening while we do it.
	 */
	isc_nm_pause(netmgr);
	isc_taskmgr_setmode(taskmgr, isc_taskmgrmode_privileged);

	result = isc_task_create(taskmgr, 0, &task1);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_task_setname(task1, "privileged", NULL);
	assert_false(isc_task_getprivilege(task1));
	isc_task_setprivilege(task1, true);
	assert_true(isc_task_getprivilege(task1));

	result = isc_task_create(taskmgr, 0, &task2);
	assert_int_equal(result, ISC_R_SUCCESS);
	isc_task_setname(task2, "normal", NULL);
	assert_false(isc_task_getprivilege(task2));

	/* First event: privileged */
	event = isc_event_allocate(test_mctx, task1, ISC_TASKEVENT_TEST,
				   set_and_drop, &a, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&a), -1);
	isc_task_send(task1, &event);

	/* Second event: not privileged */
	event = isc_event_allocate(test_mctx, task2, ISC_TASKEVENT_TEST,
				   set_and_drop, &b, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&b), -1);
	isc_task_send(task2, &event);

	/* Third event: privileged */
	event = isc_event_allocate(test_mctx, task1, ISC_TASKEVENT_TEST,
				   set_and_drop, &c, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&c), -1);
	isc_task_send(task1, &event);

	/* Fourth event: privileged */
	event = isc_event_allocate(test_mctx, task1, ISC_TASKEVENT_TEST,
				   set_and_drop, &d, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&d), -1);
	isc_task_send(task1, &event);

	/* Fifth event: not privileged */
	event = isc_event_allocate(test_mctx, task2, ISC_TASKEVENT_TEST,
				   set_and_drop, &e, sizeof(isc_event_t));
	assert_non_null(event);

	assert_int_equal(atomic_load(&e), -1);
	isc_task_send(task2, &event);

	isc_nm_resume(netmgr);

	/* We're waiting for all variables to be set. */
	while ((atomic_load(&a) == -1 || atomic_load(&b) == -1 ||
		atomic_load(&c) == -1 || atomic_load(&d) == -1 ||
		atomic_load(&e) == -1) &&
	       i++ < 5000)
	{
		isc_test_nap(1000);
	}

	/*
	 * We need to check that all privilege mode events were fired
	 * in privileged mode, and non privileged in non-privileged.
	 */
	assert_true(atomic_load(&a) <= 3);
	assert_true(atomic_load(&c) <= 3);
	assert_true(atomic_load(&d) <= 3);

	/* ...and neither of the non-privileged tasks did... */
	assert_true(atomic_load(&b) > 3);
	assert_true(atomic_load(&e) > 3);

	/* ...but all five of them did run. */
	assert_int_equal(atomic_load(&counter), 6);

	isc_task_destroy(&task1);
	assert_null(task1);
	isc_task_destroy(&task2);
	assert_null(task2);
}

static void
sleep_cb(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);
	int p = *(int *)event->ev_arg;
	if (p == 1) {
		/*
		 * Signal the main thread that we're running, so that
		 * it can trigger the race.
		 */
		LOCK(&lock);
		atomic_store(&done2, true);
		SIGNAL(&cv);
		UNLOCK(&lock);
		/*
		 * Wait for the operations in the main thread to be finished.
		 */
		LOCK(&lock);
		while (!atomic_load(&done)) {
			WAIT(&cv, &lock);
		}
		UNLOCK(&lock);
	} else {
		/*
		 * Wait for the operations in the main thread to be finished.
		 */
		LOCK(&lock);
		atomic_store(&done2, true);
		SIGNAL(&cv);
		UNLOCK(&lock);
	}
	isc_event_free(&event);
}

static void
pause_unpause(void **state) {
	isc_result_t result;
	isc_task_t *task = NULL;
	isc_event_t *event1, *event2 = NULL;
	UNUSED(state);
	atomic_store(&done, false);
	atomic_store(&done2, false);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	event1 = isc_event_allocate(test_mctx, task, ISC_TASKEVENT_TEST,
				    sleep_cb, &(int){ 1 }, sizeof(isc_event_t));
	assert_non_null(event1);
	event2 = isc_event_allocate(test_mctx, task, ISC_TASKEVENT_TEST,
				    sleep_cb, &(int){ 2 }, sizeof(isc_event_t));
	assert_non_null(event2);
	isc_task_send(task, &event1);
	isc_task_send(task, &event2);
	/* Wait for event1 to be running */
	LOCK(&lock);
	while (!atomic_load(&done2)) {
		WAIT(&cv, &lock);
	}
	UNLOCK(&lock);
	/* Pause-unpause-detach is what causes the race */
	isc_task_pause(task);
	isc_task_unpause(task);
	isc_task_detach(&task);
	/* Signal event1 to finish */
	LOCK(&lock);
	atomic_store(&done2, false);
	atomic_store(&done, true);
	SIGNAL(&cv);
	UNLOCK(&lock);
	/* Wait for event2 to finish */
	LOCK(&lock);
	while (!atomic_load(&done2)) {
		WAIT(&cv, &lock);
	}
	UNLOCK(&lock);
}

/*
 * Basic task functions:
 */
static void
basic_cb(isc_task_t *task, isc_event_t *event) {
	int i, j;

	UNUSED(task);

	j = 0;
	for (i = 0; i < 1000000; i++) {
		j += 100;
	}

	UNUSED(j);

	if (verbose) {
		print_message("# task %s\n", (char *)event->ev_arg);
	}

	isc_event_free(&event);
}

static void
basic_shutdown(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	if (verbose) {
		print_message("# shutdown %s\n", (char *)event->ev_arg);
	}

	isc_event_free(&event);
}

static void
basic_tick(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	if (verbose) {
		print_message("# %s\n", (char *)event->ev_arg);
	}

	isc_event_free(&event);
}

static char one[] = "1";
static char two[] = "2";
static char three[] = "3";
static char four[] = "4";
static char tick[] = "tick";
static char tock[] = "tock";

static void
basic(void **state) {
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
	char *testarray[] = { one, one, one,   one,  one, one,	 one,  one,
			      one, two, three, four, two, three, four, NULL };
	int i;

	UNUSED(state);

	result = isc_task_create(taskmgr, 0, &task1);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_task_create(taskmgr, 0, &task2);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_task_create(taskmgr, 0, &task3);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_task_create(taskmgr, 0, &task4);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task1, basic_shutdown, one);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_task_onshutdown(task2, basic_shutdown, two);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_task_onshutdown(task3, basic_shutdown, three);
	assert_int_equal(result, ISC_R_SUCCESS);
	result = isc_task_onshutdown(task4, basic_shutdown, four);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_time_settoepoch(&absolute);
	isc_interval_set(&interval, 1, 0);
	result = isc_timer_create(timermgr, isc_timertype_ticker, &absolute,
				  &interval, task1, basic_tick, tick, &ti1);
	assert_int_equal(result, ISC_R_SUCCESS);

	ti2 = NULL;
	isc_time_settoepoch(&absolute);
	isc_interval_set(&interval, 1, 0);
	result = isc_timer_create(timermgr, isc_timertype_ticker, &absolute,
				  &interval, task2, basic_tick, tock, &ti2);
	assert_int_equal(result, ISC_R_SUCCESS);

#ifndef WIN32
	sleep(2);
#else  /* ifndef WIN32 */
	Sleep(2000);
#endif /* ifndef WIN32 */

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
		event = isc_event_allocate(test_mctx, (void *)1, 1, basic_cb,
					   testarray[i], sizeof(*event));
		assert_non_null(event);
		isc_task_send(task1, &event);
	}

	(void)isc_task_purge(task3, NULL, 0, 0);

	isc_task_detach(&task1);
	isc_task_detach(&task2);
	isc_task_detach(&task3);
	isc_task_detach(&task4);

#ifndef WIN32
	sleep(10);
#else  /* ifndef WIN32 */
	Sleep(10000);
#endif /* ifndef WIN32 */
	isc_timer_detach(&ti1);
	isc_timer_detach(&ti2);
}

/*
 * Exclusive mode test:
 * When one task enters exclusive mode, all other active
 * tasks complete first.
 */
static int
spin(int n) {
	int i;
	int r = 0;
	for (i = 0; i < n; i++) {
		r += i;
		if (r > 1000000) {
			r = 0;
		}
	}
	return (r);
}

static void
exclusive_cb(isc_task_t *task, isc_event_t *event) {
	int taskno = *(int *)(event->ev_arg);

	if (verbose) {
		print_message("# task enter %d\n", taskno);
	}

	/* task chosen from the middle of the range */
	if (taskno == 6) {
		isc_result_t result;
		int i;

		result = isc_task_beginexclusive(task);
		assert_int_equal(result, ISC_R_SUCCESS);

		for (i = 0; i < 10; i++) {
			assert_int_equal(active[i], 0);
		}

		isc_task_endexclusive(task);
		atomic_store(&done, true);
	} else {
		active[taskno]++;
		(void)spin(10000000);
		active[taskno]--;
	}

	if (verbose) {
		print_message("# task exit %d\n", taskno);
	}

	if (atomic_load(&done)) {
		isc_mem_put(event->ev_destroy_arg, event->ev_arg, sizeof(int));
		isc_event_free(&event);
		atomic_fetch_sub(&counter, 1);
	} else {
		isc_task_send(task, &event);
	}
}

static void
task_exclusive(void **state) {
	isc_task_t *tasks[10];
	isc_result_t result;
	int i;

	UNUSED(state);

	atomic_init(&counter, 0);

	for (i = 0; i < 10; i++) {
		isc_event_t *event = NULL;
		int *v;

		tasks[i] = NULL;

		if (i == 6) {
			/* task chosen from the middle of the range */
			result = isc_task_create_bound(taskmgr, 0, &tasks[i],
						       0);
			assert_int_equal(result, ISC_R_SUCCESS);

			isc_taskmgr_setexcltask(taskmgr, tasks[6]);
		} else {
			result = isc_task_create(taskmgr, 0, &tasks[i]);
			assert_int_equal(result, ISC_R_SUCCESS);
		}

		v = isc_mem_get(test_mctx, sizeof *v);
		assert_non_null(v);

		*v = i;

		event = isc_event_allocate(test_mctx, NULL, 1, exclusive_cb, v,
					   sizeof(*event));
		assert_non_null(event);

		isc_task_send(tasks[i], &event);
		atomic_fetch_add(&counter, 1);
	}

	for (i = 0; i < 10; i++) {
		isc_task_detach(&tasks[i]);
	}

	while (atomic_load(&counter) > 0) {
		isc_test_nap(1000);
	}
}

/*
 * Max tasks test:
 * The task system can create and execute many tasks. Tests with 10000.
 */
static void
maxtask_shutdown(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	if (event->ev_arg != NULL) {
		isc_task_destroy((isc_task_t **)&event->ev_arg);
	} else {
		LOCK(&lock);
		atomic_store(&done, true);
		SIGNAL(&cv);
		UNLOCK(&lock);
	}

	isc_event_free(&event);
}

static void
maxtask_cb(isc_task_t *task, isc_event_t *event) {
	isc_result_t result;

	if (event->ev_arg != NULL) {
		isc_task_t *newtask = NULL;

		event->ev_arg = (void *)(((uintptr_t)event->ev_arg) - 1);

		/*
		 * Create a new task and forward the message.
		 */
		result = isc_task_create(taskmgr, 0, &newtask);
		assert_int_equal(result, ISC_R_SUCCESS);

		result = isc_task_onshutdown(newtask, maxtask_shutdown,
					     (void *)task);
		assert_int_equal(result, ISC_R_SUCCESS);

		isc_task_send(newtask, &event);
	} else if (task != NULL) {
		isc_task_destroy(&task);
		isc_event_free(&event);
	}
}

static void
manytasks(void **state) {
	isc_mem_t *mctx = NULL;
	isc_event_t *event = NULL;
	uintptr_t ntasks = 10000;

	UNUSED(state);

	if (verbose) {
		print_message("# Testing with %lu tasks\n",
			      (unsigned long)ntasks);
	}

	isc_mutex_init(&lock);
	isc_condition_init(&cv);

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;
	isc_mem_create(&mctx);

	isc_managers_create(mctx, 4, 0, &netmgr, &taskmgr);

	atomic_init(&done, false);

	event = isc_event_allocate(mctx, (void *)1, 1, maxtask_cb,
				   (void *)ntasks, sizeof(*event));
	assert_non_null(event);

	LOCK(&lock);
	maxtask_cb(NULL, event);
	while (!atomic_load(&done)) {
		WAIT(&cv, &lock);
	}
	UNLOCK(&lock);

	isc_managers_destroy(&netmgr, &taskmgr);

	isc_mem_destroy(&mctx);
	isc_condition_destroy(&cv);
	isc_mutex_destroy(&lock);
}

/*
 * Shutdown test:
 * When isc_task_shutdown() is called, shutdown events are posted
 * in LIFO order.
 */

static int nevents = 0;
static int nsdevents = 0;
static int senders[4];
atomic_bool ready, all_done;

static void
sd_sde1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	assert_int_equal(nevents, 256);
	assert_int_equal(nsdevents, 1);
	++nsdevents;

	if (verbose) {
		print_message("# shutdown 1\n");
	}

	isc_event_free(&event);

	atomic_store(&all_done, true);
}

static void
sd_sde2(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	assert_int_equal(nevents, 256);
	assert_int_equal(nsdevents, 0);
	++nsdevents;

	if (verbose) {
		print_message("# shutdown 2\n");
	}

	isc_event_free(&event);
}

static void
sd_event1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	while (!atomic_load(&ready)) {
		WAIT(&cv, &lock);
	}
	UNLOCK(&lock);

	if (verbose) {
		print_message("# event 1\n");
	}

	isc_event_free(&event);
}

static void
sd_event2(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	++nevents;

	if (verbose) {
		print_message("# event 2\n");
	}

	isc_event_free(&event);
}

static void
task_shutdown(void **state) {
	isc_result_t result;
	isc_eventtype_t event_type;
	isc_event_t *event = NULL;
	isc_task_t *task = NULL;
	int i;

	UNUSED(state);

	nevents = nsdevents = 0;
	event_type = 3;
	atomic_init(&ready, false);
	atomic_init(&all_done, false);

	LOCK(&lock);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * This event causes the task to wait on cv.
	 */
	event = isc_event_allocate(test_mctx, &senders[1], event_type,
				   sd_event1, NULL, sizeof(*event));
	assert_non_null(event);
	isc_task_send(task, &event);

	/*
	 * Now we fill up the task's event queue with some events.
	 */
	for (i = 0; i < 256; ++i) {
		event = isc_event_allocate(test_mctx, &senders[1], event_type,
					   sd_event2, NULL, sizeof(*event));
		assert_non_null(event);
		isc_task_send(task, &event);
	}

	/*
	 * Now we register two shutdown events.
	 */
	result = isc_task_onshutdown(task, sd_sde1, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, sd_sde2, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_task_shutdown(task);
	isc_task_detach(&task);

	/*
	 * Now we free the task by signaling cv.
	 */
	atomic_store(&ready, true);
	SIGNAL(&cv);
	UNLOCK(&lock);

	while (!atomic_load(&all_done)) {
		isc_test_nap(1000);
	}

	assert_int_equal(nsdevents, 2);
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

	while (!atomic_load(&done)) {
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

static void
post_shutdown(void **state) {
	isc_result_t result;
	isc_eventtype_t event_type;
	isc_event_t *event;
	isc_task_t *task;

	UNUSED(state);

	atomic_init(&done, false);
	event_type = 4;

	isc_condition_init(&cv);

	LOCK(&lock);

	task = NULL;
	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * This event causes the task to wait on cv.
	 */
	event = isc_event_allocate(test_mctx, &senders[1], event_type,
				   psd_event1, NULL, sizeof(*event));
	assert_non_null(event);
	isc_task_send(task, &event);

	isc_task_shutdown(task);

	result = isc_task_onshutdown(task, psd_sde, NULL);
	assert_int_equal(result, ISC_R_SHUTTINGDOWN);

	/*
	 * Release the task.
	 */
	atomic_store(&done, true);

	SIGNAL(&cv);
	UNLOCK(&lock);

	isc_task_detach(&task);
}

/*
 * Helper for the purge tests below:
 */

#define SENDERCNT 3
#define TYPECNT	  4
#define TAGCNT	  5
#define NEVENTS	  (SENDERCNT * TYPECNT * TAGCNT)

static bool testrange;
static void *purge_sender;
static isc_eventtype_t purge_type_first;
static isc_eventtype_t purge_type_last;
static void *purge_tag;
static int eventcnt;

atomic_bool started;

static void
pg_event1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	while (!atomic_load(&started)) {
		WAIT(&cv, &lock);
	}
	UNLOCK(&lock);

	isc_event_free(&event);
}

static void
pg_event2(isc_task_t *task, isc_event_t *event) {
	bool sender_match = false;
	bool type_match = false;
	bool tag_match = false;

	UNUSED(task);

	if ((purge_sender == NULL) || (purge_sender == event->ev_sender)) {
		sender_match = true;
	}

	if (testrange) {
		if ((purge_type_first <= event->ev_type) &&
		    (event->ev_type <= purge_type_last))
		{
			type_match = true;
		}
	} else {
		if (purge_type_first == event->ev_type) {
			type_match = true;
		}
	}

	if ((purge_tag == NULL) || (purge_tag == event->ev_tag)) {
		tag_match = true;
	}

	if (sender_match && type_match && tag_match) {
		if ((event->ev_attributes & ISC_EVENTATTR_NOPURGE) != 0) {
			if (verbose) {
				print_message("# event %p,%d,%p "
					      "matched but was not "
					      "purgeable\n",
					      event->ev_sender,
					      (int)event->ev_type,
					      event->ev_tag);
			}
			++eventcnt;
		} else if (verbose) {
			print_message("# event %p,%d,%p not purged\n",
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
	atomic_store(&done, true);
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

	atomic_init(&started, false);
	atomic_init(&done, false);
	eventcnt = 0;

	isc_condition_init(&cv);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, pg_sde, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Block the task on cv.
	 */
	event = isc_event_allocate(test_mctx, (void *)1, 9999, pg_event1, NULL,
				   sizeof(*event));

	assert_non_null(event);
	isc_task_send(task, &event);

	/*
	 * Fill the task's queue with some messages with varying
	 * sender, type, tag, and purgeable attribute values.
	 */
	event_cnt = 0;
	for (sender_cnt = 0; sender_cnt < SENDERCNT; ++sender_cnt) {
		for (type_cnt = 0; type_cnt < TYPECNT; ++type_cnt) {
			for (tag_cnt = 0; tag_cnt < TAGCNT; ++tag_cnt) {
				eventtab[event_cnt] = isc_event_allocate(
					test_mctx,
					&senders[sender + sender_cnt],
					(isc_eventtype_t)(type + type_cnt),
					pg_event2, NULL, sizeof(*event));

				assert_non_null(eventtab[event_cnt]);

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
		purged = isc_task_purgerange(
			task, purge_sender, (isc_eventtype_t)purge_type_first,
			(isc_eventtype_t)purge_type_last, purge_tag);
		assert_int_equal(purged, exp_purged);
	} else {
		/*
		 * We're testing isc_task_purge.
		 */
		if (verbose) {
			print_message("# purge events %p,%u,%p\n", purge_sender,
				      purge_type_first, purge_tag);
		}
		purged = isc_task_purge(task, purge_sender,
					(isc_eventtype_t)purge_type_first,
					purge_tag);
		if (verbose) {
			print_message("# purged %d expected %d\n", purged,
				      exp_purged);
		}

		assert_int_equal(purged, exp_purged);
	}

	/*
	 * Unblock the task, allowing event processing.
	 */
	LOCK(&lock);
	atomic_store(&started, true);
	SIGNAL(&cv);

	isc_task_shutdown(task);

	isc_interval_set(&interval, 5, 0);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (!atomic_load(&done)) {
		result = isc_time_nowplusinterval(&now, &interval);
		assert_int_equal(result, ISC_R_SUCCESS);

		WAITUNTIL(&cv, &lock, &now);
	}

	UNLOCK(&lock);

	isc_task_detach(&task);

	assert_int_equal(eventcnt, event_cnt - exp_purged);
}

/*
 * Purge test:
 * A call to isc_task_purge(task, sender, type, tag) purges all events of
 * type 'type' and with tag 'tag' not marked as unpurgeable from sender
 * from the task's " queue and returns the number of events purged.
 */
static void
purge(void **state) {
	UNUSED(state);

	/* Try purging on a specific sender. */
	if (verbose) {
		print_message("# testing purge on 2,4,8 expecting 1\n");
	}
	purge_sender = &senders[2];
	purge_type_first = 4;
	purge_type_last = 4;
	purge_tag = (void *)8;
	testrange = false;
	test_purge(1, 4, 7, 1);

	/* Try purging on all senders. */
	if (verbose) {
		print_message("# testing purge on 0,4,8 expecting 3\n");
	}
	purge_sender = NULL;
	purge_type_first = 4;
	purge_type_last = 4;
	purge_tag = (void *)8;
	testrange = false;
	test_purge(1, 4, 7, 3);

	/* Try purging on all senders, specified type, all tags. */
	if (verbose) {
		print_message("# testing purge on 0,4,0 expecting 15\n");
	}
	purge_sender = NULL;
	purge_type_first = 4;
	purge_type_last = 4;
	purge_tag = NULL;
	testrange = false;
	test_purge(1, 4, 7, 15);

	/* Try purging on a specified tag, no such type. */
	if (verbose) {
		print_message("# testing purge on 0,99,8 expecting 0\n");
	}
	purge_sender = NULL;
	purge_type_first = 99;
	purge_type_last = 99;
	purge_tag = (void *)8;
	testrange = false;
	test_purge(1, 4, 7, 0);

	/* Try purging on specified sender, type, all tags. */
	if (verbose) {
		print_message("# testing purge on 3,5,0 expecting 5\n");
	}
	purge_sender = &senders[3];
	purge_type_first = 5;
	purge_type_last = 5;
	purge_tag = NULL;
	testrange = false;
	test_purge(1, 4, 7, 5);
}

/*
 * Purge range test:
 * A call to isc_event_purgerange(task, sender, first, last, tag) purges
 * all events not marked unpurgeable from sender 'sender' and of type within
 * the range 'first' to 'last' inclusive from the task's event queue and
 * returns the number of tasks purged.
 */

static void
purgerange(void **state) {
	UNUSED(state);

	/* Now let's try some ranges. */
	/* testing purgerange on 2,4-5,8 expecting 1 */
	purge_sender = &senders[2];
	purge_type_first = 4;
	purge_type_last = 5;
	purge_tag = (void *)8;
	testrange = true;
	test_purge(1, 4, 7, 1);

	/* Try purging on all senders. */
	if (verbose) {
		print_message("# testing purge on 0,4-5,8 expecting 5\n");
	}
	purge_sender = NULL;
	purge_type_first = 4;
	purge_type_last = 5;
	purge_tag = (void *)8;
	testrange = true;
	test_purge(1, 4, 7, 5);

	/* Try purging on all senders, specified type, all tags. */
	if (verbose) {
		print_message("# testing purge on 0,5-6,0 expecting 28\n");
	}
	purge_sender = NULL;
	purge_type_first = 5;
	purge_type_last = 6;
	purge_tag = NULL;
	testrange = true;
	test_purge(1, 4, 7, 28);

	/* Try purging on a specified tag, no such type. */
	if (verbose) {
		print_message("# testing purge on 0,99-101,8 expecting 0\n");
	}
	purge_sender = NULL;
	purge_type_first = 99;
	purge_type_last = 101;
	purge_tag = (void *)8;
	testrange = true;
	test_purge(1, 4, 7, 0);

	/* Try purging on specified sender, type, all tags. */
	if (verbose) {
		print_message("# testing purge on 3,5-6,0 expecting 10\n");
	}
	purge_sender = &senders[3];
	purge_type_first = 5;
	purge_type_last = 6;
	purge_tag = NULL;
	testrange = true;
	test_purge(1, 4, 7, 10);
}

/*
 * Helpers for purge event tests
 */
static void
pge_event1(isc_task_t *task, isc_event_t *event) {
	UNUSED(task);

	LOCK(&lock);
	while (!atomic_load(&started)) {
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
	atomic_store(&done, true);
	SIGNAL(&cv);
	UNLOCK(&lock);

	isc_event_free(&event);
}

static void
try_purgeevent(bool purgeable) {
	isc_result_t result;
	isc_task_t *task = NULL;
	bool purged;
	isc_event_t *event1 = NULL;
	isc_event_t *event2 = NULL;
	isc_event_t *event2_clone = NULL;
	isc_time_t now;
	isc_interval_t interval;

	atomic_init(&started, false);
	atomic_init(&done, false);
	eventcnt = 0;

	isc_condition_init(&cv);

	result = isc_task_create(taskmgr, 0, &task);
	assert_int_equal(result, ISC_R_SUCCESS);

	result = isc_task_onshutdown(task, pge_sde, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	/*
	 * Block the task on cv.
	 */
	event1 = isc_event_allocate(test_mctx, (void *)1, (isc_eventtype_t)1,
				    pge_event1, NULL, sizeof(*event1));
	assert_non_null(event1);
	isc_task_send(task, &event1);

	event2 = isc_event_allocate(test_mctx, (void *)1, (isc_eventtype_t)1,
				    pge_event2, NULL, sizeof(*event2));
	assert_non_null(event2);

	event2_clone = event2;

	if (purgeable) {
		event2->ev_attributes &= ~ISC_EVENTATTR_NOPURGE;
	} else {
		event2->ev_attributes |= ISC_EVENTATTR_NOPURGE;
	}

	isc_task_send(task, &event2);

	purged = isc_task_purgeevent(task, event2_clone);
	assert_int_equal(purgeable, purged);

	/*
	 * Unblock the task, allowing event processing.
	 */
	LOCK(&lock);
	atomic_store(&started, true);
	SIGNAL(&cv);

	isc_task_shutdown(task);

	isc_interval_set(&interval, 5, 0);

	/*
	 * Wait for shutdown processing to complete.
	 */
	while (!atomic_load(&done)) {
		result = isc_time_nowplusinterval(&now, &interval);
		assert_int_equal(result, ISC_R_SUCCESS);

		WAITUNTIL(&cv, &lock, &now);
	}

	UNLOCK(&lock);

	isc_task_detach(&task);

	assert_int_equal(eventcnt, (purgeable ? 0 : 1));
}

/*
 * Purge event test:
 * When the event is marked as purgeable, a call to
 * isc_task_purgeevent(task, event) purges the event 'event' from the
 * task's queue and returns true.
 */

static void
purgeevent(void **state) {
	UNUSED(state);

	try_purgeevent(true);
}

/*
 * Purge event not purgeable test:
 * When the event is not marked as purgable, a call to
 * isc_task_purgeevent(task, event) does not purge the event
 * 'event' from the task's queue and returns false.
 */

static void
purgeevent_notpurge(void **state) {
	UNUSED(state);

	try_purgeevent(false);
}

int
main(int argc, char **argv) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(manytasks),
		cmocka_unit_test_setup_teardown(all_events, _setup, _teardown),
		cmocka_unit_test_setup_teardown(basic, _setup2, _teardown),
		cmocka_unit_test_setup_teardown(create_task, _setup, _teardown),
		cmocka_unit_test_setup_teardown(pause_unpause, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(post_shutdown, _setup2,
						_teardown),
		cmocka_unit_test_setup_teardown(privilege_drop, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(privileged_events, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(purge, _setup2, _teardown),
		cmocka_unit_test_setup_teardown(purgeevent, _setup2, _teardown),
		cmocka_unit_test_setup_teardown(purgeevent_notpurge, _setup,
						_teardown),
		cmocka_unit_test_setup_teardown(purgerange, _setup, _teardown),
		cmocka_unit_test_setup_teardown(task_shutdown, _setup4,
						_teardown),
		cmocka_unit_test_setup_teardown(task_exclusive, _setup4,
						_teardown),
	};
	struct CMUnitTest selected[sizeof(tests) / sizeof(tests[0])];
	size_t i;
	int c;

	memset(selected, 0, sizeof(selected));

	while ((c = isc_commandline_parse(argc, argv, "lt:v")) != -1) {
		switch (c) {
		case 'l':
			for (i = 0; i < (sizeof(tests) / sizeof(tests[0])); i++)
			{
				if (tests[i].name != NULL) {
					fprintf(stdout, "%s\n", tests[i].name);
				}
			}
			return (0);
		case 't':
			if (!cmocka_add_test_byname(
				    tests, isc_commandline_argument, selected))
			{
				fprintf(stderr, "unknown test '%s'\n",
					isc_commandline_argument);
				exit(1);
			}
			break;
		case 'v':
			verbose = true;
			break;
		default:
			break;
		}
	}

	if (selected[0].name != NULL) {
		return (cmocka_run_group_tests(selected, NULL, NULL));
	} else {
		return (cmocka_run_group_tests(tests, NULL, NULL));
	}
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
