/*	$NetBSD: stats.h,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#ifndef NS_STATS_H
#define NS_STATS_H 1

/*! \file include/ns/stats.h */

#include <ns/types.h>

/*%
 * Server statistics counters.  Used as isc_statscounter_t values.
 */
enum {
	ns_statscounter_requestv4 = 0,
	ns_statscounter_requestv6 = 1,
	ns_statscounter_edns0in = 2,
	ns_statscounter_badednsver = 3,
	ns_statscounter_tsigin = 4,
	ns_statscounter_sig0in = 5,
	ns_statscounter_invalidsig = 6,
	ns_statscounter_requesttcp = 7,

	ns_statscounter_authrej = 8,
	ns_statscounter_recurserej = 9,
	ns_statscounter_xfrrej = 10,
	ns_statscounter_updaterej = 11,

	ns_statscounter_response = 12,
	ns_statscounter_truncatedresp = 13,
	ns_statscounter_edns0out = 14,
	ns_statscounter_tsigout = 15,
	ns_statscounter_sig0out = 16,

	ns_statscounter_success = 17,
	ns_statscounter_authans = 18,
	ns_statscounter_nonauthans = 19,
	ns_statscounter_referral = 20,
	ns_statscounter_nxrrset = 21,
	ns_statscounter_servfail = 22,
	ns_statscounter_formerr = 23,
	ns_statscounter_nxdomain = 24,
	ns_statscounter_recursion = 25,
	ns_statscounter_duplicate = 26,
	ns_statscounter_dropped = 27,
	ns_statscounter_failure = 28,

	ns_statscounter_xfrdone = 29,

	ns_statscounter_updatereqfwd = 30,
	ns_statscounter_updaterespfwd = 31,
	ns_statscounter_updatefwdfail = 32,
	ns_statscounter_updatedone = 33,
	ns_statscounter_updatefail = 34,
	ns_statscounter_updatebadprereq = 35,

	ns_statscounter_recursclients = 36,

	ns_statscounter_dns64 = 37,

	ns_statscounter_ratedropped = 38,
	ns_statscounter_rateslipped = 39,

	ns_statscounter_rpz_rewrites = 40,

	ns_statscounter_udp = 41,
	ns_statscounter_tcp = 42,

	ns_statscounter_nsidopt = 43,
	ns_statscounter_expireopt = 44,
	ns_statscounter_otheropt = 45,
	ns_statscounter_ecsopt = 46,
	ns_statscounter_padopt = 47,
	ns_statscounter_keepaliveopt = 48,

	ns_statscounter_nxdomainredirect = 49,
	ns_statscounter_nxdomainredirect_rlookup = 50,

	ns_statscounter_cookiein = 51,
	ns_statscounter_cookiebadsize = 52,
	ns_statscounter_cookiebadtime = 53,
	ns_statscounter_cookienomatch = 54,
	ns_statscounter_cookiematch = 55,
	ns_statscounter_cookienew = 56,
	ns_statscounter_badcookie = 57,

	ns_statscounter_nxdomainsynth = 58,
	ns_statscounter_nodatasynth = 59,
	ns_statscounter_wildcardsynth = 60,

	ns_statscounter_trystale = 61,
	ns_statscounter_usedstale = 62,

	ns_statscounter_prefetch = 63,
	ns_statscounter_keytagopt = 64,

	ns_statscounter_max = 65
};

void
ns_stats_attach(ns_stats_t *stats, ns_stats_t **statsp);

void
ns_stats_detach(ns_stats_t **statsp);

isc_result_t
ns_stats_create(isc_mem_t *mctx, int ncounters, ns_stats_t **statsp);

void
ns_stats_increment(ns_stats_t *stats, isc_statscounter_t counter);

void
ns_stats_decrement(ns_stats_t *stats, isc_statscounter_t counter);

isc_stats_t *
ns_stats_get(ns_stats_t *stats);

#endif /* NS_STATS_H */
