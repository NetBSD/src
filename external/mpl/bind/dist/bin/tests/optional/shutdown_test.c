/*	$NetBSD: shutdown_test.c,v 1.8 2023/06/26 22:02:59 christos Exp $	*/

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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <isc/app.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/timer.h>
#include <isc/util.h>

typedef struct {
	isc_mem_t *mctx;
	isc_task_t *task;
	isc_timer_t *timer;
	unsigned int ticks;
	char name[16];
	bool exiting;
	isc_task_t *peer;
} t_info;

#define MAX_TASKS	3
#define T2_SHUTDOWNOK	(ISC_EVENTCLASS(1024) + 0)
#define T2_SHUTDOWNDONE (ISC_EVENTCLASS(1024) + 1)
#define FOO_EVENT	(ISC_EVENTCLASS(1024) + 2)

static t_info tasks[MAX_TASKS];
static unsigned int task_count;
static isc_nm_t *netmgr = NULL;
static isc_taskmgr_t *taskmgr = NULL;
static isc_timermgr_t *timer_manager;

static void
t1_shutdown(isc_task_t *task, isc_event_t *event) {
	t_info *info = event->ev_arg;

	printf("task %s (%p) t1_shutdown\n", info->name, task);
	isc_task_detach(&info->task);
	isc_event_free(&event);
}

static void
t2_shutdown(isc_task_t *task, isc_event_t *event) {
	t_info *info = event->ev_arg;

	printf("task %s (%p) t2_shutdown\n", info->name, task);
	info->exiting = true;
	isc_event_free(&event);
}

static void
shutdown_action(isc_task_t *task, isc_event_t *event) {
	t_info *info = event->ev_arg;
	isc_event_t *nevent;

	INSIST(event->ev_type == ISC_TASKEVENT_SHUTDOWN);

	printf("task %s (%p) shutdown\n", info->name, task);
	if (strcmp(info->name, "0") == 0) {
		isc_timer_destroy(&info->timer);
		nevent = isc_event_allocate(info->mctx, info, T2_SHUTDOWNOK,
					    t2_shutdown, &tasks[1],
					    sizeof(*event));
		RUNTIME_CHECK(nevent != NULL);
		info->exiting = true;
		isc_task_sendanddetach(&info->peer, &nevent);
	}
	isc_event_free(&event);
}

static void
foo_event(isc_task_t *task, isc_event_t *event) {
	printf("task(%p) foo\n", task);
	isc_event_free(&event);
}

static void
tick(isc_task_t *task, isc_event_t *event) {
	t_info *info = event->ev_arg;
	isc_event_t *nevent;

	INSIST(event->ev_type == ISC_TIMEREVENT_TICK);

	printf("task %s (%p) tick\n", info->name, task);

	info->ticks++;
	if (strcmp(info->name, "1") == 0) {
		if (info->ticks == 10) {
			isc_app_shutdown();
		} else if (info->ticks >= 15 && info->exiting) {
			isc_timer_destroy(&info->timer);
			isc_task_detach(&info->task);
			nevent = isc_event_allocate(
				info->mctx, info, T2_SHUTDOWNDONE, t1_shutdown,
				&tasks[0], sizeof(*event));
			RUNTIME_CHECK(nevent != NULL);
			isc_task_send(info->peer, &nevent);
			isc_task_detach(&info->peer);
		}
	} else if (strcmp(info->name, "foo") == 0) {
		isc_timer_destroy(&info->timer);
		nevent = isc_event_allocate(info->mctx, info, FOO_EVENT,
					    foo_event, task, sizeof(*event));
		RUNTIME_CHECK(nevent != NULL);
		isc_task_sendanddetach(&task, &nevent);
	}

	isc_event_free(&event);
}

static t_info *
new_task(isc_mem_t *mctx, const char *name) {
	t_info *ti;
	isc_time_t expires;
	isc_interval_t interval;

	RUNTIME_CHECK(task_count < MAX_TASKS);
	ti = &tasks[task_count];
	ti->mctx = mctx;
	ti->task = NULL;
	ti->timer = NULL;
	ti->ticks = 0;
	if (name != NULL) {
		INSIST(strlen(name) < sizeof(ti->name));
		strlcpy(ti->name, name, sizeof(ti->name));
	} else {
		snprintf(ti->name, sizeof(ti->name), "%u", task_count);
	}
	RUNTIME_CHECK(isc_task_create(taskmgr, 0, &ti->task) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_task_onshutdown(ti->task, shutdown_action, ti) ==
		      ISC_R_SUCCESS);

	isc_time_settoepoch(&expires);
	isc_interval_set(&interval, 1, 0);
	RUNTIME_CHECK(isc_timer_create(timer_manager, isc_timertype_ticker,
				       &expires, &interval, ti->task, tick, ti,
				       &ti->timer) == ISC_R_SUCCESS);

	task_count++;

	return (ti);
}

int
main(int argc, char *argv[]) {
	unsigned int workers;
	t_info *t1, *t2 = NULL;
	isc_task_t *task = NULL;
	isc_mem_t *mctx = NULL, *mctx2 = NULL;

	RUNTIME_CHECK(isc_app_start() == ISC_R_SUCCESS);

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

	isc_mem_create(&mctx);
	isc_mem_create(&mctx2);
	RUNTIME_CHECK(isc_managers_create(mctx, workers, 0, &netmgr,
					  &taskmgr) == ISC_R_SUCCESS);
	RUNTIME_CHECK(isc_timermgr_create(mctx, &timer_manager) ==
		      ISC_R_SUCCESS);

	t1 = new_task(mctx, NULL);
	t2 = new_task(mctx2, NULL);
	isc_task_attach(t2->task, &t1->peer);
	isc_task_attach(t1->task, &t2->peer);

	/*
	 * Test run-triggered shutdown.
	 */
	(void)new_task(mctx2, "foo");

	/*
	 * Test implicit shutdown.
	 */
	RUNTIME_CHECK(isc_task_create(taskmgr, 0, &task) == ISC_R_SUCCESS);
	isc_task_detach(&task);

	/*
	 * Test anti-zombie code.
	 */
	RUNTIME_CHECK(isc_task_create(taskmgr, 0, &task) == ISC_R_SUCCESS);
	isc_task_detach(&task);

	RUNTIME_CHECK(isc_app_run() == ISC_R_SUCCESS);

	isc_managers_destroy(&netmgr, &taskmgr);
	isc_timermgr_destroy(&timer_manager);

	printf("Statistics for mctx:\n");
	isc_mem_stats(mctx, stdout);
	isc_mem_destroy(&mctx);
	printf("Statistics for mctx2:\n");
	isc_mem_stats(mctx2, stdout);
	isc_mem_destroy(&mctx2);

	isc_app_finish();

	return (0);
}
