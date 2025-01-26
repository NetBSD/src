/*	$NetBSD: stats.h,v 1.10 2025/01/26 16:25:42 christos Exp $	*/

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
	isc_sockstatscounter_udp6open,
	isc_sockstatscounter_tcp4open,
	isc_sockstatscounter_tcp6open,

	isc_sockstatscounter_udp4openfail,
	isc_sockstatscounter_udp6openfail,
	isc_sockstatscounter_tcp4openfail,
	isc_sockstatscounter_tcp6openfail,

	isc_sockstatscounter_udp4close,
	isc_sockstatscounter_udp6close,
	isc_sockstatscounter_tcp4close,
	isc_sockstatscounter_tcp6close,

	isc_sockstatscounter_udp4bindfail,
	isc_sockstatscounter_udp6bindfail,
	isc_sockstatscounter_tcp4bindfail,
	isc_sockstatscounter_tcp6bindfail,

	isc_sockstatscounter_udp4connect,
	isc_sockstatscounter_udp6connect,
	isc_sockstatscounter_tcp4connect,
	isc_sockstatscounter_tcp6connect,

	isc_sockstatscounter_udp4connectfail,
	isc_sockstatscounter_udp6connectfail,
	isc_sockstatscounter_tcp4connectfail,
	isc_sockstatscounter_tcp6connectfail,

	isc_sockstatscounter_tcp4accept,
	isc_sockstatscounter_tcp6accept,

	isc_sockstatscounter_tcp4acceptfail,
	isc_sockstatscounter_tcp6acceptfail,

	isc_sockstatscounter_udp4sendfail,
	isc_sockstatscounter_udp6sendfail,
	isc_sockstatscounter_tcp4sendfail,
	isc_sockstatscounter_tcp6sendfail,

	isc_sockstatscounter_udp4recvfail,
	isc_sockstatscounter_udp6recvfail,
	isc_sockstatscounter_tcp4recvfail,
	isc_sockstatscounter_tcp6recvfail,

	isc_sockstatscounter_udp4active,
	isc_sockstatscounter_udp6active,
	isc_sockstatscounter_tcp4active,
	isc_sockstatscounter_tcp6active,

	isc_sockstatscounter_tcp4clients,
	isc_sockstatscounter_tcp6clients,

	isc_sockstatscounter_max,
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

void
isc_stats_create(isc_mem_t *mctx, isc_stats_t **statsp, int ncounters);
/*%<
 * Create a statistics counter structure of general type.  It counts a general
 * set of counters indexed by an ID between 0 and ncounters -1.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
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

isc_statscounter_t
isc_stats_increment(isc_stats_t *stats, isc_statscounter_t counter);
/*%<
 * Increment the counter-th counter of stats and return the old value.
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
