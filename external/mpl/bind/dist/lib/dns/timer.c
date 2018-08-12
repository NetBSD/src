/*	$NetBSD: timer.c,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


/*! \file */

#include <config.h>

#include <isc/result.h>
#include <isc/time.h>
#include <isc/timer.h>

#include <dns/types.h>
#include <dns/timer.h>

#define CHECK(op) \
	do { result = (op);					\
		if (result != ISC_R_SUCCESS) goto failure;	\
	} while (/*CONSTCOND*/0)

isc_result_t
dns_timer_setidle(isc_timer_t *timer, unsigned int maxtime,
		  unsigned int idletime, isc_boolean_t purge)
{
	isc_result_t result;
	isc_interval_t maxinterval, idleinterval;
	isc_time_t expires;

	/* Compute the time of expiry. */
	isc_interval_set(&maxinterval, maxtime, 0);
	CHECK(isc_time_nowplusinterval(&expires, &maxinterval));

	/*
	 * Compute the idle interval, and add a spare nanosecond to
	 * work around the silly limitation of the ISC timer interface
	 * that you cannot specify an idle interval of zero.
	 */
	isc_interval_set(&idleinterval, idletime, 1);

	CHECK(isc_timer_reset(timer, isc_timertype_once,
			      &expires, &idleinterval,
			      purge));
 failure:
	return (result);
}
