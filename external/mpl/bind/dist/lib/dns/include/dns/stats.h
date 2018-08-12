/*	$NetBSD: stats.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#ifndef DNS_STATS_H
#define DNS_STATS_H 1

/*! \file dns/stats.h */

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
	dns_resstatscounter_nextitem = 43,
	dns_resstatscounter_priming = 44,
	dns_resstatscounter_max = 45,

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

	dns_cachestatscounter_max = 7,

	/*%
	 * Query statistics counters (obsolete).
	 */
	dns_statscounter_success = 0,    /*%< Successful lookup */
	dns_statscounter_referral = 1,   /*%< Referral result */
	dns_statscounter_nxrrset = 2,    /*%< NXRRSET result */
	dns_statscounter_nxdomain = 3,   /*%< NXDOMAIN result */
	dns_statscounter_recursion = 4,  /*%< Recursion was used */
	dns_statscounter_failure = 5,    /*%< Some other failure */
	dns_statscounter_duplicate = 6,  /*%< Duplicate query */
	dns_statscounter_dropped = 7,	 /*%< Duplicate query (dropped) */

	/*%
	 * DNSTAP statistics counters.
	 */
	dns_dnstapcounter_success = 0,
	dns_dnstapcounter_drop =  1,
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
 * Traffic size statistics counters. Used as isc_statscounter_t values.
 */
enum {
	dns_sizecounter_in_0 = 0,
	dns_sizecounter_in_16 = 1,
	dns_sizecounter_in_32 = 2,
	dns_sizecounter_in_48 = 3,
	dns_sizecounter_in_64 = 4,
	dns_sizecounter_in_80 = 5,
	dns_sizecounter_in_96 = 6,
	dns_sizecounter_in_112 = 7,
	dns_sizecounter_in_128 = 8,
	dns_sizecounter_in_144 = 9,
	dns_sizecounter_in_160 = 10,
	dns_sizecounter_in_176 = 11,
	dns_sizecounter_in_192 = 12,
	dns_sizecounter_in_208 = 13,
	dns_sizecounter_in_224 = 14,
	dns_sizecounter_in_240 = 15,
	dns_sizecounter_in_256 = 16,
	dns_sizecounter_in_272 = 17,
	dns_sizecounter_in_288 = 18,

	dns_sizecounter_in_max = 19,
};

enum {
	dns_sizecounter_out_0 = 0,
	dns_sizecounter_out_16 = 1,
	dns_sizecounter_out_32 = 2,
	dns_sizecounter_out_48 = 3,
	dns_sizecounter_out_64 = 4,
	dns_sizecounter_out_80 = 5,
	dns_sizecounter_out_96 = 6,
	dns_sizecounter_out_112 = 7,
	dns_sizecounter_out_128 = 8,
	dns_sizecounter_out_144 = 9,
	dns_sizecounter_out_160 = 10,
	dns_sizecounter_out_176 = 11,
	dns_sizecounter_out_192 = 12,
	dns_sizecounter_out_208 = 13,
	dns_sizecounter_out_224 = 14,
	dns_sizecounter_out_240 = 15,
	dns_sizecounter_out_256 = 16,
	dns_sizecounter_out_272 = 17,
	dns_sizecounter_out_288 = 18,
	dns_sizecounter_out_304 = 19,
	dns_sizecounter_out_320 = 20,
	dns_sizecounter_out_336 = 21,
	dns_sizecounter_out_352 = 22,
	dns_sizecounter_out_368 = 23,
	dns_sizecounter_out_384 = 24,
	dns_sizecounter_out_400 = 25,
	dns_sizecounter_out_416 = 26,
	dns_sizecounter_out_432 = 27,
	dns_sizecounter_out_448 = 28,
	dns_sizecounter_out_464 = 29,
	dns_sizecounter_out_480 = 30,
	dns_sizecounter_out_496 = 31,
	dns_sizecounter_out_512 = 32,
	dns_sizecounter_out_528 = 33,
	dns_sizecounter_out_544 = 34,
	dns_sizecounter_out_560 = 35,
	dns_sizecounter_out_576 = 36,
	dns_sizecounter_out_592 = 37,
	dns_sizecounter_out_608 = 38,
	dns_sizecounter_out_624 = 39,
	dns_sizecounter_out_640 = 40,
	dns_sizecounter_out_656 = 41,
	dns_sizecounter_out_672 = 42,
	dns_sizecounter_out_688 = 43,
	dns_sizecounter_out_704 = 44,
	dns_sizecounter_out_720 = 45,
	dns_sizecounter_out_736 = 46,
	dns_sizecounter_out_752 = 47,
	dns_sizecounter_out_768 = 48,
	dns_sizecounter_out_784 = 49,
	dns_sizecounter_out_800 = 50,
	dns_sizecounter_out_816 = 51,
	dns_sizecounter_out_832 = 52,
	dns_sizecounter_out_848 = 53,
	dns_sizecounter_out_864 = 54,
	dns_sizecounter_out_880 = 55,
	dns_sizecounter_out_896 = 56,
	dns_sizecounter_out_912 = 57,
	dns_sizecounter_out_928 = 58,
	dns_sizecounter_out_944 = 59,
	dns_sizecounter_out_960 = 60,
	dns_sizecounter_out_976 = 61,
	dns_sizecounter_out_992 = 62,
	dns_sizecounter_out_1008 = 63,
	dns_sizecounter_out_1024 = 64,
	dns_sizecounter_out_1040 = 65,
	dns_sizecounter_out_1056 = 66,
	dns_sizecounter_out_1072 = 67,
	dns_sizecounter_out_1088 = 68,
	dns_sizecounter_out_1104 = 69,
	dns_sizecounter_out_1120 = 70,
	dns_sizecounter_out_1136 = 71,
	dns_sizecounter_out_1152 = 72,
	dns_sizecounter_out_1168 = 73,
	dns_sizecounter_out_1184 = 74,
	dns_sizecounter_out_1200 = 75,
	dns_sizecounter_out_1216 = 76,
	dns_sizecounter_out_1232 = 77,
	dns_sizecounter_out_1248 = 78,
	dns_sizecounter_out_1264 = 79,
	dns_sizecounter_out_1280 = 80,
	dns_sizecounter_out_1296 = 81,
	dns_sizecounter_out_1312 = 82,
	dns_sizecounter_out_1328 = 83,
	dns_sizecounter_out_1344 = 84,
	dns_sizecounter_out_1360 = 85,
	dns_sizecounter_out_1376 = 86,
	dns_sizecounter_out_1392 = 87,
	dns_sizecounter_out_1408 = 88,
	dns_sizecounter_out_1424 = 89,
	dns_sizecounter_out_1440 = 90,
	dns_sizecounter_out_1456 = 91,
	dns_sizecounter_out_1472 = 92,
	dns_sizecounter_out_1488 = 93,
	dns_sizecounter_out_1504 = 94,
	dns_sizecounter_out_1520 = 95,
	dns_sizecounter_out_1536 = 96,
	dns_sizecounter_out_1552 = 97,
	dns_sizecounter_out_1568 = 98,
	dns_sizecounter_out_1584 = 99,
	dns_sizecounter_out_1600 = 100,
	dns_sizecounter_out_1616 = 101,
	dns_sizecounter_out_1632 = 102,
	dns_sizecounter_out_1648 = 103,
	dns_sizecounter_out_1664 = 104,
	dns_sizecounter_out_1680 = 105,
	dns_sizecounter_out_1696 = 106,
	dns_sizecounter_out_1712 = 107,
	dns_sizecounter_out_1728 = 108,
	dns_sizecounter_out_1744 = 109,
	dns_sizecounter_out_1760 = 110,
	dns_sizecounter_out_1776 = 111,
	dns_sizecounter_out_1792 = 112,
	dns_sizecounter_out_1808 = 113,
	dns_sizecounter_out_1824 = 114,
	dns_sizecounter_out_1840 = 115,
	dns_sizecounter_out_1856 = 116,
	dns_sizecounter_out_1872 = 117,
	dns_sizecounter_out_1888 = 118,
	dns_sizecounter_out_1904 = 119,
	dns_sizecounter_out_1920 = 120,
	dns_sizecounter_out_1936 = 121,
	dns_sizecounter_out_1952 = 122,
	dns_sizecounter_out_1968 = 123,
	dns_sizecounter_out_1984 = 124,
	dns_sizecounter_out_2000 = 125,
	dns_sizecounter_out_2016 = 126,
	dns_sizecounter_out_2032 = 127,
	dns_sizecounter_out_2048 = 128,
	dns_sizecounter_out_2064 = 129,
	dns_sizecounter_out_2080 = 130,
	dns_sizecounter_out_2096 = 131,
	dns_sizecounter_out_2112 = 132,
	dns_sizecounter_out_2128 = 133,
	dns_sizecounter_out_2144 = 134,
	dns_sizecounter_out_2160 = 135,
	dns_sizecounter_out_2176 = 136,
	dns_sizecounter_out_2192 = 137,
	dns_sizecounter_out_2208 = 138,
	dns_sizecounter_out_2224 = 139,
	dns_sizecounter_out_2240 = 140,
	dns_sizecounter_out_2256 = 141,
	dns_sizecounter_out_2272 = 142,
	dns_sizecounter_out_2288 = 143,
	dns_sizecounter_out_2304 = 144,
	dns_sizecounter_out_2320 = 145,
	dns_sizecounter_out_2336 = 146,
	dns_sizecounter_out_2352 = 147,
	dns_sizecounter_out_2368 = 148,
	dns_sizecounter_out_2384 = 149,
	dns_sizecounter_out_2400 = 150,
	dns_sizecounter_out_2416 = 151,
	dns_sizecounter_out_2432 = 152,
	dns_sizecounter_out_2448 = 153,
	dns_sizecounter_out_2464 = 154,
	dns_sizecounter_out_2480 = 155,
	dns_sizecounter_out_2496 = 156,
	dns_sizecounter_out_2512 = 157,
	dns_sizecounter_out_2528 = 158,
	dns_sizecounter_out_2544 = 159,
	dns_sizecounter_out_2560 = 160,
	dns_sizecounter_out_2576 = 161,
	dns_sizecounter_out_2592 = 162,
	dns_sizecounter_out_2608 = 163,
	dns_sizecounter_out_2624 = 164,
	dns_sizecounter_out_2640 = 165,
	dns_sizecounter_out_2656 = 166,
	dns_sizecounter_out_2672 = 167,
	dns_sizecounter_out_2688 = 168,
	dns_sizecounter_out_2704 = 169,
	dns_sizecounter_out_2720 = 170,
	dns_sizecounter_out_2736 = 171,
	dns_sizecounter_out_2752 = 172,
	dns_sizecounter_out_2768 = 173,
	dns_sizecounter_out_2784 = 174,
	dns_sizecounter_out_2800 = 175,
	dns_sizecounter_out_2816 = 176,
	dns_sizecounter_out_2832 = 177,
	dns_sizecounter_out_2848 = 178,
	dns_sizecounter_out_2864 = 179,
	dns_sizecounter_out_2880 = 180,
	dns_sizecounter_out_2896 = 181,
	dns_sizecounter_out_2912 = 182,
	dns_sizecounter_out_2928 = 183,
	dns_sizecounter_out_2944 = 184,
	dns_sizecounter_out_2960 = 185,
	dns_sizecounter_out_2976 = 186,
	dns_sizecounter_out_2992 = 187,
	dns_sizecounter_out_3008 = 188,
	dns_sizecounter_out_3024 = 189,
	dns_sizecounter_out_3040 = 190,
	dns_sizecounter_out_3056 = 191,
	dns_sizecounter_out_3072 = 192,
	dns_sizecounter_out_3088 = 193,
	dns_sizecounter_out_3104 = 194,
	dns_sizecounter_out_3120 = 195,
	dns_sizecounter_out_3136 = 196,
	dns_sizecounter_out_3152 = 197,
	dns_sizecounter_out_3168 = 198,
	dns_sizecounter_out_3184 = 199,
	dns_sizecounter_out_3200 = 200,
	dns_sizecounter_out_3216 = 201,
	dns_sizecounter_out_3232 = 202,
	dns_sizecounter_out_3248 = 203,
	dns_sizecounter_out_3264 = 204,
	dns_sizecounter_out_3280 = 205,
	dns_sizecounter_out_3296 = 206,
	dns_sizecounter_out_3312 = 207,
	dns_sizecounter_out_3328 = 208,
	dns_sizecounter_out_3344 = 209,
	dns_sizecounter_out_3360 = 210,
	dns_sizecounter_out_3376 = 211,
	dns_sizecounter_out_3392 = 212,
	dns_sizecounter_out_3408 = 213,
	dns_sizecounter_out_3424 = 214,
	dns_sizecounter_out_3440 = 215,
	dns_sizecounter_out_3456 = 216,
	dns_sizecounter_out_3472 = 217,
	dns_sizecounter_out_3488 = 218,
	dns_sizecounter_out_3504 = 219,
	dns_sizecounter_out_3520 = 220,
	dns_sizecounter_out_3536 = 221,
	dns_sizecounter_out_3552 = 222,
	dns_sizecounter_out_3568 = 223,
	dns_sizecounter_out_3584 = 224,
	dns_sizecounter_out_3600 = 225,
	dns_sizecounter_out_3616 = 226,
	dns_sizecounter_out_3632 = 227,
	dns_sizecounter_out_3648 = 228,
	dns_sizecounter_out_3664 = 229,
	dns_sizecounter_out_3680 = 230,
	dns_sizecounter_out_3696 = 231,
	dns_sizecounter_out_3712 = 232,
	dns_sizecounter_out_3728 = 233,
	dns_sizecounter_out_3744 = 234,
	dns_sizecounter_out_3760 = 235,
	dns_sizecounter_out_3776 = 236,
	dns_sizecounter_out_3792 = 237,
	dns_sizecounter_out_3808 = 238,
	dns_sizecounter_out_3824 = 239,
	dns_sizecounter_out_3840 = 240,
	dns_sizecounter_out_3856 = 241,
	dns_sizecounter_out_3872 = 242,
	dns_sizecounter_out_3888 = 243,
	dns_sizecounter_out_3904 = 244,
	dns_sizecounter_out_3920 = 245,
	dns_sizecounter_out_3936 = 246,
	dns_sizecounter_out_3952 = 247,
	dns_sizecounter_out_3968 = 248,
	dns_sizecounter_out_3984 = 249,
	dns_sizecounter_out_4000 = 250,
	dns_sizecounter_out_4016 = 251,
	dns_sizecounter_out_4032 = 252,
	dns_sizecounter_out_4048 = 253,
	dns_sizecounter_out_4064 = 254,
	dns_sizecounter_out_4080 = 255,
	dns_sizecounter_out_4096 = 256,

	dns_sizecounter_out_max = 257
};

#define DNS_STATS_NCOUNTERS 8

#if 0
/*%<
 * Flag(s) for dns_xxxstats_dump().  DNS_STATSDUMP_VERBOSE is obsolete.
 * ISC_STATSDUMP_VERBOSE should be used instead.  These two values are
 * intentionally defined to be the same value to ensure binary compatibility.
 */
#define DNS_STATSDUMP_VERBOSE	0x00000001 /*%< dump 0-value counters */
#endif

/*%<
 * (Obsoleted)
 */
LIBDNS_EXTERNAL_DATA extern const char *dns_statscounter_names[];

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
 *	RRset type counters only.  This indicates a record that marked for
 *	removal.
 *
 *	Note: incrementing _STALE will decrement the corresponding non-stale
 *	counter.
 */
#define DNS_RDATASTATSTYPE_ATTR_OTHERTYPE	0x0001
#define DNS_RDATASTATSTYPE_ATTR_NXRRSET		0x0002
#define DNS_RDATASTATSTYPE_ATTR_NXDOMAIN	0x0004
#define DNS_RDATASTATSTYPE_ATTR_STALE		0x0008

/*%<
 * Conversion macros among dns_rdatatype_t, attributes and isc_statscounter_t.
 */
#define DNS_RDATASTATSTYPE_BASE(type)	((dns_rdatatype_t)((type) & 0xFFFF))
#define DNS_RDATASTATSTYPE_ATTR(type)	((type) >> 16)
#define DNS_RDATASTATSTYPE_VALUE(b, a)	(((a) << 16) | (b))

/*%<
 * Types of dump callbacks.
 */
typedef void (*dns_generalstats_dumper_t)(isc_statscounter_t, isc_uint64_t,
					  void *);
typedef void (*dns_rdatatypestats_dumper_t)(dns_rdatastatstype_t, isc_uint64_t,
					    void *);
typedef void (*dns_opcodestats_dumper_t)(dns_opcode_t, isc_uint64_t, void *);

typedef void (*dns_rcodestats_dumper_t)(dns_rcode_t, isc_uint64_t, void *);

ISC_LANG_BEGINDECLS

isc_result_t
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
 *
 * Returns:
 *\li	ISC_R_SUCCESS	-- all ok
 *
 *\li	anything else	-- failure
 */

isc_result_t
dns_rdatatypestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per rdatatype.
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

isc_result_t
dns_rdatasetstats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per RRset.
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

isc_result_t
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per opcode.
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

isc_result_t
dns_rcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp);
/*%<
 * Create a statistics counter structure per assigned rcode.
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

isc_result_t
dns_stats_alloccounters(isc_mem_t *mctx, isc_uint64_t **ctrp);
/*%<
 * Allocate an array of query statistics counters from the memory
 * context 'mctx'.
 *
 * This function is obsoleted.  Use dns_xxxstats_create() instead.
 */

void
dns_stats_freecounters(isc_mem_t *mctx, isc_uint64_t **ctrp);
/*%<
 * Free an array of query statistics counters allocated from the memory
 * context 'mctx'.
 *
 * This function is obsoleted.  Use dns_stats_destroy() instead.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_STATS_H */
