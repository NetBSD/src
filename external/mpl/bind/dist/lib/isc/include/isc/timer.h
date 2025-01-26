/*	$NetBSD: timer.h,v 1.11 2025/01/26 16:25:43 christos Exp $	*/

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

/*! \file isc/timer.h
 * \brief Provides timers which are event sources in the task system.
 *
 * Two types of timers are supported:
 *
 *\li	'ticker' timers generate a periodic tick event.
 *
 *\li	'once' timers generate an timeout event if the time reaches
 *      the set interval.
 *
 *\li MP:
 *	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *	Clients of this module must not be holding a timer's task's lock when
 *	making a call that affects that timer.  Failure to follow this rule
 *	can result in deadlock.
 *	The caller must ensure that isc_timermgr_destroy() is called only
 *	once for a given manager.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	TBS
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 */

/***
 *** Imports
 ***/

#include <stdbool.h>

#include <isc/job.h>
#include <isc/lang.h>
#include <isc/time.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/***
 *** Types
 ***/

/*% Timer Type */
typedef enum {
	isc_timertype_undefined = -1, /*%< Undefined */
	isc_timertype_ticker = 0,     /*%< Ticker */
	isc_timertype_once = 1,	      /*%< Once */
} isc_timertype_t;

/***
 *** Timer and Timer Manager Functions
 ***
 *** Note: all Ensures conditions apply only if the result is success for
 *** those functions which return an isc_result_t.
 ***/

void
isc_timer_create(isc_loop_t *loop, isc_job_cb cb, void *cbarg,
		 isc_timer_t **timerp);
/*%<
 * Create a new 'type' timer managed by 'loop'.  The timers parameters are
 * specified by 'expires' and 'interval'.  Events will be posted on the isc
 * event loop and when dispatched 'cb' will be called with 'cbarg' as the arg
 * value.  The new timer is returned in 'timerp'.
 *
 * Requires:
 *
 *\li	'loop' is a valid manager
 *\li	'cb' is a valid job
 *\li	'timerp' is a valid pointer, and *timerp == NULL
 *
 * Ensures:
 *
 *\li	'*timerp' is attached to the newly created timer
 */

void
isc_timer_stop(isc_timer_t *timer);
/*%<
 * Stop the timer.
 *
 * Requires:
 *
 *\li	'timer' is a valid timer
 */

void
isc_timer_start(isc_timer_t *timer, isc_timertype_t type,
		const isc_interval_t *interval);
/*%<
 * Start the timer.
 *
 * Notes:
 *
 *\li	For ticker timers, the timer will generate a 'tick' event every
 *	'interval' seconds.
 *
 *\li	For once timers, 'interval' specifies how long the timer
 *	can be idle before it generates an idle timeout.  If 0, then
 *	the timer will be run immediately.
 *
 *\li	If 'interval' is NULL, the zero interval will be used.
 *
 * Requires:
 *
 *\li	'timer' is a valid timer
 *\li	'type' is either 'isc_timertype_ticker' or 'isc_timertype_once'
 *\li	'interval' points to a valid interval, or is NULL.
 *
 * Ensures:
 *
 *\li	An idle timeout will not be generated until at least Now + the
 *	timer's interval if 'timer' is a once timer with a non-zero
 *	interval.
 */

void
isc_timer_async_destroy(isc_timer_t **timerp);
void
isc_timer_destroy(isc_timer_t **timerp);
/*%<
 * Destroy (asynchronously) the timer *timerp.
 *
 * Requires:
 *
 *\li	'timerp' points to a valid timer.
 *
 * Ensures:
 *
 *\li	*timerp is NULL.
 */

ISC_LANG_ENDDECLS
