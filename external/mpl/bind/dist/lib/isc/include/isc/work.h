/*	$NetBSD: work.h,v 1.3 2026/08/29 14:55:19 christos Exp $	*/

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

/*! \file isc/work.h
 * \brief Offload work from an event loop onto a dedicated worker thread.
 *
 * Each isc event loop has one worker thread per lane (see isc_worklane_t).
 * isc_work_enqueue() runs a callback on the worker thread bound to the calling
 * loop's lane and, when it finishes, runs a second callback back on that loop
 * with the first callback's result.  The handle it returns can be used to
 * cancel a task that has not started running yet.
 */

#pragma once

#include <isc/lang.h>
#include <isc/mem.h>

typedef enum isc_worklane {
	ISC_WORKLANE_FAST = 0, /*%< short, bounded tasks (e.g. crypto) */
	ISC_WORKLANE_SLOW,     /*%< blocking/long tasks (e.g. zone dump) */
	ISC_WORKLANE_COUNT,
} isc_worklane_t;
/*%<
 * Selects which per-loop worker thread runs an enqueued task.  Keeping long,
 * blocking SLOW work (disk I/O, zone dump/load) on its own lane stops it from
 * holding up short FAST tasks behind it.
 */

typedef isc_result_t (*isc_work_cb)(void *arg);
typedef void (*isc_work_done_cb)(void *arg, isc_result_t result);
typedef struct isc_work isc_work_t;

ISC_LANG_BEGINDECLS

isc_work_t *
isc_work_enqueue(isc_loop_t *loop, isc_worklane_t lane, isc_work_cb cb,
		 isc_work_done_cb done_cb, void *cbarg);
/*%<
 * Schedule 'cb' to run on the worker thread bound to 'loop' and 'lane'.  When
 * 'cb' returns, 'done_cb' is scheduled back on 'loop' with the result of the
 * work: the value 'cb' returned, or ISC_R_CANCELED if the task was canceled
 * before it started (see isc_work_cancel()).
 *
 * Returns a handle that may be passed to isc_work_cancel().  The handle is
 * owned by 'loop' and stays valid until 'done_cb' has run; it must not be used
 * afterwards.
 *
 * Requires:
 *
 *\li	'loop' is a valid isc event loop.
 *\li	'cb' is non-NULL.
 *\li	'done_cb' is non-NULL.
 *\li	'cbarg' is passed to both callbacks, may be NULL.
 */

bool
isc_work_cancel(isc_work_t *work);
/*%<
 * Try to cancel 'work' before its 'cb' starts running.  If the task is still
 * queued it is marked canceled and 'cb' will not run; if it is already running
 * or has finished this has no effect.  Either way the 'done_cb' passed to
 * isc_work_enqueue() still runs on the origin loop, with ISC_R_CANCELED when
 * the cancel succeeded.  Nothing is freed here.
 *
 * Returns:
 *
 *\li	true	the task was still queued; 'cb' will not run.
 *\li	false	the task is already running or done (uv_cancel() semantics).
 *
 * Requires:
 *
 *\li	'work' is a handle from isc_work_enqueue() whose 'done_cb' has not run.
 */

/* private */

typedef struct isc__workthread isc__workthread_t;

isc__workthread_t *
isc__workthread_create(isc_loop_t *loop, isc_worklane_t lane);
/*%<
 * Create one worker thread for 'lane' with its own dispatch queue (the loop
 * manager creates one per loop).  Used by the loop manager; not for general
 * use.
 */

void
isc__workthread_shutdown(isc__workthread_t *thread);
/*%<
 * Begin shutdown of 'thread': set its SHUTDOWN flag (after which new enqueues
 * run inline on the caller instead of queuing), then, after an RCU grace period
 * that fences any in-flight enqueue, wake the worker so it drains its queue and
 * exits.  Does not join the worker; see isc__workthread_destroy().  Idempotent.
 */

void
isc__workthread_pause(isc__workthread_t *thread);
/*%<
 * Quiesce 'thread' for isc_loopmgr_pause(): mark it paused and block until it
 * has parked.  A no-op if the worker is already shutting down.  Must be paired
 * with isc__workthread_resume() and called from the worker's owning loop.
 */

void
isc__workthread_resume(isc__workthread_t *thread);
/*%<
 * Release a worker previously parked by isc__workthread_pause().
 */

void
isc__workthread_destroy(isc__workthread_t **threadp);
/*%<
 * Join the worker thread, then free '*threadp' and set it to NULL.  Must be
 * called after isc__workthread_shutdown().
 */

ISC_LANG_ENDDECLS
