/*	$NetBSD: ratelimiter.h,v 1.8 2025/01/26 16:25:42 christos Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file isc/ratelimiter.h
 * \brief A rate limiter is a mechanism for dispatching events at a limited
 * rate.  This is intended to be used when sending zone maintenance
 * SOA queries, NOTIFY messages, etc.
 */

/***
 *** Imports.
 ***/

#include <inttypes.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/loop.h>
#include <isc/time.h>
#include <isc/types.h>

struct isc_rlevent {
	isc_loop_t	  *loop;
	isc_ratelimiter_t *rl;
	bool		   canceled;
	isc_job_cb	   cb;
	void		  *arg;
	ISC_LINK(isc_rlevent_t) link;
};

ISC_LANG_BEGINDECLS

/*****
***** Functions.
*****/

void
isc_ratelimiter_create(isc_loop_t *loop, isc_ratelimiter_t **rlp);
/*%<
 * Create a rate limiter.  The execution interval is initially undefined.
 */

void
isc_ratelimiter_setinterval(isc_ratelimiter_t *restrict rl,
			    const isc_interval_t *const interval);
/*!<
 * Set the minimum interval between event executions.
 * The interval value is copied, so the caller need not preserve it.
 *
 * Requires:
 *	'*interval' is a nonzero interval.
 */

void
isc_ratelimiter_setpertic(isc_ratelimiter_t *restrict rl,
			  const uint32_t perint);
/*%<
 * Set the number of events processed per interval timer tick.
 * If 'perint' is zero it is treated as 1.
 */

void
isc_ratelimiter_setpushpop(isc_ratelimiter_t *restrict rl, const bool pushpop);
/*%<
 * Set / clear the ratelimiter to from push pop mode rather
 * first in - first out mode (default).
 */

isc_result_t
isc_ratelimiter_enqueue(isc_ratelimiter_t *restrict rl,
			isc_loop_t *restrict loop, isc_job_cb cb, void *arg,
			isc_rlevent_t **rlep);
/*%<
 * Queue an event for rate-limited execution.
 *
 * This is similar to doing an isc_async_run() to the 'loop', except
 * that the execution may be delayed to achieve the desired rate of
 * execution.
 *
 * '*rlep' will be set to point to an allocated ratelimiter event,
 * which can be freed by the caller using isc_rlevent_free() when the
 * event fires, or by dequeueing.
 *
 * Requires:
 *\li	'rl' is a valid ratelimiter.
 *\li	'loop ' is non NULL.
 *\li	'rlep' is non NULL and '*rlep' is NULL.
 */

isc_result_t
isc_ratelimiter_dequeue(isc_ratelimiter_t *restrict rl,
			isc_rlevent_t **rleventp);
/*
 * Dequeue a event off the ratelimiter queue. If the event has not already
 * been posted, it will be freed and '*rleventp' will be set to NULL.
 *
 * Returns:
 * \li	ISC_R_NOTFOUND if the event is no longer linked to the rate limiter.
 * \li	ISC_R_SUCCESS
 */

void
isc_ratelimiter_shutdown(isc_ratelimiter_t *restrict rl);
/*%<
 * Shut down a rate limiter.
 *
 * Ensures:
 *\li	All pending events are dispatched immediately with
 *	rle->canceled set to true.
 *
 *\li	Further attempts to enqueue events will fail with
 *	#ISC_R_SHUTTINGDOWN.
 */

void
isc_rlevent_free(isc_rlevent_t **rlep);
/*%<
 * Free the rate limiter event '*rlep'.
 */

ISC_REFCOUNT_DECL(isc_ratelimiter);
/*%<
 * The rate limiter reference counting.
 */

ISC_LANG_ENDDECLS
