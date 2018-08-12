/*	$NetBSD: timer.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#ifndef DNS_TIMER_H
#define DNS_TIMER_H 1

/*! \file dns/timer.h */

/***
 ***	Imports
 ***/

#include <isc/buffer.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_timer_setidle(isc_timer_t *timer, unsigned int maxtime,
		  unsigned int idletime, isc_boolean_t purge);
/*%<
 * Convenience function for setting up simple, one-second-granularity
 * idle timers as used by zone transfers.
 * \brief
 * Set the timer 'timer' to go off after 'idletime' seconds of inactivity,
 * or after 'maxtime' at the very latest.  Events are purged iff
 * 'purge' is ISC_TRUE.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TIMER_H */
