/*	$NetBSD: timer_test.c,v 1.8 2023/06/26 22:02:59 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

isc_mem_t *mctx1, *mctx2, *mctx3;
isc_task_t *t1, *t2, *t3;
isc_timer_t *ti1, *ti2, *ti3;
int tick_count = 0;

static void
shutdown_task(isc_task_t *task, isc_event_t *event) {
	char *name = event->ev_arg;

	printf("task %p shutdown %s\n", task, name);
	isc_event_free(&event);
}

static void
tick(isc_task_t *task, isc_event_t *event) {
	char *name = event->ev_arg;

	INSIST(event->ev_type == ISC_TIMEREVENT_TICK);

	printf("task %s (%p) tick\n", name, task);

	tick_count++;
	if (ti3 != NULL && tick_count % 3 == 0) {
		isc_timer_touch(ti3);
	}

	if (ti3 != NULL && tick_count == 7) {
		isc_time_t expires;
		isc_interval_t interval;

		isc_interval_set(&interval, 5, 0);
		(void)isc_time_nowplusinterval(&expires, &interval);
		isc_interval_set(&interval, 4, 0);
		printf("*** resetting ti3 ***\n");
		RUNTIME_CHECK(isc_timer_reset(ti3, isc_timertype_once, &expires,
					      &interval,
					      true) == ISC_R_SUCCESS);
	}

	isc_event_free(&event);
}

static void
timeout(isc_task_t *task, isc_event_t *event) {
	char *name = event->ev_arg;
	const char *type;

	INSIST(event->ev_type == ISC_TIMEREVENT_IDLE ||
	       event->ev_type == ISC_TIMEREVENT_LIFE);

	if (event->ev_type == ISC_TIMEREVENT_IDLE) {
		type = "idle";
	} else {
		type = "life";
	}
	printf("task %s (%p) %s timeout\n", name, task, type);

	if (strcmp(name, "3") == 0) {
		printf("*** saving task 3 ***\n");
		isc_event_free(&event);
		return;
	}

	isc_event_free(&event);
	isc_task_shutdown(task);
}

static char one[] = "1";
static char two[] = "2";
static char three[] = "3";

int
main(int argc, char *argv[]) {
	isc_nm_t *netmgr = NULL;
	isc_taskmgr_t *taskmgr = NULL;
	isc_timermgr_t *timgr = NULL;
	unsigned int workers;
	isc_time_t expires, now;
	isc_interval_t interval;

	if (argc > 1) {
		workers = atoi(argv[1]);
		if (workers < 1) {
			workers = 1;
		}
		if (workers > 8192) {
			workers = 8192;
		}
	} else {
		workers = 2;
	}
	printf("%u workers\n", workers);

	isc_mem_create(&mctx1);
	RUNTIME_CHECK(isc_managers_create(mctx1, workers, 0, &netmgr,
					  &taskmgr) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_timermgr_create(mctx1, &timgr) == ISC_R_SUCCESS);

	RUNTIME_CHECK(isc_task_create(taskmgr, 0, &t1) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_create(taskmgr, 0, &t2) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_create(taskmgr, 0, &t3) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_onshutdown(t1, shutdown_task, one) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_onshutdown(t2, shutdown_task, two) ==
		      ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_onshutdown(t3, shutdown_task, three) ==
		      ISC_R_SUCCESS);

	printf("task 1: %p\n", t1);
	printf("task 2: %p\n", t2);
	printf("task 3: %p\n", t3);

	TIME_NOW(&now);

	isc_interval_set(&interval, 2, 0);
	RUNTIME_CHECK(isc_timer_create(timgr, isc_timertype_once, NULL,
				       &interval, t2, timeout, two,
				       &ti2) == ISC_R_SUCCESS);

	isc_interval_set(&interval, 1, 0);
	RUNTIME_CHECK(isc_timer_create(timgr, isc_timertype_ticker, NULL,
				       &interval, t1, tick, one,
				       &ti1) == ISC_R_SUCCESS);

	isc_interval_set(&interval, 10, 0);
	RUNTIME_CHECK(isc_time_add(&now, &interval, &expires) == ISC_R_SUCCESS);
	isc_interval_set(&interval, 2, 0);
	RUNTIME_CHECK(isc_timer_create(timgr, isc_timertype_once, &expires,
				       &interval, t3, timeout, three,
				       &ti3) == ISC_R_SUCCESS);

	isc_task_detach(&t1);
	isc_task_detach(&t2);
	isc_task_detach(&t3);

#ifndef WIN32
	sleep(15);
#else  /* ifndef WIN32 */
	Sleep(15000);
#endif /* ifndef WIN32 */
	printf("destroy\n");
	isc_timer_destroy(&ti1);
	isc_timer_destroy(&ti2);
	isc_timer_destroy(&ti3);
#ifndef WIN32
	sleep(2);
#else  /* ifndef WIN32 */
	Sleep(2000);
#endif /* ifndef WIN32 */
	isc_timermgr_destroy(&timgr);
	isc_managers_destroy(&netmgr, &taskmgr);
	printf("destroyed\n");

	printf("Statistics for mctx1:\n");
	isc_mem_stats(mctx1, stdout);
	isc_mem_destroy(&mctx1);

	return (0);
}
