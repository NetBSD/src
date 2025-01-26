/*	$NetBSD: work.h,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#include <stdlib.h>

#include <isc/lang.h>
#include <isc/loop.h>

typedef void (*isc_work_cb)(void *arg);
typedef void (*isc_after_work_cb)(void *arg);
typedef struct isc_work isc_work_t;

ISC_LANG_BEGINDECLS

void
isc_work_enqueue(isc_loop_t *loop, isc_work_cb work_cb,
		 isc_after_work_cb after_work_cb, void *cbarg);
/*%<
 * Schedules work to be handled by the libuv thread pool (see uv_work_t).
 * The function specified in `work_cb` will be run by a thread in the
 * thread pool; when complete, the `after_work_cb` function will run
 * in 'loop' to inform the caller that the work was completed.
 *
 * Requires:
 * \li 'loop' is a valid event loop.
 * \li 'work_cb' and 'after_work_cb' are not NULL.
 */

ISC_LANG_ENDDECLS
