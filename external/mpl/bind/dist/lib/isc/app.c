/*	$NetBSD: app.c,v 1.7.2.2 2024/02/25 15:47:15 martin Exp $	*/

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

/*! \file */

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/atomic.h>
#include <isc/condition.h>
#include <isc/event.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/util.h>

/*%
 * For BIND9 applications built with threads, we use a single app
 * context and let multiple taskmgr and netmgr threads do actual jobs.
 */

static isc_thread_t blockedthread;
static atomic_bool is_running = 0;

/*
 * The application context of this module.
 */
#define APPCTX_MAGIC	ISC_MAGIC('A', 'p', 'c', 'x')
#define VALID_APPCTX(c) ISC_MAGIC_VALID(c, APPCTX_MAGIC)

struct isc_appctx {
	unsigned int magic;
	isc_mem_t *mctx;
	isc_mutex_t lock;
	isc_eventlist_t on_run;
	atomic_bool shutdown_requested;
	atomic_bool running;
	atomic_bool want_shutdown;
	atomic_bool want_reload;
	atomic_bool blocked;
	isc_mutex_t readylock;
	isc_condition_t ready;
};

static isc_appctx_t isc_g_appctx;

static void
handle_signal(int sig, void (*handler)(int)) {
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;

	if (sigfillset(&sa.sa_mask) != 0 || sigaction(sig, &sa, NULL) < 0) {
		FATAL_SYSERROR(errno, "signal %d", sig);
	}
}

isc_result_t
isc_app_ctxstart(isc_appctx_t *ctx) {
	int presult;
	sigset_t sset;

	REQUIRE(VALID_APPCTX(ctx));

	/*
	 * Start an ISC library application.
	 */

	isc_mutex_init(&ctx->lock);

	isc_mutex_init(&ctx->readylock);
	isc_condition_init(&ctx->ready);

	ISC_LIST_INIT(ctx->on_run);

	atomic_init(&ctx->shutdown_requested, false);
	atomic_init(&ctx->running, false);
	atomic_init(&ctx->want_shutdown, false);
	atomic_init(&ctx->want_reload, false);
	atomic_init(&ctx->blocked, false);

	/*
	 * Always ignore SIGPIPE.
	 */
	handle_signal(SIGPIPE, SIG_IGN);

	handle_signal(SIGHUP, SIG_DFL);
	handle_signal(SIGTERM, SIG_DFL);
	handle_signal(SIGINT, SIG_DFL);

	/*
	 * Block SIGHUP, SIGINT, SIGTERM.
	 *
	 * If isc_app_start() is called from the main thread before any other
	 * threads have been created, then the pthread_sigmask() call below
	 * will result in all threads having SIGHUP, SIGINT and SIGTERM
	 * blocked by default, ensuring that only the thread that calls
	 * sigwait() for them will get those signals.
	 */
	if (sigemptyset(&sset) != 0 || sigaddset(&sset, SIGHUP) != 0 ||
	    sigaddset(&sset, SIGINT) != 0 || sigaddset(&sset, SIGTERM) != 0)
	{
		FATAL_SYSERROR(errno, "sigsetops");
	}
	presult = pthread_sigmask(SIG_BLOCK, &sset, NULL);
	if (presult != 0) {
		FATAL_SYSERROR(presult, "pthread_sigmask()");
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_start(void) {
	isc_g_appctx.magic = APPCTX_MAGIC;
	isc_g_appctx.mctx = NULL;
	/* The remaining members will be initialized in ctxstart() */

	return (isc_app_ctxstart(&isc_g_appctx));
}

isc_result_t
isc_app_onrun(isc_mem_t *mctx, isc_task_t *task, isc_taskaction_t action,
	      void *arg) {
	return (isc_app_ctxonrun(&isc_g_appctx, mctx, task, action, arg));
}

isc_result_t
isc_app_ctxonrun(isc_appctx_t *ctx, isc_mem_t *mctx, isc_task_t *task,
		 isc_taskaction_t action, void *arg) {
	isc_event_t *event;
	isc_task_t *cloned_task = NULL;

	if (atomic_load_acquire(&ctx->running)) {
		return (ISC_R_ALREADYRUNNING);
	}

	/*
	 * Note that we store the task to which we're going to send the event
	 * in the event's "sender" field.
	 */
	isc_task_attach(task, &cloned_task);
	event = isc_event_allocate(mctx, cloned_task, ISC_APPEVENT_SHUTDOWN,
				   action, arg, sizeof(*event));

	LOCK(&ctx->lock);
	ISC_LINK_INIT(event, ev_link);
	ISC_LIST_APPEND(ctx->on_run, event, ev_link);
	UNLOCK(&ctx->lock);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_ctxrun(isc_appctx_t *ctx) {
	isc_event_t *event, *next_event;
	isc_task_t *task;

	REQUIRE(VALID_APPCTX(ctx));

	if (atomic_compare_exchange_strong_acq_rel(&ctx->running,
						   &(bool){ false }, true))
	{
		/*
		 * Post any on-run events (in FIFO order).
		 */
		LOCK(&ctx->lock);
		for (event = ISC_LIST_HEAD(ctx->on_run); event != NULL;
		     event = next_event)
		{
			next_event = ISC_LIST_NEXT(event, ev_link);
			ISC_LIST_UNLINK(ctx->on_run, event, ev_link);
			task = event->ev_sender;
			event->ev_sender = NULL;
			isc_task_sendanddetach(&task, &event);
		}
		UNLOCK(&ctx->lock);
	}

	/*
	 * There is no danger if isc_app_shutdown() is called before we
	 * wait for signals.  Signals are blocked, so any such signal will
	 * simply be made pending and we will get it when we call
	 * sigwait().
	 */
	while (!atomic_load_acquire(&ctx->want_shutdown)) {
		if (ctx == &isc_g_appctx) {
			sigset_t sset;
			int sig;
			/*
			 * Wait for SIGHUP, SIGINT, or SIGTERM.
			 */
			if (sigemptyset(&sset) != 0 ||
			    sigaddset(&sset, SIGHUP) != 0 ||
			    sigaddset(&sset, SIGINT) != 0 ||
			    sigaddset(&sset, SIGTERM) != 0)
			{
				FATAL_SYSERROR(errno, "sigsetops");
			}

			if (sigwait(&sset, &sig) == 0) {
				switch (sig) {
				case SIGINT:
				case SIGTERM:
					atomic_store_release(
						&ctx->want_shutdown, true);
					break;
				case SIGHUP:
					atomic_store_release(&ctx->want_reload,
							     true);
					break;
				default:
					UNREACHABLE();
				}
			}
		} else {
			/*
			 * Tools using multiple contexts don't
			 * rely on a signal, just wait until woken
			 * up.
			 */
			if (atomic_load_acquire(&ctx->want_shutdown)) {
				break;
			}
			if (!atomic_load_acquire(&ctx->want_reload)) {
				LOCK(&ctx->readylock);
				WAIT(&ctx->ready, &ctx->readylock);
				UNLOCK(&ctx->readylock);
			}
		}
		if (atomic_compare_exchange_strong_acq_rel(
			    &ctx->want_reload, &(bool){ true }, false))
		{
			return (ISC_R_RELOAD);
		}

		if (atomic_load_acquire(&ctx->want_shutdown) &&
		    atomic_load_acquire(&ctx->blocked))
		{
			exit(1);
		}
	}

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_app_run(void) {
	isc_result_t result;

	atomic_compare_exchange_enforced(&is_running, &(bool){ false }, true);

	result = isc_app_ctxrun(&isc_g_appctx);
	atomic_store_release(&is_running, false);

	return (result);
}

bool
isc_app_isrunning(void) {
	return (atomic_load_acquire(&is_running));
}

void
isc_app_ctxshutdown(isc_appctx_t *ctx) {
	REQUIRE(VALID_APPCTX(ctx));

	REQUIRE(atomic_load_acquire(&ctx->running));

	/* If ctx->shutdown_requested == true, we are already shutting
	 * down and we want to just bail out.
	 */
	if (atomic_compare_exchange_strong_acq_rel(&ctx->shutdown_requested,
						   &(bool){ false }, true))
	{
		if (ctx != &isc_g_appctx) {
			/* Tool using multiple contexts */
			atomic_store_release(&ctx->want_shutdown, true);
			SIGNAL(&ctx->ready);
		} else {
			/* Normal single BIND9 context */
			if (kill(getpid(), SIGTERM) < 0) {
				FATAL_SYSERROR(errno, "kill");
			}
		}
	}
}

void
isc_app_shutdown(void) {
	isc_app_ctxshutdown(&isc_g_appctx);
}

void
isc_app_ctxsuspend(isc_appctx_t *ctx) {
	REQUIRE(VALID_APPCTX(ctx));

	REQUIRE(atomic_load(&ctx->running));

	/*
	 * Don't send the reload signal if we're shutting down.
	 */
	if (!atomic_load_acquire(&ctx->shutdown_requested)) {
		if (ctx != &isc_g_appctx) {
			/* Tool using multiple contexts */
			atomic_store_release(&ctx->want_reload, true);
			SIGNAL(&ctx->ready);
		} else {
			/* Normal single BIND9 context */
			if (kill(getpid(), SIGHUP) < 0) {
				FATAL_SYSERROR(errno, "kill");
			}
		}
	}
}

void
isc_app_reload(void) {
	isc_app_ctxsuspend(&isc_g_appctx);
}

void
isc_app_ctxfinish(isc_appctx_t *ctx) {
	REQUIRE(VALID_APPCTX(ctx));

	isc_mutex_destroy(&ctx->lock);
	isc_mutex_destroy(&ctx->readylock);
	isc_condition_destroy(&ctx->ready);
}

void
isc_app_finish(void) {
	isc_app_ctxfinish(&isc_g_appctx);
}

void
isc_app_block(void) {
	sigset_t sset;

	REQUIRE(atomic_load_acquire(&isc_g_appctx.running));

	atomic_compare_exchange_enforced(&isc_g_appctx.blocked,
					 &(bool){ false }, true);

	blockedthread = pthread_self();
	RUNTIME_CHECK(sigemptyset(&sset) == 0 &&
		      sigaddset(&sset, SIGINT) == 0 &&
		      sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(pthread_sigmask(SIG_UNBLOCK, &sset, NULL) == 0);
}

void
isc_app_unblock(void) {
	sigset_t sset;

	REQUIRE(atomic_load_acquire(&isc_g_appctx.running));
	REQUIRE(blockedthread == pthread_self());

	atomic_compare_exchange_enforced(&isc_g_appctx.blocked, &(bool){ true },
					 false);

	RUNTIME_CHECK(sigemptyset(&sset) == 0 &&
		      sigaddset(&sset, SIGINT) == 0 &&
		      sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(pthread_sigmask(SIG_BLOCK, &sset, NULL) == 0);
}

isc_result_t
isc_appctx_create(isc_mem_t *mctx, isc_appctx_t **ctxp) {
	isc_appctx_t *ctx;

	REQUIRE(mctx != NULL);
	REQUIRE(ctxp != NULL && *ctxp == NULL);

	ctx = isc_mem_get(mctx, sizeof(*ctx));
	*ctx = (isc_appctx_t){ .magic = 0 };

	isc_mem_attach(mctx, &ctx->mctx);
	ctx->magic = APPCTX_MAGIC;

	*ctxp = ctx;

	return (ISC_R_SUCCESS);
}

void
isc_appctx_destroy(isc_appctx_t **ctxp) {
	isc_appctx_t *ctx;

	REQUIRE(ctxp != NULL);
	ctx = *ctxp;
	*ctxp = NULL;
	REQUIRE(VALID_APPCTX(ctx));

	ctx->magic = 0;

	isc_mem_putanddetach(&ctx->mctx, ctx, sizeof(*ctx));
}
