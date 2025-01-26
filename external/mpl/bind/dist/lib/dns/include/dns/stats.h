/*	$NetBSD: stats.h,v 1.9 2025/01/26 16:25:28 christos Exp $	*/

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

/*! \file dns/stats.h */

#include <inttypes.h>

#include <isc/histo.h>

#include <dns/types.h>

/*%
 * Statistics counters.  Used as isc_statscounter_t values.
 */
enum {
	/*%
	 * Resolver statistics counters.
	 */
	dns_resstatscounter_queryv4 = 0,
	dns_resstatscounter_queryv6 = 1,
	dns_resstatscounter_responsev4 = 2,
	dns_resstatscounter_responsev6 = 3,
	dns_resstatscounter_nxdomain = 4,
	dns_resstatscounter_servfail = 5,
	dns_resstatscounter_formerr = 6,
	dns_resstatscounter_othererror = 7,
	dns_resstatscounter_edns0fail = 8,
	dns_resstatscounter_mismatch = 9,
	dns_resstatscounter_truncated = 10,
	dns_resstatscounter_lame = 11,
	dns_resstatscounter_retry = 12,
	dns_resstatscounter_gluefetchv4 = 13,
	dns_resstatscounter_gluefetchv6 = 14,
	dns_resstatscounter_gluefetchv4fail = 15,
	dns_resstatscounter_gluefetchv6fail = 16,
	dns_resstatscounter_val = 17,
	dns_resstatscounter_valsuccess = 18,
	dns_resstatscounter_valnegsuccess = 19,
	dns_resstatscounter_valfail = 20,
	dns_resstatscounter_dispabort = 21,
	dns_resstatscounter_dispsockfail = 22,
	dns_resstatscounter_querytimeout = 23,
	dns_resstatscounter_queryrtt0 = 24,
	dns_resstatscounter_queryrtt1 = 25,
	dns_resstatscounter_queryrtt2 = 26,
	dns_resstatscounter_queryrtt3 = 27,
	dns_resstatscounter_queryrtt4 = 28,
	dns_resstatscounter_queryrtt5 = 29,
	dns_resstatscounter_nfetch = 30,
	dns_resstatscounter_disprequdp = 31,
	dns_resstatscounter_dispreqtcp = 32,
	dns_resstatscounter_buckets = 33,
	dns_resstatscounter_refused = 34,
	dns_resstatscounter_cookienew = 35,
	dns_resstatscounter_cookieout = 36,
	dns_resstatscounter_cookiein = 37,
	dns_resstatscounter_cookieok = 38,
	dns_resstatscounter_badvers = 39,
	dns_resstatscounter_badcookie = 40,
	dns_resstatscounter_zonequota = 41,
	dns_resstatscounter_serverquota = 42,
	dns_resstatscounter_clientquota = 43,
	dns_resstatscounter_nextitem = 44,
	dns_resstatscounter_priming = 45,
	dns_resstatscounter_max = 46,

	/*
	 * DNSSEC stats.
	 */
	dns_dnssecstats_asis = 0,
	dns_dnssecstats_downcase = 1,
	dns_dnssecstats_wildcard = 2,
	dns_dnssecstats_fail = 3,

	dns_dnssecstats_max = 4,

	/*%
	 * Zone statistics counters.
	 */
	dns_zonestatscounter_notifyoutv4 = 0,
	dns_zonestatscounter_notifyoutv6 = 1,
	dns_zonestatscounter_notifyinv4 = 2,
	dns_zonestatscounter_notifyinv6 = 3,
	dns_zonestatscounter_notifyrej = 4,
	dns_zonestatscounter_soaoutv4 = 5,
	dns_zonestatscounter_soaoutv6 = 6,
	dns_zonestatscounter_axfrreqv4 = 7,
	dns_zonestatscounter_axfrreqv6 = 8,
	dns_zonestatscounter_ixfrreqv4 = 9,
	dns_zonestatscounter_ixfrreqv6 = 10,
	dns_zonestatscounter_xfrsuccess = 11,
	dns_zonestatscounter_xfrfail = 12,

	dns_zonestatscounter_max = 13,

	/*
	 * Adb statistics values.
	 */
	dns_adbstats_nentries = 0,
	dns_adbstats_entriescnt = 1,
	dns_adbstats_nnames = 2,
	dns_adbstats_namescnt = 3,

	dns_adbstats_max = 4,

	/*
	 * Cache statistics values.
	 */
	dns_cachestatscounter_hits = 1,
	dns_cachestatscounter_misses = 2,
	dns_cachestatscounter_queryhits = 3,
	dns_cachestatscounter_querymisses = 4,
	dns_cachestatscounter_deletelru = 5,
	dns_cachestatscounter_deletettl = 6,
	dns_cachestatscounter_coveringnsec = 7,

	dns_cachestatscounter_max = 8,

	/*%
	 * Query statistics counters (obsolete).
	 */
	dns_statscounter_success = 0,	/*%< Successful lookup */
	dns_statscounter_referral = 1,	/*%< Referral result */
	dns_statscounter_nxrrset = 2,	/*%< NXRRSET result */
	dns_statscounter_nxdomain = 3,	/*%< NXDOMAIN result */
	dns_statscounter_recursion = 4, /*%< Recursion was used */
	dns_statscounter_failure = 5,	/*%< Some other failure */
	dns_statscounter_duplicate = 6, /*%< Duplicate query */
	dns_statscounter_dropped = 7,	/*%< Duplicate query (dropped) */

	/*%
	 * DNSTAP statistics counters.
	 */
	dns_dnstapcounter_success = 0,
	dns_dnstapcounter_drop = 1,
	dns_dnstapcounter_max = 2,

	/*
	 * Glue cache statistics counters.
	 */
	dns_gluecachestatscounter_hits_present = 0,
	dns_gluecachestatscounter_hits_absent = 1,
	dns_gluecachestatscounter_inserts_present = 2,
	dns_gluecachestatscounter_inserts_absent = 3,

	dns_gluecachestatscounter_max = 4,
};

/*%
 * Traffic size statistics, according to RSSAC002 section 2.4
 * https://www.icann.org/en/system/files/files/rssac-002-measurements-root-20nov14-en.pdf
 *
 * The RSSAC002 linear bucketing does not directly match the log-linear
 * bucketing of an `isc_histo_t`, so we need to adjust some parameters
 * to fit.
 *
 * To map a message size to an `isc_histo_t`, first divide by
 * DNS_SIZEHISTO_QUANTUM so that `isc_histo_inc()` is presented with
 * one value per RSSAC002 bucket.
 *
 * Configure the `isc_histo_t` with large enough `sigbits` that its
 * one-value-per-bucket range (its `UNITBUCKETS`) covers the range
 * required by RSSAC002.
 */
#define DNS_SIZEHISTO_QUANTUM	 16
#define DNS_SIZEHISTO_MAXIN	 (288 / DNS_SIZEHISTO_QUANTUM)
#define DNS_SIZEHISTO_MAXOUT	 (4096 / DNS_SIZEHISTO_QUANTUM)
#define DNS_SIZEHISTO_SIGBITSIN	 4
#define DNS_SIZEHISTO_SIGBITSOUT 7

#define DNS_SIZEHISTO_BUCKETIN(size) \
	ISC_MIN(size / DNS_SIZEHISTO_QUANTUM, DNS_SIZEHISTO_MAXIN)

#define DNS_SIZEHISTO_BUCKETOUT(size) \
	ISC_MIN(size / DNS_SIZEHISTO_QUANTUM, DNS_SIZEHISTO_MAXOUT)

STATIC_ASSERT(DNS_SIZEHISTO_MAXIN <=
		      ISC_HISTO_UNITBUCKETS(DNS_SIZEHISTO_SIGBITSIN),
	      "must be enough histogram buckets for RSSAC002");

STATIC_ASSERT(DNS_SIZEHISTO_MAXOUT <=
		      ISC_HISTO_UNITBUCKETS(DNS_SIZEHISTO_SIGBITSOUT),
	      "must be enough histogram buckets for RSSAC002");

/*
 * For consistency with other stats counters
 */
enum {
	dns_sizecounter_in_max = DNS_SIZEHISTO_MAXIN + 1,
	dns_sizecounter_out_max = DNS_SIZEHISTO_MAXOUT + 1,
};

/*%
 * Attributes for statistics counters of RRset and Rdatatype types.
 *
 * _OTHERTYPE
 *	The rdata type is not explicitly supported and the corresponding counter
 *	is counted for other such types, too.  When this attribute is set,
 *	the base type is of no use.
 *
 * _NXRRSET
 * 	RRset type counters only.  Indicates the RRset is non existent.
 *
 * _NXDOMAIN
 *	RRset type counters only.  Indicates a non existent name.  When this
 *	attribute is set, the base type is of no use.
 *
 * _STALE
 *	RRset type counters only.  This indicates a record that is stale
 *	but may still be served.
 *
 * _ANCIENT
 *	RRset type counters only.  This indicates a record that is marked for
 *	removal.
 */
#define DNS_RDATASTATSTYPE_ATTR_OTHERTYPE 0x0001
#define DNS_RDATASTATSTYPE_ATTR_NXRRSET	  0x0002
#define DNS_RDATASTATSTYPE_ATTR_NXDOMAIN  0x0004
#define DNS_RDATASTATSTYPE_ATTR_STALE	  0x0008
#define DNS_RDATASTATSTYPE_ATTR_ANCIENT	  0x0010

/*%<
 * Conversion macros among dns_rdatatype_t, attributes and isc_statscounter_t.
 */
#define DNS_RDATASTATSTYPE_BASE(type)  ((dns_rdatatype_t)((type) & 0xFFFF))
#define DNS_RDATASTATSTYPE_ATTR(type)  ((type) >> 16)
#define DNS_RDATASTATSTYPE_VALUE(b, a) (((a) << 16) | (b))

/*%
 * Types of DNSSEC sign statistics operations.
 */
typedef enum {
	dns_dnssecsignstats_sign = 1,
	dns_dnssecsignstats_refresh = 2
} dnssecsignstats_type_t;

/*%<
 * Types of dump callbacks.
 */
typedef void (*dns_generalstats_dumper_t)(isc_statscounter_t, uint64_t, void *);
typedef void (*dns_rdatatypestats_dumper_t)(dns_rdatastatstype_t, uint64_t,
					    void *);
typedef void (*dns_dnssecsignstats_dumper_t)(uint32_t, uint64_t, void *);
typedef void (*dns_opcodestats_dumper_t)(dns_opcode_t, uint64_t, void *);
typedef void (*dns_rcodestats_dumper_t)(dns_rcode_t, uint64_t, void *);

ISC_LANG_BEGINDECLS

void
dns_generalstats_create(isc_mem_t *mctx, dns_stats_t **statsp, int ncounters);
/*%<
 * Create a statistics counter structure of general type.  It counts a general
 * set of counters indexed by an ID between 0 and ncounters -1.
 * This function is obsolete.  A more general function, isc_stats_create(),
 * should be used.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 */

void
dns_rdatatypestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per rdatatype.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 */

void
dns_rdatasetstats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per RRset.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 */

void
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per opcode.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 */

void
dns_rcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per assigned rcode.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 */

void
dns_dnssecsignstats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per assigned DNSKEY id.
 *
 * Requires:
 *\li	'mctx' must be a valid memory context.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL.
 */

void
dns_stats_attach(dns_stats_t *stats, dns_stats_t **statsp);
/*%<
 * Attach to a statistics set.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t.
 *
 *\li	'statsp' != NULL && '*statsp' == NULL
 */

void
dns_stats_detach(dns_stats_t **statsp);
/*%<
 * Detaches from the statistics set.
 *
 * Requires:
 *\li	'statsp' != NULL and '*statsp' is a valid dns_stats_t.
 */

void
dns_generalstats_increment(dns_stats_t *stats, isc_statscounter_t counter);
/*%<
 * Increment the counter-th counter of stats.  This function is obsolete.
 * A more general function, isc_stats_increment(), should be used.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 *
 *\li	counter is less than the maximum available ID for the stats specified
 *	on creation.
 */

void
dns_rdatatypestats_increment(dns_stats_t *stats, dns_rdatatype_t type);
/*%<
 * Increment the statistics counter for 'type'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_rdatatypestats_create().
 */

void
dns_rdatasetstats_increment(dns_stats_t *stats, dns_rdatastatstype_t rrsettype);
/*%<
 * Increment the statistics counter for 'rrsettype'.
 *
 * Note: if 'rrsettype' has the _STALE attribute set the corresponding
 * non-stale counter will be decremented.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_rdatasetstats_create().
 */

void
dns_rdatasetstats_decrement(dns_stats_t *stats, dns_rdatastatstype_t rrsettype);
/*%<
 * Decrement the statistics counter for 'rrsettype'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_rdatasetstats_create().
 */

void
dns_opcodestats_increment(dns_stats_t *stats, dns_opcode_t code);
/*%<
 * Increment the statistics counter for 'code'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_opcodestats_create().
 */

void
dns_rcodestats_increment(dns_stats_t *stats, dns_opcode_t code);
/*%<
 * Increment the statistics counter for 'code'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_rcodestats_create().
 */

void
dns_dnssecsignstats_increment(dns_stats_t *stats, dns_keytag_t id, uint8_t alg,
			      dnssecsignstats_type_t operation);
/*%<
 * Increment the statistics counter for the DNSKEY 'id' with algorithm 'alg'.
 * The 'operation' determines what counter is incremented.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_dnssecsignstats_create().
 */

void
dns_dnssecsignstats_clear(dns_stats_t *stats, dns_keytag_t id, uint8_t alg);
/*%<
 * Clear the statistics counter for the DNSKEY 'id' with algorithm 'alg'.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_dnssecsignstats_create().
 */

void
dns_generalstats_dump(dns_stats_t *stats, dns_generalstats_dumper_t dump_fn,
		      void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with its current value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * This function is obsolete.  A more general function, isc_stats_dump(),
 * should be used.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_rdatatypestats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
			void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding type in the form of
 * dns_rdatastatstype_t, the current counter value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_rdatasetstats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
		       void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding type in the form of
 * dns_rdatastatstype_t, the current counter value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_dnssecsignstats_dump(dns_stats_t *stats, dnssecsignstats_type_t operation,
			 dns_dnssecsignstats_dumper_t dump_fn, void *arg,
			 unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding type in the form of
 * dns_rdatastatstype_t, the current counter value and the given argument
 * arg.  By default counters that have a value of 0 is skipped; if options has
 * the ISC_STATSDUMP_VERBOSE flag, even such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_opcodestats_dump(dns_stats_t *stats, dns_opcodestats_dumper_t dump_fn,
		     void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding opcode, the current
 * counter value and the given argument arg.  By default counters that have a
 * value of 0 is skipped; if options has the ISC_STATSDUMP_VERBOSE flag, even
 * such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

void
dns_rcodestats_dump(dns_stats_t *stats, dns_rcodestats_dumper_t dump_fn,
		    void *arg, unsigned int options);
/*%<
 * Dump the current statistics counters in a specified way.  For each counter
 * in stats, dump_fn is called with the corresponding rcode, the current
 * counter value and the given argument arg.  By default counters that have a
 * value of 0 is skipped; if options has the ISC_STATSDUMP_VERBOSE flag, even
 * such counters are dumped.
 *
 * Requires:
 *\li	'stats' is a valid dns_stats_t created by dns_generalstats_create().
 */

ISC_LANG_ENDDECLS
