/*	$NetBSD: task_test.c,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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

#include <config.h>

#include <stdlib.h>
#include <unistd.h>

#include <isc/mem.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

isc_mem_t *mctx = NULL;

static void
my_callback(isc_task_t *task, isc_event_t *event) {
	int i, j;
	char *name = event->ev_arg;

	j = 0;
	for (i = 0; i < 1000000; i++)
		j += 100;
	printf("task %s (%p): %d\n", name, task, j);
	isc_event_free(&event);
}

static void
my_shutdown(isc_task_t *task, isc_event_t *event) {
	char *name = event->ev_arg;

	printf("shutdown %s (%p)\n", name, task);
	isc_event_free(&event);
}

static void
my_tick(isc_task_t *task, isc_event_t *event) {
	char *name = event->ev_arg;

	printf("task %p tick %s\n", task, name);
	isc_event_free(&event);
}

static char one[] = "1";
static char two[] = "2";
static char three[] = "3";
static char four[] = "4";
static char foo[] = "foo";
static char bar[] = "bar";

int
main(int argc, char *argv[]) {
	isc_taskmgr_t *manager = NULL;
	isc_task_t *t1 = NULL, *t2 = NULL;
	isc_task_t *t3 = NULL, *t4 = NULL;
	isc_event_t *event;
	unsigned int workers;
	isc_timermgr_t *timgr;
	isc_timer_t *ti1, *ti2;
	struct isc_interval interval;

	if (argc > 1) {
		workers = atoi(argv[1]);
		if (workers < 1)
			workers = 1;
		if (workers > 8192)
			workers = 8192;
	} else
		workers = 2;
	printf("%u workers\n", workers);

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);

	RUNTIME_CHECK(isc_taskmgr_create(mctx, workers, 0, &manager) ==
		      ISC_R_SUCCESS);

	RUNTIME_CHECK(isc_task_create(manager, 0, &t1) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_create(manager, 0, &t2) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_create(manager, 0, &t3) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_create(manager, 0, &t4) == ISC_R_SUCCESS);

	RUNTIME_CHECK(isc_task_onshutdown(t1, my_shutdown, one) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_onshutdown(t2, my_shutdown, two) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_onshutdown(t3, my_shutdown, three) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_onshutdown(t4, my_shutdown, four) ==
		      ISC_R_SUCCESS);

	timgr = NULL;
	RUNTIME_CHECK(isc_timermgr_create(mctx, &timgr) == ISC_R_SUCCESS);
	ti1 = NULL;

	isc_interval_set(&interval, 1, 0);
	RUNTIME_CHECK(isc_timer_create(timgr, isc_timertype_ticker, NULL,
				       &interval, t1, my_tick, foo, &ti1) ==
		      ISC_R_SUCCESS);

	ti2 = NULL;
	isc_interval_set(&interval, 1, 0);
	RUNTIME_CHECK(isc_timer_create(timgr, isc_timertype_ticker, NULL,
				       &interval, t2, my_tick, bar, &ti2) ==
		      ISC_R_SUCCESS);

	printf("task 1 = %p\n", t1);
	printf("task 2 = %p\n", t2);
#ifndef WIN32
	sleep(2);
#else
	Sleep(2000);
#endif

	/*
	 * Note:  (void *)1 is used as a sender here, since some compilers
	 * don't like casting a function pointer to a (void *).
	 *
	 * In a real use, it is more likely the sender would be a
	 * structure (socket, timer, task, etc) but this is just a test
	 * program.
	 */
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, one,
				   sizeof(*event));
	isc_task_send(t1, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, two,
				   sizeof(*event));
	isc_task_send(t2, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, three,
				   sizeof(*event));
	isc_task_send(t3, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, four,
				   sizeof(*event));
	isc_task_send(t4, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, two,
				   sizeof(*event));
	isc_task_send(t2, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, three,
				   sizeof(*event));
	isc_task_send(t3, &event);
	event = isc_event_allocate(mctx, (void *)1, 1, my_callback, four,
				   sizeof(*event));
	isc_task_send(t4, &event);
	isc_task_purgerange(t3,
			    NULL,
			    ISC_EVENTTYPE_FIRSTEVENT,
			    ISC_EVENTTYPE_LASTEVENT, NULL);

	isc_task_detach(&t1);
	isc_task_detach(&t2);
	isc_task_detach(&t3);
	isc_task_detach(&t4);

#ifndef WIN32
	sleep(10);
#else
	Sleep(10000);
#endif
	printf("destroy\n");
	isc_timer_detach(&ti1);
	isc_timer_detach(&ti2);
	isc_timermgr_destroy(&timgr);
	isc_taskmgr_destroy(&manager);
	printf("destroyed\n");

	isc_mem_stats(mctx, stdout);
	isc_mem_destroy(&mctx);

	return (0);
}
