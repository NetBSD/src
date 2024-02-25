/*	$NetBSD: stats.h,v 1.7.2.1 2024/02/25 15:47:23 martin Exp $	*/

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

/*! \file isc/stats.h */

#include <inttypes.h>

#include <isc/types.h>

/*%
 * Statistics counters.  Used as isc_statscounter_t values.
 */
enum {
	/*%
	 * Socket statistics counters.
	 */
	isc_sockstatscounter_udp4open = 0,
	isc_sockstatscounter_udp6open = 1,
	isc_sockstatscounter_tcp4open = 2,
	isc_sockstatscounter_tcp6open = 3,
	isc_sockstatscounter_unixopen = 4,

	isc_sockstatscounter_udp4openfail = 5,
	isc_sockstatscounter_udp6openfail = 6,
	isc_sockstatscounter_tcp4openfail = 7,
	isc_sockstatscounter_tcp6openfail = 8,
	isc_sockstatscounter_unixopenfail = 9,

	isc_sockstatscounter_udp4close = 10,
	isc_sockstatscounter_udp6close = 11,
	isc_sockstatscounter_tcp4close = 12,
	isc_sockstatscounter_tcp6close = 13,
	isc_sockstatscounter_unixclose = 14,
	isc_sockstatscounter_fdwatchclose = 15,

	isc_sockstatscounter_udp4bindfail = 16,
	isc_sockstatscounter_udp6bindfail = 17,
	isc_sockstatscounter_tcp4bindfail = 18,
	isc_sockstatscounter_tcp6bindfail = 19,
	isc_sockstatscounter_unixbindfail = 20,
	isc_sockstatscounter_fdwatchbindfail = 21,

	isc_sockstatscounter_udp4connect = 22,
	isc_sockstatscounter_udp6connect = 23,
	isc_sockstatscounter_tcp4connect = 24,
	isc_sockstatscounter_tcp6connect = 25,
	isc_sockstatscounter_unixconnect = 26,
	isc_sockstatscounter_fdwatchconnect = 27,

	isc_sockstatscounter_udp4connectfail = 28,
	isc_sockstatscounter_udp6connectfail = 29,
	isc_sockstatscounter_tcp4connectfail = 30,
	isc_sockstatscounter_tcp6connectfail = 31,
	isc_sockstatscounter_unixconnectfail = 32,
	isc_sockstatscounter_fdwatchconnectfail = 33,

	isc_sockstatscounter_tcp4accept = 34,
	isc_sockstatscounter_tcp6accept = 35,
	isc_sockstatscounter_unixaccept = 36,

	isc_sockstatscounter_tcp4acceptfail = 37,
	isc_sockstatscounter_tcp6acceptfail = 38,
	isc_sockstatscounter_unixacceptfail = 39,

	isc_sockstatscounter_udp4sendfail = 40,
	isc_sockstatscounter_udp6sendfail = 41,
	isc_sockstatscounter_tcp4sendfail = 42,
	isc_sockstatscounter_tcp6sendfail = 43,
	isc_sockstatscounter_unixsendfail = 44,
	isc_sockstatscounter_fdwatchsendfail = 45,

	isc_sockstatscounter_udp4recvfail = 46,
	isc_sockstatscounter_udp6recvfail = 47,
	isc_sockstatscounter_tcp4recvfail = 48,
	isc_sockstatscounter_tcp6recvfail = 49,
	isc_sockstatscounter_unixrecvfail = 50,
	isc_sockstatscounter_fdwatchrecvfail = 51,

	isc_sockstatscounter_udp4active = 52,
	isc_sockstatscounter_udp6active = 53,
	isc_sockstatscounter_tcp4active = 54,
	isc_sockstatscounter_tcp6active = 55,
	isc_sockstatscounter_unixactive = 56,

	isc_sockstatscounter_rawopen = 57,
	isc_sockstatscounter_rawopenfail = 58,
	isc_sockstatscounter_rawclose = 59,
	isc_sockstatscounter_rawrecvfail = 60,
	isc_sockstatscounter_rawactive = 61,

	isc_sockstatscounter_max = 62
};

ISC_LANG_BEGINDECLS

/*%<
 * Flag(s) for isc_stats_dump().
 */
#define ISC_STATSDUMP_VERBOSE 0x00000001 /*%< dump 0-value counters */

/*%<
 * Dump callback type.
 */
typedef void (*isc_stats_dumper_t)(isc_statscounter_t, uint64_t, void *);

isc_result_t
isc_stats_create(isc_mem_t *mctx, isc_stats_t **statsp, int ncounters);
/*%<
 * Create a statistics counter structure of general type.  It counts a general
 * set of counters indexed by an ID between 0 and ncounters -1.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- all ok
 *
 *\li	anything else	-- failure
 */

void
isc_stats_attach(isc_stats_t *stats, isc_stats_t **statsp);
/*%<
 * Attach to a statistics set.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL
 */

void
isc_stats_detach(isc_stats_t **statsp);
/*%<
 * Detaches from the statistics set.
 *
 * Requires:
 *\li	'statsp' != NULL and '*statsp' is a valid isc_stats_t.
 */

int
isc_stats_ncounters(isc_stats_t *stats);
/*%<
 * Returns the number of counters contained in stats.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 *
 */

void
isc_stats_increment(isc_stats_t *stats, isc_statscounter_t counter);
/*%<
 * Increment the counter-th counter of stats.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 *
 *\li	counter is less than the maximum available ID for the stats specified
 *	on creation.
 */

void
isc_stats_decrement(isc_stats_t *stats, isc_statscounter_t counter);
/*%<
 * Decrement the counter-th counter of stats.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 */

void
isc_stats_dump(isc_stats_t *stats, isc_stats_dumper_t dump_fn, void *arg,
	       unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with its current value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 */

void
isc_stats_set(isc_stats_t *stats, uint64_t val, isc_statscounter_t counter);
/*%<
 * Set the given counter to the specified value.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 */

void
isc_stats_set(isc_stats_t *stats, uint64_t val, isc_statscounter_t counter);
/*%<
 * Set the given counter to the specified value.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 */

void
isc_stats_update_if_greater(isc_stats_t *stats, isc_statscounter_t counter,
			    isc_statscounter_t value);
/*%<
 * Atomically assigns 'value' to 'counter' if value > counter.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 *
 *\li	counter is less than the maximum available ID for the stats specified
 *	on creation.
 */

isc_statscounter_t
isc_stats_get_counter(isc_stats_t *stats, isc_statscounter_t counter);
/*%<
 * Returns value currently stored in counter.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 *
 *\li	counter is less than the maximum available ID for the stats specified
 *	on creation.
 */

void
isc_stats_resize(isc_stats_t **stats, int ncounters);
/*%<
 * Resize a statistics counter structure of general type. The new set of
 * counters are indexed by an ID between 0 and ncounters -1.
 *
 * Requires:
 *\li	'stats' is a valid isc_stats_t.
 *\li	'ncounters' is a non-zero positive number.
 */

ISC_LANG_ENDDECLS
