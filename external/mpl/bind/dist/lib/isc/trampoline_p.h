/*	$NetBSD: trampoline_p.h,v 1.1.1.1 2021/04/29 16:46:32 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
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

isc_threadresult_t
isc__trampoline_run(isc_threadarg_t arg);
/*%<
 * Run the thread trampoline, this will get passed to the actual
 * pthread_create() (or Windows equivalent), initialize the isc_tid_v.
 *
 * Requires:
 *\li	'arg' is a valid isc_trampoline_t
 *
 * Returns:
 *\li	return value from start_routine (see isc__trampoline_get())
 */
