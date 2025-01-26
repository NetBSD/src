/*	$NetBSD: async.h,v 1.2 2025/01/26 16:25:40 christos Exp $	*/

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

/*! \file isc/async.h
 * \brief The isc_async unit provides a way to schedule jobs on any isc
 * event loop (isc_loop unit)
 *
 * The unit is built around the uv_async_t primitive and lock-free stack with
 * isc_job_cb.  Jobs are first scheduled onto the locked list, then the
 * uv_async_send() is called and the uv_async_t callback processes the enqueued
 * jobs are scheduled to be run on the isc event loop.
 */

#pragma once

#include <inttypes.h>

#include <isc/job.h>
#include <isc/lang.h>
#include <isc/loop.h>
#include <isc/mem.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

void
isc_async_run(isc_loop_t *loop, isc_job_cb cb, void *cbarg);
/*%<
 * Schedule the job callback 'cb' to be run on the 'loop' event loop.
 *
 * Requires:
 *
 *\li	'loop' is a valid isc event loop
 *\li	'cb' is a callback function, must be non-NULL
 *\li	'cbarg' is passed to the 'cb' as the only argument, may be NULL
 */

#define isc_async_current(cb, cbarg) isc_async_run(isc_loop(), cb, cbarg)
/*%<
 * Helper macro to run the job on the current loop
 */

ISC_LANG_ENDDECLS
