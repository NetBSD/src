/*	$NetBSD: trampoline_p.h,v 1.3.2.1 2024/02/25 15:47:19 martin Exp $	*/

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

#include <isc/thread.h>

/*! \file isc/trampoline_p.h
 * \brief isc__trampoline: allows safe reuse of thread ID numbers.
 *
 * The 'isc_hp' hazard pointer API uses an internal thread ID
 * variable ('tid_v') that is incremented for each new thread that uses
 * hazard pointers. This thread ID is then used as an index into a global
 * shared table of hazard pointer state.
 *
 * Since the thread ID is only incremented and never decremented, the
 * table can overflow if threads are frequently created and destroyed.
 *
 * A trampoline is a thin wrapper around any function to be called from
 * a newly launched thread. It maintains a table of thread IDs used by
 * current and previous threads; when a thread is destroyed, its slot in
 * the trampoline table becomes available, and the next thread to occupy
 * that slot can use the same thread ID that its predecessor did.
 *
 * The trampoline table initially has space for 64 worker threads in
 * addition to the main thread. If more threads than that are in
 * concurrent use, the table is reallocated with twice as much space.
 * (Note that the number of concurrent threads is currently capped at
 * 128 by the queue and hazard pointer implementations.)
 */

typedef struct isc__trampoline isc__trampoline_t;

void
isc__trampoline_initialize(void);
/*%<
 * Initialize the thread trampoline internal structures, must be called only
 * once as a library constructor (see lib/isc/lib.c).
 */

void
isc__trampoline_shutdown(void);
/*%<
 * Destroy the thread trampoline internal structures, must be called only
 * once as a library destructor (see lib/isc/lib.c).
 */

isc__trampoline_t *
isc__trampoline_get(isc_threadfunc_t start_routine, isc_threadarg_t arg);
/*%<
 * Get a free thread trampoline structure and initialize it with
 * start_routine and arg passed to start_routine.
 *
 * Requires:
 *\li	'start_routine' is a valid non-NULL thread start_routine
 */

void
isc__trampoline_attach(isc__trampoline_t *trampoline);
void
isc__trampoline_detach(isc__trampoline_t *trampoline);
/*%<
 * Attach/detach the trampoline to/from the current thread.
 *
 * Requires:
 * \li  'trampoline' is a valid isc__trampoline_t
 */

isc_threadresult_t
isc__trampoline_run(isc_threadarg_t arg);
/*%<
 * Run the thread trampoline, this will get passed to the actual
 * pthread_create(), initialize the isc_tid_v.
 *
 * Requires:
 *\li	'arg' is a valid isc_trampoline_t
 *
 * Returns:
 *\li	return value from start_routine (see isc__trampoline_get())
 */
