/*	$NetBSD: loop.h,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#pragma once

#include <inttypes.h>

#include <isc/job.h>
#include <isc/lang.h>
#include <isc/mem.h>
#include <isc/refcount.h>
#include <isc/types.h>

typedef void (*isc_job_cb)(void *);

/* Add -DISC_LOOP_TRACE=1 to CFLAGS for detailed reference tracing */

ISC_LANG_BEGINDECLS

/*%<
 * Returns the current running loop.
 */

extern thread_local isc_loop_t *isc__loop_local;

static inline isc_loop_t *
isc_loop(void) {
	return isc__loop_local;
}
void
isc_loopmgr_create(isc_mem_t *mctx, uint32_t nloops, isc_loopmgr_t **loopmgrp);
/*%<
 * Create a loop manager supporting 'nloops' loops.
 *
 * Requires:
 *\li	'nloops' is greater than 0.
 */

void
isc_loopmgr_destroy(isc_loopmgr_t **loopmgrp);
/*%<
 * Destroy the loop manager pointed to by 'loopmgrp'.
 *
 * Requires:
 *\li	'loopmgr' points to a valid loop manager.
 */

void
isc_loopmgr_shutdown(isc_loopmgr_t *loopmgr);
/*%<
 * Request shutdown of the loop manager 'loopmgr'.
 *
 * This will stop all signal handlers and send shutdown events to
 * all active loops. As a final action on shutting down, each loop
 * will run the function (or functions) set by isc_loopmgr_teardown()
 * or isc_loop_teardown().
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 */

void
isc_loopmgr_run(isc_loopmgr_t *loopmgr);
/*%<
 * Run the loops in 'loopmgr'. Each loop will start by running the
 * function (or functions) set by isc_loopmgr_setup() or isc_loop_setup().
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 */

void
isc_loopmgr_pause(isc_loopmgr_t *loopmgr);
/*%<
 * Send pause events to all running loops in 'loopmgr' except the
 * current one. This can only be called from a running loop.
 * All the paused loops will wait until isc_loopmgr_resume() is
 * run in the calling loop before continuing.
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 *\li	We are in a running loop.
 */

void
isc_loopmgr_resume(isc_loopmgr_t *loopmgr);
/*%<
 * Send resume events to all paused loops in 'loopmgr'. This can
 * only be called by a running loop (which must therefore be the
 * loop that called isc_loopmgr_pause()).
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 *\li	We are in a running loop.
 */

uint32_t
isc_loopmgr_nloops(isc_loopmgr_t *loopmgr);

isc_job_t *
isc_loop_setup(isc_loop_t *loop, isc_job_cb cb, void *cbarg);
isc_job_t *
isc_loop_teardown(isc_loop_t *loop, isc_job_cb cb, void *cbarg);
/*%<
 * Schedule actions to be run when starting, and when shutting down,
 * one of the loops in a loop manager.
 *
 * Requires:
 *\li	'loop' is a valid loop.
 *\li	The loop manager associated with 'loop' is paused or has not
 *	yet been started.
 */

void
isc_loopmgr_setup(isc_loopmgr_t *loopmgr, isc_job_cb cb, void *cbarg);
void
isc_loopmgr_teardown(isc_loopmgr_t *loopmgr, isc_job_cb cb, void *cbarg);
/*%<
 * Schedule actions to be run when starting, and when shutting down,
 * *all* of the loops in loopmgr.
 *
 * This is the same as running isc_loop_setup() or
 * isc_loop_teardown() on each of the loops in turn.
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 *\li	'loopmgr' is paused or has not yet been started.
 */

isc_mem_t *
isc_loop_getmctx(isc_loop_t *loop);
/*%<
 * Return a pointer to the a memory context that was created for
 * 'loop' when it was initialized.
 *
 * Requires:
 *\li	'loop' is a valid loop.
 */

isc_loop_t *
isc_loop_main(isc_loopmgr_t *loopmgr);
/*%<
 * Returns the main loop for the 'loopmgr' (which is 'loopmgr->loops[0]',
 * regardless of how many loops there are).
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 */

isc_loop_t *
isc_loop_get(isc_loopmgr_t *loopmgr, uint32_t tid);
/*%<
 * Return the loop object associated with the 'tid' threadid
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 *\li   'tid' is smaller than number of initialized loops
 */

#

#if ISC_LOOP_TRACE
#define isc_loop_ref(ptr)   isc_loop__ref(ptr, __func__, __FILE__, __LINE__)
#define isc_loop_unref(ptr) isc_loop__unref(ptr, __func__, __FILE__, __LINE__)
#define isc_loop_attach(ptr, ptrp) \
	isc_loop__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define isc_loop_detach(ptrp) \
	isc_loop__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(isc_loop);
#else
ISC_REFCOUNT_DECL(isc_loop);
#endif
/*%<
 * Reference counting functions for isc_loop
 */

void
isc_loopmgr_blocking(isc_loopmgr_t *loopmgr);
void
isc_loopmgr_nonblocking(isc_loopmgr_t *loopmgr);
/*%<
 * isc_loopmgr_blocking() stops the SIGINT and SIGTERM signal handlers
 * during blocking operations, for example while waiting for user
 * interaction; isc_loopmgr_nonblocking() restarts them.
 *
 * Requires:
 *\li	'loopmgr' is a valid loop manager.
 */

isc_loopmgr_t *
isc_loop_getloopmgr(isc_loop_t *loop);
/*%<
 * Return the loopmgr associated with 'loop'.
 *
 * Requires:
 *\li	'loop' is a valid loop.
 */

isc_time_t
isc_loop_now(isc_loop_t *loop);
/*%<
 * Returns the start time of the current loop tick.
 *
 * Requires:
 *
 * \li 'loop' is a valid loop.
 */

bool
isc_loop_shuttingdown(isc_loop_t *loop);
/*%<
 * Returns whether the loop is shutting down.
 *
 * Requires:
 *
 * \li 'loop' is a valid loop and the loop tid matches the current tid.
 */
ISC_LANG_ENDDECLS
