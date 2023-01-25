/*	$NetBSD: app.c,v 1.8 2023/01/25 21:43:31 christos Exp $	*/

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
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef WIN32
#include <inttypes.h>
#include <signal.h>
#include <sys/time.h>
#endif /* WIN32 */

#include <isc/app.h>
#include <isc/atomic.h>
#include <isc/condition.h>
#include <isc/event.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/platform.h>
#include <isc/strerr.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/time.h>
#include <isc/util.h>

#ifdef WIN32
#include <process.h>
#else /* WIN32 */
#include <pthread.h>
#endif /* WIN32 */

/*%
 * For BIND9 internal applications built with threads, we use a single app
 * context and let multiple worker, I/O, timer threads do actual jobs.
 */

static isc_thread_t blockedthread;
static atomic_bool is_running = 0;

#ifdef WIN32
/*
 * We need to remember which thread is the main thread...
 */
static isc_thread_t main_thread;
#endif /* ifdef WIN32 */

/*
 * The application context of this module.
 */
#define APPCTX_MAGIC	ISC_MAGIC('A', 'p', 'c', 'x')
#define VALID_APPCTX(c) ISC_MAGIC_VALID(c, APPCTX_MAGIC)

#ifdef WIN32
#define NUM_EVENTS 2

enum { RELOAD_EVENT, SHUTDOWN_EVENT };
#endif /* WIN32 */

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
#ifdef WIN32
	HANDLE hEvents[NUM_EVENTS];
#else  /* WIN32 */
	isc_mutex_t readylock;
	isc_condition_t ready;
#endif /* WIN32 */
};

static isc_appctx_t isc_g_appctx;

#ifndef WIN32
static void
handle_signal(int sig, void (*handler)(int)) {
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;

	if (sigfillset(&sa.sa_mask) != 0 || sigaction(sig, &sa, NULL) < 0) {
		char strbuf[ISC_STRERRORSIZE];
		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_error_fatal(__FILE__, __LINE__,
				"handle_signal() %d setup: %s", sig, strbuf);
	}
}
#endif /* ifndef WIN32 */

isc_result_t
isc_app_ctxstart(isc_appctx_t *ctx) {
	REQUIRE(VALID_APPCTX(ctx));

	/*
	 * Start an ISC library application.
	 */

	isc_mutex_init(&ctx->lock);

#ifndef WIN32
	isc_mutex_init(&ctx->readylock);
	isc_condition_init(&ctx->ready);
#endif /* WIN32 */

	ISC_LIST_INIT(ctx->on_run);

	atomic_init(&ctx->shutdown_requested, false);
	atomic_init(&ctx->running, false);
	atomic_init(&ctx->want_shutdown, false);
	atomic_init(&ctx->want_reload, false);
	atomic_init(&ctx->blocked, false);

#ifdef WIN32
	main_thread = GetCurrentThread();

	/* Create the reload event in a non-signaled state */
	ctx->hEvents[RELOAD_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);

	/* Create the shutdown event in a non-signaled state */
	ctx->hEvents[SHUTDOWN_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
#else /* WIN32 */
	int presult;
	sigset_t sset;
	char strbuf[ISC_STRERRORSIZE];

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
	if (isc_bind9) {

	if (sigemptyset(&sset) != 0 || sigaddset(&sset, SIGHUP) != 0 ||
	    sigaddset(&sset, SIGINT) != 0 || sigaddset(&sset, SIGTERM) != 0)
	{
		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_error_fatal(__FILE__, __LINE__,
				"isc_app_start() sigsetops: %s", strbuf);
	}
	presult = pthread_sigmask(SIG_BLOCK, &sset, NULL);
	if (presult != 0) {
		strerror_r(presult, strbuf, sizeof(strbuf));
		isc_error_fatal(__FILE__, __LINE__,
				"isc_app_start() pthread_sigmask: %s", strbuf);
	}

	}

#endif /* WIN32 */

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

#ifdef WIN32
	REQUIRE(main_thread == GetCurrentThread());
#endif /* ifdef WIN32 */

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

#ifndef WIN32
	/*
	 * BIND9 internal tools using multiple contexts do not
	 * rely on signal. */
	if (isc_bind9 && ctx != &isc_g_appctx) {
		return (ISC_R_SUCCESS);
	}
#endif /* WIN32 */

	/*
	 * There is no danger if isc_app_shutdown() is called before we
	 * wait for signals.  Signals are blocked, so any such signal will
	 * simply be made pending and we will get it when we call
	 * sigwait().
	 */
	while (!atomic_load_acquire(&ctx->want_shutdown)) {
#ifdef WIN32
		DWORD dwWaitResult = WaitForMultipleObjects(
			NUM_EVENTS, ctx->hEvents, FALSE, INFINITE);

		/* See why we returned */

		if (WaitSucceeded(dwWaitResult, NUM_EVENTS)) {
			/*
			 * The return was due to one of the events
			 * being signaled
			 */
			switch (WaitSucceededIndex(dwWaitResult)) {
			case RELOAD_EVENT:
				atomic_store_release(&ctx->want_reload, true);

				break;

			case SHUTDOWN_EVENT:
				atomic_store_release(&ctx->want_shutdown, true);
				break;
			}
		}
#else  /* WIN32 */
		if (isc_bind9) {
			sigset_t sset;
			int sig;
			/*
			 * BIND9 internal; single context:
			 * Wait for SIGHUP, SIGINT, or SIGTERM.
			 */
			if (sigemptyset(&sset) != 0 ||
			    sigaddset(&sset, SIGHUP) != 0 ||
			    sigaddset(&sset, SIGINT) != 0 ||
			    sigaddset(&sset, SIGTERM) != 0)
			{
				char strbuf[ISC_STRERRORSIZE];
				strerror_r(errno, strbuf, sizeof(strbuf));
				isc_error_fatal(__FILE__, __LINE__,
						"isc_app_run() sigsetops: %s",
						strbuf);
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
			 * External, or BIND9 using multiple contexts:
			 * wait until woken up.
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
#endif /* WIN32 */
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

	REQUIRE(atomic_compare_exchange_strong_acq_rel(&is_running,
						       &(bool){ false }, true));
	result = isc_app_ctxrun(&isc_g_appctx);
	atomic_store_release(&is_running, false);

	return (result);
}

bool
isc_app_isrunning() {
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
#ifdef WIN32
		SetEvent(ctx->hEvents[SHUTDOWN_EVENT]);
#else  /* WIN32 */
		if (isc_bind9 && ctx != &isc_g_appctx) {
			/* BIND9 internal, but using multiple contexts */
			atomic_store_release(&ctx->want_shutdown, true);
		} else if (isc_bind9) {
			/* BIND9 internal, single context */
			if (kill(getpid(), SIGTERM) < 0) {
				char strbuf[ISC_STRERRORSIZE];
				strerror_r(errno, strbuf, sizeof(strbuf));
				isc_error_fatal(__FILE__, __LINE__,
						"isc_app_shutdown() "
						"kill: %s",
						strbuf);
			}
		} else {
			/* External, multiple contexts */
			atomic_store_release(&ctx->want_shutdown, true);
			SIGNAL(&ctx->ready);
		}
#endif /* WIN32 */
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
#ifdef WIN32
		SetEvent(ctx->hEvents[RELOAD_EVENT]);
#else  /* WIN32 */
		if (isc_bind9 && ctx != &isc_g_appctx) {
			/* BIND9 internal, but using multiple contexts */
			atomic_store_release(&ctx->want_reload, true);
		} else if (isc_bind9) {
			/* BIND9 internal, single context */
			if (kill(getpid(), SIGHUP) < 0) {
				char strbuf[ISC_STRERRORSIZE];
				strerror_r(errno, strbuf, sizeof(strbuf));
				isc_error_fatal(__FILE__, __LINE__,
						"isc_app_reload() "
						"kill: %s",
						strbuf);
			}
		} else {
			/* External, multiple contexts */
			atomic_store_release(&ctx->want_reload, true);
			SIGNAL(&ctx->ready);
		}
#endif /* WIN32 */
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
#ifndef WIN32
	isc_mutex_destroy(&ctx->readylock);
	isc_condition_destroy(&ctx->ready);
#endif /* WIN32 */
}

void
isc_app_finish(void) {
	isc_app_ctxfinish(&isc_g_appctx);
}

void
isc_app_block(void) {
	REQUIRE(atomic_load_acquire(&isc_g_appctx.running));
	REQUIRE(atomic_compare_exchange_strong_acq_rel(&isc_g_appctx.blocked,
						       &(bool){ false }, true));

#ifdef WIN32
	blockedthread = GetCurrentThread();
#else  /* WIN32 */
	sigset_t sset;
	blockedthread = pthread_self();
	RUNTIME_CHECK(sigemptyset(&sset) == 0 &&
		      sigaddset(&sset, SIGINT) == 0 &&
		      sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(pthread_sigmask(SIG_UNBLOCK, &sset, NULL) == 0);
#endif /* WIN32 */
}

void
isc_app_unblock(void) {
	REQUIRE(atomic_load_acquire(&isc_g_appctx.running));
	REQUIRE(atomic_compare_exchange_strong_acq_rel(&isc_g_appctx.blocked,
						       &(bool){ true }, false));

#ifdef WIN32
	REQUIRE(blockedthread == GetCurrentThread());
#else  /* WIN32 */
	REQUIRE(blockedthread == pthread_self());

	sigset_t sset;
	RUNTIME_CHECK(sigemptyset(&sset) == 0 &&
		      sigaddset(&sset, SIGINT) == 0 &&
		      sigaddset(&sset, SIGTERM) == 0);
	RUNTIME_CHECK(pthread_sigmask(SIG_BLOCK, &sset, NULL) == 0);
#endif /* WIN32 */
}

isc_result_t
isc_appctx_create(isc_mem_t *mctx, isc_appctx_t **ctxp) {
	isc_appctx_t *ctx;

	REQUIRE(mctx != NULL);
	REQUIRE(ctxp != NULL && *ctxp == NULL);

	ctx = isc_mem_get(mctx, sizeof(*ctx));

	ctx->magic = APPCTX_MAGIC;

	ctx->mctx = NULL;
	isc_mem_attach(mctx, &ctx->mctx);

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
