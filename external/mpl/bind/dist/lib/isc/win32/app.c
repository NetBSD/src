/*	$NetBSD: app.c,v 1.4 2019/02/24 20:01:32 christos Exp $	*/

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

#include <sys/types.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <process.h>

#include <isc/app.h>
#include <isc/condition.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/event.h>
#include <isc/platform.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/time.h>
#include <isc/util.h>
#include <isc/thread.h>

/*%
 * For BIND9 internal applications built with threads, we use a single app
 * context and let multiple worker, I/O, timer threads do actual jobs.
 */

static isc_thread_t	blockedthread;
static bool	is_running;

#define APPCTX_MAGIC		ISC_MAGIC('A', 'p', 'c', 'x')
#define VALID_APPCTX(c)		ISC_MAGIC_VALID(c, APPCTX_MAGIC)

/* Events to wait for */

#define NUM_EVENTS 2

enum {
	RELOAD_EVENT,
	SHUTDOWN_EVENT
};

typedef struct isc__appctx {
	isc_appctx_t		common;
	isc_mem_t		*mctx;
	isc_eventlist_t		on_run;
	isc_mutex_t		lock;
	bool		shutdown_requested;
	bool		running;
	/*
	 * We assume that 'want_shutdown' can be read and written atomically.
	 */
	bool		want_shutdown;
	/*
	 * We assume that 'want_reload' can be read and written atomically.
	 */
	bool		want_reload;

	bool		blocked;

	HANDLE			hEvents[NUM_EVENTS];

	isc_taskmgr_t		*taskmgr;
	isc_socketmgr_t		*socketmgr;
	isc_timermgr_t		*timermgr;
} isc__appctx_t;

static isc__appctx_t isc_g_appctx;

/*
 * We need to remember which thread is the main thread...
 */
static isc_thread_t	main_thread;

isc_result_t
isc_app_ctxstart(isc_appctx_t *ctx0) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;
	isc_result_t result;

	REQUIRE(VALID_APPCTX(ctx));

	/*
	 * Start an ISC library application.
	 */

	main_thread = GetCurrentThread();

	isc_mutex_init(&ctx->lock);

	ctx->shutdown_requested = false;
	ctx->running = false;
	ctx->want_shutdown = false;
	ctx->want_reload = false;
	ctx->blocked  = false;

	/* Create the reload event in a non-signaled state */
	ctx->hEvents[RELOAD_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);

	/* Create the shutdown event in a non-signaled state */
	ctx->hEvents[SHUTDOWN_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);

	ISC_LIST_INIT(ctx->on_run);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_start(void) {
	isc_g_appctx.common.impmagic = APPCTX_MAGIC;
	isc_g_appctx.common.magic = ISCAPI_APPCTX_MAGIC;
	isc_g_appctx.mctx = NULL;
	/* The remaining members will be initialized in ctxstart() */

	return (isc_app_ctxstart((isc_appctx_t *)&isc_g_appctx));
}

isc_result_t
isc_app_onrun(isc_mem_t *mctx, isc_task_t *task, isc_taskaction_t action,
	       void *arg)
{
	return (isc_app_ctxonrun((isc_appctx_t *)&isc_g_appctx, mctx,
				  task, action, arg));
}

isc_result_t
isc_app_ctxonrun(isc_appctx_t *ctx0, isc_mem_t *mctx, isc_task_t *task,
		  isc_taskaction_t action, void *arg)
{
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;
	isc_event_t *event;
	isc_task_t *cloned_task = NULL;
	isc_result_t result;

	LOCK(&ctx->lock);

	if (ctx->running) {
		result = ISC_R_ALREADYRUNNING;
		goto unlock;
	}

	/*
	 * Note that we store the task to which we're going to send the event
	 * in the event's "sender" field.
	 */
	isc_task_attach(task, &cloned_task);
	event = isc_event_allocate(mctx, cloned_task, ISC_APPEVENT_SHUTDOWN,
				   action, arg, sizeof(*event));
	if (event == NULL) {
		result = ISC_R_NOMEMORY;
		goto unlock;
	}

	ISC_LINK_INIT(event, ev_link);
	ISC_LIST_APPEND(ctx->on_run, event, ev_link);

	result = ISC_R_SUCCESS;

 unlock:
	UNLOCK(&ctx->lock);

	return (result);
}

isc_result_t
isc_app_ctxrun(isc_appctx_t *ctx0) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;
	isc_event_t *event, *next_event;
	isc_task_t *task;
	HANDLE *pHandles = NULL;
	DWORD  dwWaitResult;

	REQUIRE(VALID_APPCTX(ctx));

	REQUIRE(main_thread == GetCurrentThread());

	LOCK(&ctx->lock);

	if (!ctx->running) {
		ctx->running = true;

		/*
		 * Post any on-run events (in FIFO order).
		 */
		for (event = ISC_LIST_HEAD(ctx->on_run);
		     event != NULL;
		     event = next_event) {
			next_event = ISC_LIST_NEXT(event, ev_link);
			ISC_LIST_UNLINK(ctx->on_run, event, ev_link);
			task = event->ev_sender;
			event->ev_sender = NULL;
			isc_task_sendanddetach(&task, &event);
		}

	}

	UNLOCK(&ctx->lock);

	/*
	 * There is no danger if isc_app_shutdown() is called before we wait
	 * for events.
	 */

	while (!ctx->want_shutdown) {
		dwWaitResult = WaitForMultipleObjects(NUM_EVENTS, ctx->hEvents,
						      FALSE, INFINITE);

		/* See why we returned */

		if (WaitSucceeded(dwWaitResult, NUM_EVENTS)) {
			/*
			 * The return was due to one of the events
			 * being signaled
			 */
			switch (WaitSucceededIndex(dwWaitResult)) {
			case RELOAD_EVENT:
				ctx->want_reload = true;
				break;

			case SHUTDOWN_EVENT:
				ctx->want_shutdown = true;
				break;
			}
		}

		if (ctx->want_reload) {
			ctx->want_reload = false;
			return (ISC_R_RELOAD);
		}

		if (ctx->want_shutdown && ctx->blocked)
			exit(-1);
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_run(void) {
	isc_result_t result;

	is_running = true;
	result = isc_app_ctxrun((isc_appctx_t *)&isc_g_appctx);
	is_running = false;

	return (result);
}

bool
isc_app_isrunning() {
	return (is_running);
}

isc_result_t
isc_app_ctxshutdown(isc_appctx_t *ctx0) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;
	bool want_kill = true;

	REQUIRE(VALID_APPCTX(ctx));

	LOCK(&ctx->lock);

	REQUIRE(ctx->running);

	if (ctx->shutdown_requested)
		want_kill = false;		/* We're only signaling once */
	else
		ctx->shutdown_requested = true;

	UNLOCK(&ctx->lock);

	if (want_kill)
		SetEvent(ctx->hEvents[SHUTDOWN_EVENT]);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_shutdown(void) {
	return (isc_app_ctxshutdown((isc_appctx_t *)&isc_g_appctx));
}

isc_result_t
isc_app_ctxsuspend(isc_appctx_t *ctx0) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;
	bool want_kill = true;

	REQUIRE(VALID_APPCTX(ctx));

	LOCK(&ctx->lock);

	REQUIRE(ctx->running);

	/*
	 * Don't send the reload signal if we're shutting down.
	 */
	if (ctx->shutdown_requested)
		want_kill = false;

	UNLOCK(&ctx->lock);

	if (want_kill)
		SetEvent(ctx->hEvents[RELOAD_EVENT]);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_reload(void) {
	return (isc_app_ctxsuspend((isc_appctx_t *)&isc_g_appctx));
}

void
isc_app_ctxfinish(isc_appctx_t *ctx0) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;

	REQUIRE(VALID_APPCTX(ctx));

	isc_mutex_destroy(&ctx->lock);
}

void
isc_app_finish(void) {
	isc_app_ctxfinish((isc_appctx_t *)&isc_g_appctx);
}

void
isc_app_block(void) {
	REQUIRE(isc_g_appctx.running);
	REQUIRE(!isc_g_appctx.blocked);

	isc_g_appctx.blocked = true;
	blockedthread = GetCurrentThread();
}

void
isc_app_unblock(void) {
	REQUIRE(isc_g_appctx.running);
	REQUIRE(isc_g_appctx.blocked);

	isc_g_appctx.blocked = false;
	REQUIRE(blockedthread == GetCurrentThread());
}

isc_result_t
isc_appctx_create(isc_mem_t *mctx, isc_appctx_t **ctxp) {
	isc__appctx_t *ctx;

	REQUIRE(mctx != NULL);
	REQUIRE(ctxp != NULL && *ctxp == NULL);

	ctx = isc_mem_get(mctx, sizeof(*ctx));
	if (ctx == NULL)
		return (ISC_R_NOMEMORY);

	ctx->common.impmagic = APPCTX_MAGIC;
	ctx->common.magic = ISCAPI_APPCTX_MAGIC;

	ctx->mctx = NULL;
	isc_mem_attach(mctx, &ctx->mctx);

	ctx->taskmgr = NULL;
	ctx->socketmgr = NULL;
	ctx->timermgr = NULL;

	*ctxp = (isc_appctx_t *)ctx;

	return (ISC_R_SUCCESS);
}

void
isc_appctx_destroy(isc_appctx_t **ctxp) {
	isc__appctx_t *ctx;

	REQUIRE(ctxp != NULL);
	ctx = (isc__appctx_t *)*ctxp;
	REQUIRE(VALID_APPCTX(ctx));

	isc_mem_putanddetach(&ctx->mctx, ctx, sizeof(*ctx));

	*ctxp = NULL;
}

void
isc_appctx_settaskmgr(isc_appctx_t *ctx0, isc_taskmgr_t *taskmgr) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;

	REQUIRE(VALID_APPCTX(ctx));

	ctx->taskmgr = taskmgr;
}

void
isc_appctx_setsocketmgr(isc_appctx_t *ctx0, isc_socketmgr_t *socketmgr) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;

	REQUIRE(VALID_APPCTX(ctx));

	ctx->socketmgr = socketmgr;
}

void
isc_appctx_settimermgr(isc_appctx_t *ctx0, isc_timermgr_t *timermgr) {
	isc__appctx_t *ctx = (isc__appctx_t *)ctx0;

	REQUIRE(VALID_APPCTX(ctx));

	ctx->timermgr = timermgr;
}
