/*	$NetBSD: statschannel.c,v 1.16 2025/01/26 16:24:33 christos Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/httpd.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/cache.h>
#include <dns/db.h>
#include <dns/opcode.h>
#include <dns/rcode.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/stats.h>
#include <dns/transport.h>
#include <dns/view.h>
#include <dns/xfrin.h>
#include <dns/zt.h>

#include <ns/stats.h>

#include <named/log.h>
#include <named/server.h>
#include <named/statschannel.h>

#if HAVE_JSON_C
#include <json_object.h>
#include <linkhash.h>
#endif /* HAVE_JSON_C */

#if HAVE_LIBXML2
#include <libxml/xmlwriter.h>
#define ISC_XMLCHAR (const xmlChar *)
#endif /* HAVE_LIBXML2 */

#include "xsl_p.h"

#define STATS_XML_VERSION_MAJOR "3"
#define STATS_XML_VERSION_MINOR "14"
#define STATS_XML_VERSION	STATS_XML_VERSION_MAJOR "." STATS_XML_VERSION_MINOR

#define STATS_JSON_VERSION_MAJOR "1"
#define STATS_JSON_VERSION_MINOR "8"
#define STATS_JSON_VERSION	 STATS_JSON_VERSION_MAJOR "." STATS_JSON_VERSION_MINOR

#define CHECK(m)                               \
	do {                                   \
		result = (m);                  \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

struct named_statschannel {
	/* Unlocked */
	isc_httpdmgr_t *httpdmgr;
	isc_sockaddr_t address;
	isc_mem_t *mctx;

	/*
	 * Locked by channel lock
	 */
	isc_mutex_t lock;
	dns_acl_t *acl;

	/* Locked by main loop. */
	ISC_LINK(struct named_statschannel) link;
};

typedef struct stats_dumparg {
	isc_statsformat_t type;
	void *arg;		 /* type dependent argument */
	int ncounters;		 /* for general statistics */
	int *counterindices;	 /* for general statistics */
	uint64_t *countervalues; /* for general statistics */
	isc_result_t result;
} stats_dumparg_t;

static isc_once_t once = ISC_ONCE_INIT;

#if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C)
#define EXTENDED_STATS
#else /* if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C) */
#undef EXTENDED_STATS
#endif /* if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C) */

#ifdef EXTENDED_STATS
static const char *
user_zonetype(dns_zone_t *zone) {
	dns_zonetype_t ztype;
	dns_view_t *view;
	static const struct zt {
		const dns_zonetype_t type;
		const char *const string;
	} typemap[] = { { dns_zone_none, "none" },
			{ dns_zone_primary, "primary" },
			{ dns_zone_secondary, "secondary" },
			{ dns_zone_mirror, "mirror" },
			{ dns_zone_stub, "stub" },
			{ dns_zone_staticstub, "static-stub" },
			{ dns_zone_key, "key" },
			{ dns_zone_dlz, "dlz" },
			{ dns_zone_redirect, "redirect" },
			{ 0, NULL } };
	const struct zt *tp;

	if ((dns_zone_getoptions(zone) & DNS_ZONEOPT_AUTOEMPTY) != 0) {
		return "builtin";
	}

	view = dns_zone_getview(zone);
	if (view != NULL && strcmp(view->name, "_bind") == 0) {
		return "builtin";
	}

	ztype = dns_zone_gettype(zone);
	for (tp = typemap; tp->string != NULL && tp->type != ztype; tp++) {
		/* empty */
	}
	return tp->string;
}
#endif /* ifdef EXTENDED_STATS */

/*%
 * Statistics descriptions.  These could be statistically initialized at
 * compile time, but we configure them run time in the init_desc() function
 * below so that they'll be less susceptible to counter name changes.
 */
static const char *nsstats_desc[ns_statscounter_max];
static const char *resstats_desc[dns_resstatscounter_max];
static const char *adbstats_desc[dns_adbstats_max];
static const char *zonestats_desc[dns_zonestatscounter_max];
static const char *sockstats_desc[isc_sockstatscounter_max];
static const char *dnssecstats_desc[dns_dnssecstats_max];
static const char *udpinsizestats_desc[dns_sizecounter_in_max];
static const char *udpoutsizestats_desc[dns_sizecounter_out_max];
static const char *tcpinsizestats_desc[dns_sizecounter_in_max];
static const char *tcpoutsizestats_desc[dns_sizecounter_out_max];
static const char *dnstapstats_desc[dns_dnstapcounter_max];
static const char *gluecachestats_desc[dns_gluecachestatscounter_max];
#if defined(EXTENDED_STATS)
static const char *nsstats_xmldesc[ns_statscounter_max];
static const char *resstats_xmldesc[dns_resstatscounter_max];
static const char *adbstats_xmldesc[dns_adbstats_max];
static const char *zonestats_xmldesc[dns_zonestatscounter_max];
static const char *sockstats_xmldesc[isc_sockstatscounter_max];
static const char *dnssecstats_xmldesc[dns_dnssecstats_max];
static const char *udpinsizestats_xmldesc[dns_sizecounter_in_max];
static const char *udpoutsizestats_xmldesc[dns_sizecounter_out_max];
static const char *tcpinsizestats_xmldesc[dns_sizecounter_in_max];
static const char *tcpoutsizestats_xmldesc[dns_sizecounter_out_max];
static const char *dnstapstats_xmldesc[dns_dnstapcounter_max];
static const char *gluecachestats_xmldesc[dns_gluecachestatscounter_max];
#else /* if defined(EXTENDED_STATS) */
#define nsstats_xmldesc		NULL
#define resstats_xmldesc	NULL
#define adbstats_xmldesc	NULL
#define zonestats_xmldesc	NULL
#define sockstats_xmldesc	NULL
#define dnssecstats_xmldesc	NULL
#define udpinsizestats_xmldesc	NULL
#define udpoutsizestats_xmldesc NULL
#define tcpinsizestats_xmldesc	NULL
#define tcpoutsizestats_xmldesc NULL
#define dnstapstats_xmldesc	NULL
#define gluecachestats_xmldesc	NULL
#endif /* EXTENDED_STATS */

#define TRY0(a)                       \
	do {                          \
		xmlrc = (a);          \
		if (xmlrc < 0)        \
			goto cleanup; \
	} while (0)

/*%
 * Mapping arrays to represent statistics counters in the order of our
 * preference, regardless of the order of counter indices.  For example,
 * nsstats_desc[nsstats_index[0]] will be the description that is shown first.
 */
static int nsstats_index[ns_statscounter_max];
static int resstats_index[dns_resstatscounter_max];
static int adbstats_index[dns_adbstats_max];
static int zonestats_index[dns_zonestatscounter_max];
static int sockstats_index[isc_sockstatscounter_max];
static int dnssecstats_index[dns_dnssecstats_max];
static int udpinsizestats_index[dns_sizecounter_in_max];
static int udpoutsizestats_index[dns_sizecounter_out_max];
static int tcpinsizestats_index[dns_sizecounter_in_max];
static int tcpoutsizestats_index[dns_sizecounter_out_max];
static int dnstapstats_index[dns_dnstapcounter_max];
static int gluecachestats_index[dns_gluecachestatscounter_max];

static void
set_desc(int counter, int maxcounter, const char *fdesc, const char **fdescs,
	 const char *xdesc, const char **xdescs) {
	REQUIRE(counter < maxcounter);
	REQUIRE(fdescs != NULL && fdescs[counter] == NULL);
#if defined(EXTENDED_STATS)
	REQUIRE(xdescs != NULL && xdescs[counter] == NULL);
#endif /* if defined(EXTENDED_STATS) */

	fdescs[counter] = fdesc;
#if defined(EXTENDED_STATS)
	xdescs[counter] = xdesc;
#else  /* if defined(EXTENDED_STATS) */
	UNUSED(xdesc);
	UNUSED(xdescs);
#endif /* if defined(EXTENDED_STATS) */
}

static const char *
get_histo_desc(const char *prefix, int i, int inf, bool ext) {
	static char buf[(DNS_SIZEHISTO_MAXIN + DNS_SIZEHISTO_MAXOUT) * 80];
	static size_t used = 0;
	char *desc = buf + used;
	size_t space = sizeof(buf) - used;
	int min = DNS_SIZEHISTO_QUANTUM * i;
	int max = DNS_SIZEHISTO_QUANTUM * (i + 1) - 1;
	int len = 0;

	if (!ext && i < inf) {
		len = snprintf(desc, space, "%s %u-%u bytes", prefix, min, max);
	} else if (!ext && i >= inf) {
		len = snprintf(desc, space, "%s %u+ bytes", prefix, min);
	} else if (ext && i < inf) {
		len = snprintf(desc, space, "%u-%u", min, max);
	} else if (ext && i >= inf) {
		len = snprintf(desc, space, "%u+", min);
	}
	INSIST(0 < len && (size_t)len < space);
	used += len + 1;
	return desc;
}

static void
init_desc(void) {
	int i;

	/* Initialize name server statistics */
	for (i = 0; i < ns_statscounter_max; i++) {
		nsstats_desc[i] = NULL;
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < ns_statscounter_max; i++) {
		nsstats_xmldesc[i] = NULL;
	}
#endif /* if defined(EXTENDED_STATS) */

#define SET_NSSTATDESC(counterid, desc, xmldesc)                           \
	do {                                                               \
		set_desc(ns_statscounter_##counterid, ns_statscounter_max, \
			 desc, nsstats_desc, xmldesc, nsstats_xmldesc);    \
		nsstats_index[i++] = ns_statscounter_##counterid;          \
	} while (0)

	i = 0;
	SET_NSSTATDESC(requestv4, "IPv4 requests received", "Requestv4");
	SET_NSSTATDESC(requestv6, "IPv6 requests received", "Requestv6");
	SET_NSSTATDESC(edns0in, "requests with EDNS(0) received", "ReqEdns0");
	SET_NSSTATDESC(badednsver,
		       "requests with unsupported EDNS version received",
		       "ReqBadEDNSVer");
	SET_NSSTATDESC(tsigin, "requests with TSIG received", "ReqTSIG");
	SET_NSSTATDESC(sig0in, "requests with SIG(0) received", "ReqSIG0");
	SET_NSSTATDESC(invalidsig, "requests with invalid signature",
		       "ReqBadSIG");
	SET_NSSTATDESC(requesttcp, "TCP requests received", "ReqTCP");
	SET_NSSTATDESC(tcphighwater, "TCP connection high-water",
		       "TCPConnHighWater");
	SET_NSSTATDESC(authrej, "auth queries rejected", "AuthQryRej");
	SET_NSSTATDESC(recurserej, "recursive queries rejected", "RecQryRej");
	SET_NSSTATDESC(xfrrej, "transfer requests rejected", "XfrRej");
	SET_NSSTATDESC(updaterej, "update requests rejected", "UpdateRej");
	SET_NSSTATDESC(response, "responses sent", "Response");
	SET_NSSTATDESC(truncatedresp, "truncated responses sent",
		       "TruncatedResp");
	SET_NSSTATDESC(edns0out, "responses with EDNS(0) sent", "RespEDNS0");
	SET_NSSTATDESC(tsigout, "responses with TSIG sent", "RespTSIG");
	SET_NSSTATDESC(sig0out, "responses with SIG(0) sent", "RespSIG0");
	SET_NSSTATDESC(success, "queries resulted in successful answer",
		       "QrySuccess");
	SET_NSSTATDESC(authans, "queries resulted in authoritative answer",
		       "QryAuthAns");
	SET_NSSTATDESC(nonauthans,
		       "queries resulted in non authoritative answer",
		       "QryNoauthAns");
	SET_NSSTATDESC(referral, "queries resulted in referral answer",
		       "QryReferral");
	SET_NSSTATDESC(nxrrset, "queries resulted in nxrrset", "QryNxrrset");
	SET_NSSTATDESC(servfail, "queries resulted in SERVFAIL", "QrySERVFAIL");
	SET_NSSTATDESC(formerr, "queries resulted in FORMERR", "QryFORMERR");
	SET_NSSTATDESC(nxdomain, "queries resulted in NXDOMAIN", "QryNXDOMAIN");
	SET_NSSTATDESC(recursion, "queries caused recursion", "QryRecursion");
	SET_NSSTATDESC(duplicate, "duplicate queries received", "QryDuplicate");
	SET_NSSTATDESC(dropped, "queries dropped", "QryDropped");
	SET_NSSTATDESC(failure, "other query failures", "QryFailure");
	SET_NSSTATDESC(xfrdone, "requested transfers completed", "XfrReqDone");
	SET_NSSTATDESC(updatereqfwd, "update requests forwarded",
		       "UpdateReqFwd");
	SET_NSSTATDESC(updaterespfwd, "update responses forwarded",
		       "UpdateRespFwd");
	SET_NSSTATDESC(updatefwdfail, "update forward failed", "UpdateFwdFail");
	SET_NSSTATDESC(updatedone, "updates completed", "UpdateDone");
	SET_NSSTATDESC(updatefail, "updates failed", "UpdateFail");
	SET_NSSTATDESC(updatebadprereq,
		       "updates rejected due to prerequisite failure",
		       "UpdateBadPrereq");
	SET_NSSTATDESC(recurshighwater, "Recursive clients high-water",
		       "RecursHighwater");
	SET_NSSTATDESC(recursclients, "recursing clients", "RecursClients");
	SET_NSSTATDESC(dns64, "queries answered by DNS64", "DNS64");
	SET_NSSTATDESC(ratedropped, "responses dropped for rate limits",
		       "RateDropped");
	SET_NSSTATDESC(rateslipped, "responses truncated for rate limits",
		       "RateSlipped");
	SET_NSSTATDESC(rpz_rewrites, "response policy zone rewrites",
		       "RPZRewrites");
	SET_NSSTATDESC(udp, "UDP queries received", "QryUDP");
	SET_NSSTATDESC(tcp, "TCP queries received", "QryTCP");
	SET_NSSTATDESC(nsidopt, "NSID option received", "NSIDOpt");
	SET_NSSTATDESC(expireopt, "Expire option received", "ExpireOpt");
	SET_NSSTATDESC(keepaliveopt, "EDNS TCP keepalive option received",
		       "KeepAliveOpt");
	SET_NSSTATDESC(padopt, "EDNS padding option received", "PadOpt");
	SET_NSSTATDESC(otheropt, "Other EDNS option received", "OtherOpt");
	SET_NSSTATDESC(cookiein, "COOKIE option received", "CookieIn");
	SET_NSSTATDESC(cookienew, "COOKIE - client only", "CookieNew");
	SET_NSSTATDESC(cookiebadsize, "COOKIE - bad size", "CookieBadSize");
	SET_NSSTATDESC(cookiebadtime, "COOKIE - bad time", "CookieBadTime");
	SET_NSSTATDESC(cookienomatch, "COOKIE - no match", "CookieNoMatch");
	SET_NSSTATDESC(cookiematch, "COOKIE - match", "CookieMatch");
	SET_NSSTATDESC(ecsopt, "EDNS client subnet option received", "ECSOpt");
	SET_NSSTATDESC(nxdomainredirect,
		       "queries resulted in NXDOMAIN that were redirected",
		       "QryNXRedir");
	SET_NSSTATDESC(nxdomainredirect_rlookup,
		       "queries resulted in NXDOMAIN that were redirected and "
		       "resulted in a successful remote lookup",
		       "QryNXRedirRLookup");
	SET_NSSTATDESC(badcookie, "sent badcookie response", "QryBADCOOKIE");
	SET_NSSTATDESC(nxdomainsynth, "synthesized a NXDOMAIN response",
		       "SynthNXDOMAIN");
	SET_NSSTATDESC(nodatasynth, "synthesized a no-data response",
		       "SynthNODATA");
	SET_NSSTATDESC(wildcardsynth, "synthesized a wildcard response",
		       "SynthWILDCARD");
	SET_NSSTATDESC(trystale,
		       "attempts to use stale cache data after lookup failure",
		       "QryTryStale");
	SET_NSSTATDESC(usedstale,
		       "successful uses of stale cache data after lookup "
		       "failure",
		       "QryUsedStale");
	SET_NSSTATDESC(prefetch, "queries triggered prefetch", "Prefetch");
	SET_NSSTATDESC(keytagopt, "Keytag option received", "KeyTagOpt");
	SET_NSSTATDESC(reclimitdropped,
		       "queries dropped due to recursive client limit",
		       "RecLimitDropped");
	SET_NSSTATDESC(updatequota, "Update quota exceeded", "UpdateQuota");

	INSIST(i == ns_statscounter_max);

	/* Initialize resolver statistics */
	for (i = 0; i < dns_resstatscounter_max; i++) {
		resstats_desc[i] = NULL;
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < dns_resstatscounter_max; i++) {
		resstats_xmldesc[i] = NULL;
	}
#endif /* if defined(EXTENDED_STATS) */

#define SET_RESSTATDESC(counterid, desc, xmldesc)                      \
	do {                                                           \
		set_desc(dns_resstatscounter_##counterid,              \
			 dns_resstatscounter_max, desc, resstats_desc, \
			 xmldesc, resstats_xmldesc);                   \
		resstats_index[i++] = dns_resstatscounter_##counterid; \
	} while (0)

	i = 0;
	SET_RESSTATDESC(queryv4, "IPv4 queries sent", "Queryv4");
	SET_RESSTATDESC(queryv6, "IPv6 queries sent", "Queryv6");
	SET_RESSTATDESC(responsev4, "IPv4 responses received", "Responsev4");
	SET_RESSTATDESC(responsev6, "IPv6 responses received", "Responsev6");
	SET_RESSTATDESC(nxdomain, "NXDOMAIN received", "NXDOMAIN");
	SET_RESSTATDESC(servfail, "SERVFAIL received", "SERVFAIL");
	SET_RESSTATDESC(formerr, "FORMERR received", "FORMERR");
	SET_RESSTATDESC(othererror, "other errors received", "OtherError");
	SET_RESSTATDESC(edns0fail, "EDNS(0) query failures", "EDNS0Fail");
	SET_RESSTATDESC(mismatch, "mismatch responses received", "Mismatch");
	SET_RESSTATDESC(truncated, "truncated responses received", "Truncated");
	SET_RESSTATDESC(lame, "lame delegations received", "Lame");
	SET_RESSTATDESC(retry, "query retries", "Retry");
	SET_RESSTATDESC(dispabort, "queries aborted due to quota",
			"QueryAbort");
	SET_RESSTATDESC(dispsockfail, "failures in opening query sockets",
			"QuerySockFail");
	SET_RESSTATDESC(disprequdp, "UDP queries in progress", "QueryCurUDP");
	SET_RESSTATDESC(dispreqtcp, "TCP queries in progress", "QueryCurTCP");
	SET_RESSTATDESC(querytimeout, "query timeouts", "QueryTimeout");
	SET_RESSTATDESC(gluefetchv4, "IPv4 NS address fetches", "GlueFetchv4");
	SET_RESSTATDESC(gluefetchv6, "IPv6 NS address fetches", "GlueFetchv6");
	SET_RESSTATDESC(gluefetchv4fail, "IPv4 NS address fetch failed",
			"GlueFetchv4Fail");
	SET_RESSTATDESC(gluefetchv6fail, "IPv6 NS address fetch failed",
			"GlueFetchv6Fail");
	SET_RESSTATDESC(val, "DNSSEC validation attempted", "ValAttempt");
	SET_RESSTATDESC(valsuccess, "DNSSEC validation succeeded", "ValOk");
	SET_RESSTATDESC(valnegsuccess, "DNSSEC NX validation succeeded",
			"ValNegOk");
	SET_RESSTATDESC(valfail, "DNSSEC validation failed", "ValFail");
	SET_RESSTATDESC(queryrtt0,
			"queries with RTT < " DNS_RESOLVER_QRYRTTCLASS0STR "ms",
			"QryRTT" DNS_RESOLVER_QRYRTTCLASS0STR);
	SET_RESSTATDESC(queryrtt1,
			"queries with RTT " DNS_RESOLVER_QRYRTTCLASS0STR
			"-" DNS_RESOLVER_QRYRTTCLASS1STR "ms",
			"QryRTT" DNS_RESOLVER_QRYRTTCLASS1STR);
	SET_RESSTATDESC(queryrtt2,
			"queries with RTT " DNS_RESOLVER_QRYRTTCLASS1STR
			"-" DNS_RESOLVER_QRYRTTCLASS2STR "ms",
			"QryRTT" DNS_RESOLVER_QRYRTTCLASS2STR);
	SET_RESSTATDESC(queryrtt3,
			"queries with RTT " DNS_RESOLVER_QRYRTTCLASS2STR
			"-" DNS_RESOLVER_QRYRTTCLASS3STR "ms",
			"QryRTT" DNS_RESOLVER_QRYRTTCLASS3STR);
	SET_RESSTATDESC(queryrtt4,
			"queries with RTT " DNS_RESOLVER_QRYRTTCLASS3STR
			"-" DNS_RESOLVER_QRYRTTCLASS4STR "ms",
			"QryRTT" DNS_RESOLVER_QRYRTTCLASS4STR);
	SET_RESSTATDESC(queryrtt5,
			"queries with RTT > " DNS_RESOLVER_QRYRTTCLASS4STR "ms",
			"QryRTT" DNS_RESOLVER_QRYRTTCLASS4STR "+");
	SET_RESSTATDESC(nfetch, "active fetches", "NumFetch");
	SET_RESSTATDESC(buckets, "bucket size", "BucketSize");
	SET_RESSTATDESC(refused, "REFUSED received", "REFUSED");
	SET_RESSTATDESC(cookienew, "COOKIE send with client cookie only",
			"ClientCookieOut");
	SET_RESSTATDESC(cookieout, "COOKIE sent with client and server cookie",
			"ServerCookieOut");
	SET_RESSTATDESC(cookiein, "COOKIE replies received", "CookieIn");
	SET_RESSTATDESC(cookieok, "COOKIE client ok", "CookieClientOk");
	SET_RESSTATDESC(badvers, "bad EDNS version", "BadEDNSVersion");
	SET_RESSTATDESC(badcookie, "bad cookie rcode", "BadCookieRcode");
	SET_RESSTATDESC(zonequota, "spilled due to zone quota", "ZoneQuota");
	SET_RESSTATDESC(serverquota, "spilled due to server quota",
			"ServerQuota");
	SET_RESSTATDESC(clientquota, "spilled due to clients per query quota",
			"ClientQuota");
	SET_RESSTATDESC(nextitem, "waited for next item", "NextItem");
	SET_RESSTATDESC(priming, "priming queries", "Priming");

	INSIST(i == dns_resstatscounter_max);

	/* Initialize adb statistics */
	for (i = 0; i < dns_adbstats_max; i++) {
		adbstats_desc[i] = NULL;
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < dns_adbstats_max; i++) {
		adbstats_xmldesc[i] = NULL;
	}
#endif /* if defined(EXTENDED_STATS) */

#define SET_ADBSTATDESC(id, desc, xmldesc)                          \
	do {                                                        \
		set_desc(dns_adbstats_##id, dns_adbstats_max, desc, \
			 adbstats_desc, xmldesc, adbstats_xmldesc); \
		adbstats_index[i++] = dns_adbstats_##id;            \
	} while (0)
	i = 0;
	SET_ADBSTATDESC(nentries, "Address hash table size", "nentries");
	SET_ADBSTATDESC(entriescnt, "Addresses in hash table", "entriescnt");
	SET_ADBSTATDESC(nnames, "Name hash table size", "nnames");
	SET_ADBSTATDESC(namescnt, "Names in hash table", "namescnt");

	INSIST(i == dns_adbstats_max);

	/* Initialize zone statistics */
	for (i = 0; i < dns_zonestatscounter_max; i++) {
		zonestats_desc[i] = NULL;
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < dns_zonestatscounter_max; i++) {
		zonestats_xmldesc[i] = NULL;
	}
#endif /* if defined(EXTENDED_STATS) */

#define SET_ZONESTATDESC(counterid, desc, xmldesc)                       \
	do {                                                             \
		set_desc(dns_zonestatscounter_##counterid,               \
			 dns_zonestatscounter_max, desc, zonestats_desc, \
			 xmldesc, zonestats_xmldesc);                    \
		zonestats_index[i++] = dns_zonestatscounter_##counterid; \
	} while (0)

	i = 0;
	SET_ZONESTATDESC(notifyoutv4, "IPv4 notifies sent", "NotifyOutv4");
	SET_ZONESTATDESC(notifyoutv6, "IPv6 notifies sent", "NotifyOutv6");
	SET_ZONESTATDESC(notifyinv4, "IPv4 notifies received", "NotifyInv4");
	SET_ZONESTATDESC(notifyinv6, "IPv6 notifies received", "NotifyInv6");
	SET_ZONESTATDESC(notifyrej, "notifies rejected", "NotifyRej");
	SET_ZONESTATDESC(soaoutv4, "IPv4 SOA queries sent", "SOAOutv4");
	SET_ZONESTATDESC(soaoutv6, "IPv6 SOA queries sent", "SOAOutv6");
	SET_ZONESTATDESC(axfrreqv4, "IPv4 AXFR requested", "AXFRReqv4");
	SET_ZONESTATDESC(axfrreqv6, "IPv6 AXFR requested", "AXFRReqv6");
	SET_ZONESTATDESC(ixfrreqv4, "IPv4 IXFR requested", "IXFRReqv4");
	SET_ZONESTATDESC(ixfrreqv6, "IPv6 IXFR requested", "IXFRReqv6");
	SET_ZONESTATDESC(xfrsuccess, "transfer requests succeeded",
			 "XfrSuccess");
	SET_ZONESTATDESC(xfrfail, "transfer requests failed", "XfrFail");
	INSIST(i == dns_zonestatscounter_max);

	/* Initialize socket statistics */
	for (i = 0; i < isc_sockstatscounter_max; i++) {
		sockstats_desc[i] = NULL;
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < isc_sockstatscounter_max; i++) {
		sockstats_xmldesc[i] = NULL;
	}
#endif /* if defined(EXTENDED_STATS) */

#define SET_SOCKSTATDESC(counterid, desc, xmldesc)                       \
	do {                                                             \
		set_desc(isc_sockstatscounter_##counterid,               \
			 isc_sockstatscounter_max, desc, sockstats_desc, \
			 xmldesc, sockstats_xmldesc);                    \
		sockstats_index[i++] = isc_sockstatscounter_##counterid; \
	} while (0)

	i = 0;
	SET_SOCKSTATDESC(udp4open, "UDP/IPv4 sockets opened", "UDP4Open");
	SET_SOCKSTATDESC(udp6open, "UDP/IPv6 sockets opened", "UDP6Open");
	SET_SOCKSTATDESC(tcp4open, "TCP/IPv4 sockets opened", "TCP4Open");
	SET_SOCKSTATDESC(tcp6open, "TCP/IPv6 sockets opened", "TCP6Open");
	SET_SOCKSTATDESC(udp4openfail, "UDP/IPv4 socket open failures",
			 "UDP4OpenFail");
	SET_SOCKSTATDESC(udp6openfail, "UDP/IPv6 socket open failures",
			 "UDP6OpenFail");
	SET_SOCKSTATDESC(tcp4openfail, "TCP/IPv4 socket open failures",
			 "TCP4OpenFail");
	SET_SOCKSTATDESC(tcp6openfail, "TCP/IPv6 socket open failures",
			 "TCP6OpenFail");
	SET_SOCKSTATDESC(udp4close, "UDP/IPv4 sockets closed", "UDP4Close");
	SET_SOCKSTATDESC(udp6close, "UDP/IPv6 sockets closed", "UDP6Close");
	SET_SOCKSTATDESC(tcp4close, "TCP/IPv4 sockets closed", "TCP4Close");
	SET_SOCKSTATDESC(tcp6close, "TCP/IPv6 sockets closed", "TCP6Close");
	SET_SOCKSTATDESC(udp4bindfail, "UDP/IPv4 socket bind failures",
			 "UDP4BindFail");
	SET_SOCKSTATDESC(udp6bindfail, "UDP/IPv6 socket bind failures",
			 "UDP6BindFail");
	SET_SOCKSTATDESC(tcp4bindfail, "TCP/IPv4 socket bind failures",
			 "TCP4BindFail");
	SET_SOCKSTATDESC(tcp6bindfail, "TCP/IPv6 socket bind failures",
			 "TCP6BindFail");
	SET_SOCKSTATDESC(udp4connectfail, "UDP/IPv4 socket connect failures",
			 "UDP4ConnFail");
	SET_SOCKSTATDESC(udp6connectfail, "UDP/IPv6 socket connect failures",
			 "UDP6ConnFail");
	SET_SOCKSTATDESC(tcp4connectfail, "TCP/IPv4 socket connect failures",
			 "TCP4ConnFail");
	SET_SOCKSTATDESC(tcp6connectfail, "TCP/IPv6 socket connect failures",
			 "TCP6ConnFail");
	SET_SOCKSTATDESC(udp4connect, "UDP/IPv4 connections established",
			 "UDP4Conn");
	SET_SOCKSTATDESC(udp6connect, "UDP/IPv6 connections established",
			 "UDP6Conn");
	SET_SOCKSTATDESC(tcp4connect, "TCP/IPv4 connections established",
			 "TCP4Conn");
	SET_SOCKSTATDESC(tcp6connect, "TCP/IPv6 connections established",
			 "TCP6Conn");
	SET_SOCKSTATDESC(tcp4acceptfail, "TCP/IPv4 connection accept failures",
			 "TCP4AcceptFail");
	SET_SOCKSTATDESC(tcp6acceptfail, "TCP/IPv6 connection accept failures",
			 "TCP6AcceptFail");
	SET_SOCKSTATDESC(tcp4accept, "TCP/IPv4 connections accepted",
			 "TCP4Accept");
	SET_SOCKSTATDESC(tcp6accept, "TCP/IPv6 connections accepted",
			 "TCP6Accept");
	SET_SOCKSTATDESC(udp4sendfail, "UDP/IPv4 send errors", "UDP4SendErr");
	SET_SOCKSTATDESC(udp6sendfail, "UDP/IPv6 send errors", "UDP6SendErr");
	SET_SOCKSTATDESC(tcp4sendfail, "TCP/IPv4 send errors", "TCP4SendErr");
	SET_SOCKSTATDESC(tcp6sendfail, "TCP/IPv6 send errors", "TCP6SendErr");
	SET_SOCKSTATDESC(udp4recvfail, "UDP/IPv4 recv errors", "UDP4RecvErr");
	SET_SOCKSTATDESC(udp6recvfail, "UDP/IPv6 recv errors", "UDP6RecvErr");
	SET_SOCKSTATDESC(tcp4recvfail, "TCP/IPv4 recv errors", "TCP4RecvErr");
	SET_SOCKSTATDESC(tcp6recvfail, "TCP/IPv6 recv errors", "TCP6RecvErr");
	SET_SOCKSTATDESC(udp4active, "UDP/IPv4 sockets active", "UDP4Active");
	SET_SOCKSTATDESC(udp6active, "UDP/IPv6 sockets active", "UDP6Active");
	SET_SOCKSTATDESC(tcp4active, "TCP/IPv4 sockets active", "TCP4Active");
	SET_SOCKSTATDESC(tcp6active, "TCP/IPv6 sockets active", "TCP6Active");
	SET_SOCKSTATDESC(tcp4clients, "TCP/IPv4 clients currently connected",
			 "TCP4Clients");
	SET_SOCKSTATDESC(tcp6clients, "TCP/IPv6 clients currently connected",
			 "TCP6Clients");
	INSIST(i == isc_sockstatscounter_max);

	/* Initialize DNSSEC statistics */
	for (i = 0; i < dns_dnssecstats_max; i++) {
		dnssecstats_desc[i] = NULL;
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < dns_dnssecstats_max; i++) {
		dnssecstats_xmldesc[i] = NULL;
	}
#endif /* if defined(EXTENDED_STATS) */

#define SET_DNSSECSTATDESC(counterid, desc, xmldesc)                       \
	do {                                                               \
		set_desc(dns_dnssecstats_##counterid, dns_dnssecstats_max, \
			 desc, dnssecstats_desc, xmldesc,                  \
			 dnssecstats_xmldesc);                             \
		dnssecstats_index[i++] = dns_dnssecstats_##counterid;      \
	} while (0)

	i = 0;
	SET_DNSSECSTATDESC(asis,
			   "dnssec validation success with signer "
			   "\"as is\"",
			   "DNSSECasis");
	SET_DNSSECSTATDESC(downcase,
			   "dnssec validation success with signer "
			   "lower cased",
			   "DNSSECdowncase");
	SET_DNSSECSTATDESC(wildcard, "dnssec validation of wildcard signature",
			   "DNSSECwild");
	SET_DNSSECSTATDESC(fail, "dnssec validation failures", "DNSSECfail");
	INSIST(i == dns_dnssecstats_max);

	/* Initialize dnstap statistics */
	for (i = 0; i < dns_dnstapcounter_max; i++) {
		dnstapstats_desc[i] = NULL;
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < dns_dnstapcounter_max; i++) {
		dnstapstats_xmldesc[i] = NULL;
	}
#endif /* if defined(EXTENDED_STATS) */

#define SET_DNSTAPSTATDESC(counterid, desc, xmldesc)                           \
	do {                                                                   \
		set_desc(dns_dnstapcounter_##counterid, dns_dnstapcounter_max, \
			 desc, dnstapstats_desc, xmldesc,                      \
			 dnstapstats_xmldesc);                                 \
		dnstapstats_index[i++] = dns_dnstapcounter_##counterid;        \
	} while (0)
	i = 0;
	SET_DNSTAPSTATDESC(success, "dnstap messages written", "DNSTAPsuccess");
	SET_DNSTAPSTATDESC(drop, "dnstap messages dropped", "DNSTAPdropped");
	INSIST(i == dns_dnstapcounter_max);

#define SET_GLUECACHESTATDESC(counterid, desc, xmldesc)         \
	do {                                                    \
		set_desc(dns_gluecachestatscounter_##counterid, \
			 dns_gluecachestatscounter_max, desc,   \
			 gluecachestats_desc, xmldesc,          \
			 gluecachestats_xmldesc);               \
		gluecachestats_index[i++] =                     \
			dns_gluecachestatscounter_##counterid;  \
	} while (0)
	i = 0;
	SET_GLUECACHESTATDESC(hits_present, "Hits for present glue (cached)",
			      "GLUECACHEhitspresent");
	SET_GLUECACHESTATDESC(hits_absent,
			      "Hits for non-existent glue (cached)",
			      "GLUECACHEhitsabsent");
	SET_GLUECACHESTATDESC(inserts_present,
			      "Miss-plus-cache-inserts for present glue",
			      "GLUECACHEinsertspresent");
	SET_GLUECACHESTATDESC(inserts_absent,
			      "Miss-plus-cache-inserts for non-existent glue",
			      "GLUECACHEinsertsabsent");
	INSIST(i == dns_gluecachestatscounter_max);

	/* Sanity check */
	for (i = 0; i < ns_statscounter_max; i++) {
		INSIST(nsstats_desc[i] != NULL);
	}
	for (i = 0; i < dns_resstatscounter_max; i++) {
		INSIST(resstats_desc[i] != NULL);
	}
	for (i = 0; i < dns_adbstats_max; i++) {
		INSIST(adbstats_desc[i] != NULL);
	}
	for (i = 0; i < dns_zonestatscounter_max; i++) {
		INSIST(zonestats_desc[i] != NULL);
	}
	for (i = 0; i < isc_sockstatscounter_max; i++) {
		INSIST(sockstats_desc[i] != NULL);
	}
	for (i = 0; i < dns_dnssecstats_max; i++) {
		INSIST(dnssecstats_desc[i] != NULL);
	}
	for (i = 0; i < dns_dnstapcounter_max; i++) {
		INSIST(dnstapstats_desc[i] != NULL);
	}
	for (i = 0; i < dns_gluecachestatscounter_max; i++) {
		INSIST(gluecachestats_desc[i] != NULL);
	}
#if defined(EXTENDED_STATS)
	for (i = 0; i < ns_statscounter_max; i++) {
		INSIST(nsstats_xmldesc[i] != NULL);
	}
	for (i = 0; i < dns_resstatscounter_max; i++) {
		INSIST(resstats_xmldesc[i] != NULL);
	}
	for (i = 0; i < dns_adbstats_max; i++) {
		INSIST(adbstats_xmldesc[i] != NULL);
	}
	for (i = 0; i < dns_zonestatscounter_max; i++) {
		INSIST(zonestats_xmldesc[i] != NULL);
	}
	for (i = 0; i < isc_sockstatscounter_max; i++) {
		INSIST(sockstats_xmldesc[i] != NULL);
	}
	for (i = 0; i < dns_dnssecstats_max; i++) {
		INSIST(dnssecstats_xmldesc[i] != NULL);
	}
	for (i = 0; i < dns_dnstapcounter_max; i++) {
		INSIST(dnstapstats_xmldesc[i] != NULL);
	}
	for (i = 0; i < dns_gluecachestatscounter_max; i++) {
		INSIST(gluecachestats_xmldesc[i] != NULL);
	}
#endif /* if defined(EXTENDED_STATS) */

	/* Initialize traffic size statistics */

	for (i = 0; i < DNS_SIZEHISTO_MAXOUT; i++) {
		udpoutsizestats_index[i] = i;
		tcpoutsizestats_index[i] = i;
		udpoutsizestats_desc[i] = get_histo_desc(
			"responses sent", i, DNS_SIZEHISTO_MAXOUT, false);
		tcpoutsizestats_desc[i] = udpoutsizestats_desc[i];
#if defined(EXTENDED_STATS)
		udpoutsizestats_xmldesc[i] = get_histo_desc(
			"responses sent", i, DNS_SIZEHISTO_MAXOUT, true);
		tcpoutsizestats_xmldesc[i] = udpoutsizestats_xmldesc[i];
#endif /* if defined(EXTENDED_STATS) */
	}

	for (i = 0; i <= DNS_SIZEHISTO_MAXIN; i++) {
		udpinsizestats_index[i] = i;
		tcpinsizestats_index[i] = i;
		udpinsizestats_desc[i] = get_histo_desc(
			"requests received", i, DNS_SIZEHISTO_MAXIN, false);
		tcpinsizestats_desc[i] = udpinsizestats_desc[i];
#if defined(EXTENDED_STATS)
		if (i < DNS_SIZEHISTO_MAXIN) {
			udpinsizestats_xmldesc[i] = udpoutsizestats_xmldesc[i];
			tcpinsizestats_xmldesc[i] = tcpoutsizestats_xmldesc[i];
		} else {
			udpinsizestats_xmldesc[i] =
				get_histo_desc("requests received", i,
					       DNS_SIZEHISTO_MAXIN, true);
			tcpinsizestats_xmldesc[i] = udpinsizestats_xmldesc[i];
		}
#endif /* if defined(EXTENDED_STATS) */
	}
}

/*%
 * Dump callback functions.
 */

static isc_result_t
dump_counters(isc_statsformat_t type, void *arg, const char *category,
	      const char **desc, int ncounters, int *indices, uint64_t *values,
	      int options);

static void
generalstat_dump(isc_statscounter_t counter, uint64_t val, void *arg) {
	stats_dumparg_t *dumparg = arg;

	REQUIRE(counter < dumparg->ncounters);
	dumparg->countervalues[counter] = val;
}

static isc_result_t
dump_stats(isc_stats_t *stats, isc_statsformat_t type, void *arg,
	   const char *category, const char **desc, int ncounters, int *indices,
	   uint64_t *values, int options) {
	stats_dumparg_t dumparg;

	dumparg.type = type;
	dumparg.ncounters = ncounters;
	dumparg.counterindices = indices;
	dumparg.countervalues = values;

	memset(values, 0, sizeof(values[0]) * ncounters);
	isc_stats_dump(stats, generalstat_dump, &dumparg, options);

	return dump_counters(type, arg, category, desc, ncounters, indices,
			     values, options);
}

#if defined(EXTENDED_STATS)
static isc_result_t
dump_histo(isc_histomulti_t *hm, isc_statsformat_t type, void *arg,
	   const char *category, const char **desc, int ncounters, int *indices,
	   uint64_t *values, int options) {
	isc_histo_t *hg = NULL;

	isc_histomulti_merge(&hg, hm);
	for (int i = 0; i < ncounters; i++) {
		isc_histo_get(hg, i, NULL, NULL, &values[i]);
	}
	isc_histo_destroy(&hg);

	return dump_counters(type, arg, category, desc, ncounters, indices,
			     values, options);
}
#endif /* defined(EXTENDED_STATS) */

static isc_result_t
dump_counters(isc_statsformat_t type, void *arg, const char *category,
	      const char **desc, int ncounters, int *indices, uint64_t *values,
	      int options) {
	int i, idx;
	uint64_t value;
	FILE *fp;
#ifdef HAVE_LIBXML2
	void *writer;
	int xmlrc;
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	json_object *job, *cat, *counter;
#endif /* ifdef HAVE_JSON_C */

#if !defined(EXTENDED_STATS)
	UNUSED(category);
#endif /* if !defined(EXTENDED_STATS) */

#ifdef HAVE_JSON_C
	cat = job = (json_object *)arg;
	if (ncounters > 0 && type == isc_statsformat_json) {
		if (category != NULL) {
			cat = json_object_new_object();
			if (cat == NULL) {
				return ISC_R_NOMEMORY;
			}
			json_object_object_add(job, category, cat);
		}
	}
#endif /* ifdef HAVE_JSON_C */

	for (i = 0; i < ncounters; i++) {
		idx = indices[i];
		value = values[idx];

		if (value == 0 && (options & ISC_STATSDUMP_VERBOSE) == 0) {
			continue;
		}

		switch (type) {
		case isc_statsformat_file:
			fp = arg;
			fprintf(fp, "%20" PRIu64 " %s\n", value, desc[idx]);
			break;
		case isc_statsformat_xml:
#ifdef HAVE_LIBXML2
			writer = arg;

			if (category != NULL) {
				/* <NameOfCategory> */
				TRY0(xmlTextWriterStartElement(
					writer, ISC_XMLCHAR category));

				/* <name> inside category */
				TRY0(xmlTextWriterStartElement(
					writer, ISC_XMLCHAR "name"));
				TRY0(xmlTextWriterWriteString(
					writer, ISC_XMLCHAR desc[idx]));
				TRY0(xmlTextWriterEndElement(writer));
				/* </name> */

				/* <counter> */
				TRY0(xmlTextWriterStartElement(
					writer, ISC_XMLCHAR "counter"));
				TRY0(xmlTextWriterWriteFormatString(
					writer, "%" PRIu64, value));

				TRY0(xmlTextWriterEndElement(writer));
				/* </counter> */
				TRY0(xmlTextWriterEndElement(writer));
				/* </NameOfCategory> */
			} else {
				TRY0(xmlTextWriterStartElement(
					writer, ISC_XMLCHAR "counter"));
				TRY0(xmlTextWriterWriteAttribute(
					writer, ISC_XMLCHAR "name",
					ISC_XMLCHAR desc[idx]));
				TRY0(xmlTextWriterWriteFormatString(
					writer, "%" PRIu64, value));
				TRY0(xmlTextWriterEndElement(writer));
				/* counter */
			}

#endif /* ifdef HAVE_LIBXML2 */
			break;
		case isc_statsformat_json:
#ifdef HAVE_JSON_C
			counter = json_object_new_int64(value);
			if (counter == NULL) {
				return ISC_R_NOMEMORY;
			}
			json_object_object_add(cat, desc[idx], counter);
#endif /* ifdef HAVE_JSON_C */
			break;
		}
	}
	return ISC_R_SUCCESS;
#ifdef HAVE_LIBXML2
cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at dump_counters()");
	return ISC_R_FAILURE;
#endif /* ifdef HAVE_LIBXML2 */
}

static void
rdtypestat_dump(dns_rdatastatstype_t type, uint64_t val, void *arg) {
	char typebuf[64];
	const char *typestr;
	stats_dumparg_t *dumparg = arg;
	FILE *fp;
#ifdef HAVE_LIBXML2
	void *writer;
	int xmlrc;
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	json_object *zoneobj, *obj;
#endif /* ifdef HAVE_JSON_C */

	if ((DNS_RDATASTATSTYPE_ATTR(type) &
	     DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) == 0)
	{
		dns_rdatatype_format(DNS_RDATASTATSTYPE_BASE(type), typebuf,
				     sizeof(typebuf));
		typestr = typebuf;
	} else {
		typestr = "Others";
	}

	switch (dumparg->type) {
	case isc_statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" PRIu64 " %s\n", val, typestr);
		break;
	case isc_statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
						 ISC_XMLCHAR typestr));

		TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64, val));

		TRY0(xmlTextWriterEndElement(writer)); /* type */
#endif						       /* ifdef HAVE_LIBXML2 */
		break;
	case isc_statsformat_json:
#ifdef HAVE_JSON_C
		zoneobj = (json_object *)dumparg->arg;
		obj = json_object_new_int64(val);
		if (obj == NULL) {
			return;
		}
		json_object_object_add(zoneobj, typestr, obj);
#endif /* ifdef HAVE_JSON_C */
		break;
	}
	return;
#ifdef HAVE_LIBXML2
cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at rdtypestat_dump()");
	dumparg->result = ISC_R_FAILURE;
	return;
#endif /* ifdef HAVE_LIBXML2 */
}

static bool
rdatastatstype_attr(dns_rdatastatstype_t type, unsigned int attr) {
	return (DNS_RDATASTATSTYPE_ATTR(type) & attr) != 0;
}

static void
rdatasetstats_dump(dns_rdatastatstype_t type, uint64_t val, void *arg) {
	stats_dumparg_t *dumparg = arg;
	FILE *fp;
	char typebuf[64];
	const char *typestr;
	bool nxrrset = false;
	bool stale = false;
	bool ancient = false;
#ifdef HAVE_LIBXML2
	void *writer;
	int xmlrc;
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	json_object *zoneobj, *obj;
	char buf[1024];
#endif /* ifdef HAVE_JSON_C */

	if ((DNS_RDATASTATSTYPE_ATTR(type) &
	     DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) != 0)
	{
		typestr = "NXDOMAIN";
	} else if ((DNS_RDATASTATSTYPE_ATTR(type) &
		    DNS_RDATASTATSTYPE_ATTR_OTHERTYPE) != 0)
	{
		typestr = "Others";
	} else {
		dns_rdatatype_format(DNS_RDATASTATSTYPE_BASE(type), typebuf,
				     sizeof(typebuf));
		typestr = typebuf;
	}

	nxrrset = rdatastatstype_attr(type, DNS_RDATASTATSTYPE_ATTR_NXRRSET);
	stale = rdatastatstype_attr(type, DNS_RDATASTATSTYPE_ATTR_STALE);
	ancient = rdatastatstype_attr(type, DNS_RDATASTATSTYPE_ATTR_ANCIENT);

	switch (dumparg->type) {
	case isc_statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" PRIu64 " %s%s%s%s\n", val, ancient ? "~" : "",
			stale ? "#" : "", nxrrset ? "!" : "", typestr);
		break;
	case isc_statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "rrset"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "name"));
		TRY0(xmlTextWriterWriteFormatString(
			writer, "%s%s%s%s", ancient ? "~" : "",
			stale ? "#" : "", nxrrset ? "!" : "", typestr));
		TRY0(xmlTextWriterEndElement(writer)); /* name */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64, val));
		TRY0(xmlTextWriterEndElement(writer)); /* counter */

		TRY0(xmlTextWriterEndElement(writer)); /* rrset */
#endif						       /* ifdef HAVE_LIBXML2 */
		break;
	case isc_statsformat_json:
#ifdef HAVE_JSON_C
		zoneobj = (json_object *)dumparg->arg;
		snprintf(buf, sizeof(buf), "%s%s%s%s", ancient ? "~" : "",
			 stale ? "#" : "", nxrrset ? "!" : "", typestr);
		obj = json_object_new_int64(val);
		if (obj == NULL) {
			return;
		}
		json_object_object_add(zoneobj, buf, obj);
#endif /* ifdef HAVE_JSON_C */
		break;
	}
	return;
#ifdef HAVE_LIBXML2
cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at rdatasetstats_dump()");
	dumparg->result = ISC_R_FAILURE;
#endif /* ifdef HAVE_LIBXML2 */
}

static void
opcodestat_dump(dns_opcode_t code, uint64_t val, void *arg) {
	FILE *fp;
	isc_buffer_t b;
	char codebuf[64];
	stats_dumparg_t *dumparg = arg;
#ifdef HAVE_LIBXML2
	void *writer;
	int xmlrc;
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	json_object *zoneobj, *obj;
#endif /* ifdef HAVE_JSON_C */

	isc_buffer_init(&b, codebuf, sizeof(codebuf) - 1);
	dns_opcode_totext(code, &b);
	codebuf[isc_buffer_usedlength(&b)] = '\0';

	switch (dumparg->type) {
	case isc_statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" PRIu64 " %s\n", val, codebuf);
		break;
	case isc_statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
						 ISC_XMLCHAR codebuf));
		TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64, val));
		TRY0(xmlTextWriterEndElement(writer)); /* counter */
#endif						       /* ifdef HAVE_LIBXML2 */
		break;
	case isc_statsformat_json:
#ifdef HAVE_JSON_C
		zoneobj = (json_object *)dumparg->arg;
		obj = json_object_new_int64(val);
		if (obj == NULL) {
			return;
		}
		json_object_object_add(zoneobj, codebuf, obj);
#endif /* ifdef HAVE_JSON_C */
		break;
	}
	return;

#ifdef HAVE_LIBXML2
cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at opcodestat_dump()");
	dumparg->result = ISC_R_FAILURE;
	return;
#endif /* ifdef HAVE_LIBXML2 */
}

static void
rcodestat_dump(dns_rcode_t code, uint64_t val, void *arg) {
	FILE *fp;
	isc_buffer_t b;
	char codebuf[64];
	stats_dumparg_t *dumparg = arg;
#ifdef HAVE_LIBXML2
	void *writer;
	int xmlrc;
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	json_object *zoneobj, *obj;
#endif /* ifdef HAVE_JSON_C */

	isc_buffer_init(&b, codebuf, sizeof(codebuf) - 1);
	dns_rcode_totext(code, &b);
	codebuf[isc_buffer_usedlength(&b)] = '\0';

	switch (dumparg->type) {
	case isc_statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" PRIu64 " %s\n", val, codebuf);
		break;
	case isc_statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
						 ISC_XMLCHAR codebuf));
		TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64, val));
		TRY0(xmlTextWriterEndElement(writer)); /* counter */
#endif						       /* ifdef HAVE_LIBXML2 */
		break;
	case isc_statsformat_json:
#ifdef HAVE_JSON_C
		zoneobj = (json_object *)dumparg->arg;
		obj = json_object_new_int64(val);
		if (obj == NULL) {
			return;
		}
		json_object_object_add(zoneobj, codebuf, obj);
#endif /* ifdef HAVE_JSON_C */
		break;
	}
	return;

#ifdef HAVE_LIBXML2
cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at rcodestat_dump()");
	dumparg->result = ISC_R_FAILURE;
	return;
#endif /* ifdef HAVE_LIBXML2 */
}

#if defined(EXTENDED_STATS)
static void
dnssecsignstat_dump(uint32_t kval, uint64_t val, void *arg) {
	FILE *fp;
	char tagbuf[64];
	stats_dumparg_t *dumparg = arg;
#ifdef HAVE_LIBXML2
	xmlTextWriterPtr writer;
	int xmlrc;
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	json_object *zoneobj, *obj;
#endif /* ifdef HAVE_JSON_C */

	/*
	 * kval is '(algorithm << 16) | keyid'.
	 */
	snprintf(tagbuf, sizeof(tagbuf), "%u+%u", (kval >> 16) & 0xff,
		 kval & 0xffff);

	switch (dumparg->type) {
	case isc_statsformat_file:
		fp = dumparg->arg;
		fprintf(fp, "%20" PRIu64 " %s\n", val, tagbuf);
		break;
	case isc_statsformat_xml:
#ifdef HAVE_LIBXML2
		writer = dumparg->arg;
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counter"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
						 ISC_XMLCHAR tagbuf));
		TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64, val));
		TRY0(xmlTextWriterEndElement(writer)); /* counter */
#endif						       /* ifdef HAVE_LIBXML2 */
		break;
	case isc_statsformat_json:
#ifdef HAVE_JSON_C
		zoneobj = (json_object *)dumparg->arg;
		obj = json_object_new_int64(val);
		if (obj == NULL) {
			return;
		}
		json_object_object_add(zoneobj, tagbuf, obj);
#endif /* ifdef HAVE_JSON_C */
		break;
	}
	return;
#ifdef HAVE_LIBXML2
cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at dnssecsignstat_dump()");
	dumparg->result = ISC_R_FAILURE;
	return;
#endif /* ifdef HAVE_LIBXML2 */
}
#endif /* defined(EXTENDED_STATS) */

#ifdef HAVE_LIBXML2
/*
 * Which statistics to include when rendering to XML
 */
#define STATS_XML_STATUS  0x00 /* display only common statistics */
#define STATS_XML_SERVER  0x01
#define STATS_XML_ZONES	  0x02
#define STATS_XML_XFRINS  0x04
#define STATS_XML_NET	  0x08
#define STATS_XML_MEM	  0x10
#define STATS_XML_TRAFFIC 0x20
#define STATS_XML_ALL	  0xff

static isc_result_t
zone_xmlrender(dns_zone_t *zone, void *arg) {
	isc_result_t result;
	char buf[1024 + 32]; /* sufficiently large for zone name and class */
	dns_rdataclass_t rdclass;
	uint32_t serial;
	xmlTextWriterPtr writer = arg;
	dns_zonestat_level_t statlevel;
	int xmlrc;
	stats_dumparg_t dumparg;
	const char *ztype;
	isc_time_t timestamp;

	statlevel = dns_zone_getstatlevel(zone);
	if (statlevel == dns_zonestat_none) {
		return ISC_R_SUCCESS;
	}

	dumparg.type = isc_statsformat_xml;
	dumparg.arg = writer;

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "zone"));

	dns_zone_nameonly(zone, buf, sizeof(buf));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
					 ISC_XMLCHAR buf));

	rdclass = dns_zone_getclass(zone);
	dns_rdataclass_format(rdclass, buf, sizeof(buf));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "rdataclass",
					 ISC_XMLCHAR buf));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "type"));
	ztype = user_zonetype(zone);
	if (ztype != NULL) {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR ztype));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "unknown"));
	}
	TRY0(xmlTextWriterEndElement(writer)); /* type */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "serial"));
	if (dns_zone_getserial(zone, &serial) == ISC_R_SUCCESS) {
		TRY0(xmlTextWriterWriteFormatString(writer, "%u", serial));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer)); /* serial */

	/*
	 * Export zone timers to the statistics channel in XML format.  For
	 * primary zones, only include the loaded time.  For secondary zones,
	 * also include the expire and refresh times.
	 */
	CHECK(dns_zone_getloadtime(zone, &timestamp));

	isc_time_formatISO8601(&timestamp, buf, 64);
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "loaded"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR buf));
	TRY0(xmlTextWriterEndElement(writer));

	if (dns_zone_gettype(zone) == dns_zone_secondary) {
		CHECK(dns_zone_getexpiretime(zone, &timestamp));
		isc_time_formatISO8601(&timestamp, buf, 64);
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "expires"));
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR buf));
		TRY0(xmlTextWriterEndElement(writer));

		CHECK(dns_zone_getrefreshtime(zone, &timestamp));
		isc_time_formatISO8601(&timestamp, buf, 64);
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "refresh"));
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR buf));
		TRY0(xmlTextWriterEndElement(writer));
	}

	if (statlevel == dns_zonestat_full) {
		isc_stats_t *zonestats;
		isc_stats_t *gluecachestats;
		dns_stats_t *rcvquerystats;
		dns_stats_t *dnssecsignstats;
		uint64_t nsstat_values[ns_statscounter_max];
		uint64_t gluecachestats_values[dns_gluecachestatscounter_max];

		zonestats = dns_zone_getrequeststats(zone);
		if (zonestats != NULL) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "counters"));
			TRY0(xmlTextWriterWriteAttribute(writer,
							 ISC_XMLCHAR "type",
							 ISC_XMLCHAR "rcode"));

			CHECK(dump_stats(zonestats, isc_statsformat_xml, writer,
					 NULL, nsstats_xmldesc,
					 ns_statscounter_max, nsstats_index,
					 nsstat_values, ISC_STATSDUMP_VERBOSE));
			/* counters type="rcode"*/
			TRY0(xmlTextWriterEndElement(writer));
		}

		gluecachestats = dns_zone_getgluecachestats(zone);
		if (gluecachestats != NULL) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "counters"));
			TRY0(xmlTextWriterWriteAttribute(
				writer, ISC_XMLCHAR "type",
				ISC_XMLCHAR "gluecache"));

			CHECK(dump_stats(gluecachestats, isc_statsformat_xml,
					 writer, NULL, gluecachestats_xmldesc,
					 dns_gluecachestatscounter_max,
					 gluecachestats_index,
					 gluecachestats_values,
					 ISC_STATSDUMP_VERBOSE));
			/* counters type="rcode"*/
			TRY0(xmlTextWriterEndElement(writer));
		}

		rcvquerystats = dns_zone_getrcvquerystats(zone);
		if (rcvquerystats != NULL) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "counters"));
			TRY0(xmlTextWriterWriteAttribute(writer,
							 ISC_XMLCHAR "type",
							 ISC_XMLCHAR "qtype"));

			dumparg.result = ISC_R_SUCCESS;
			dns_rdatatypestats_dump(rcvquerystats, rdtypestat_dump,
						&dumparg, 0);
			CHECK(dumparg.result);

			/* counters type="qtype"*/
			TRY0(xmlTextWriterEndElement(writer));
		}

		dnssecsignstats = dns_zone_getdnssecsignstats(zone);
		if (dnssecsignstats != NULL) {
			/* counters type="dnssec-sign"*/
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "counters"));
			TRY0(xmlTextWriterWriteAttribute(
				writer, ISC_XMLCHAR "type",
				ISC_XMLCHAR "dnssec-sign"));

			dumparg.result = ISC_R_SUCCESS;
			dns_dnssecsignstats_dump(
				dnssecsignstats, dns_dnssecsignstats_sign,
				dnssecsignstat_dump, &dumparg, 0);
			CHECK(dumparg.result);

			/* counters type="dnssec-sign"*/
			TRY0(xmlTextWriterEndElement(writer));

			/* counters type="dnssec-refresh"*/
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "counters"));
			TRY0(xmlTextWriterWriteAttribute(
				writer, ISC_XMLCHAR "type",
				ISC_XMLCHAR "dnssec-refresh"));

			dumparg.result = ISC_R_SUCCESS;
			dns_dnssecsignstats_dump(
				dnssecsignstats, dns_dnssecsignstats_refresh,
				dnssecsignstat_dump, &dumparg, 0);
			CHECK(dumparg.result);

			/* counters type="dnssec-refresh"*/
			TRY0(xmlTextWriterEndElement(writer));
		}
	}

	TRY0(xmlTextWriterEndElement(writer)); /* zone */

	return ISC_R_SUCCESS;
cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "Failed at zone_xmlrender()");
	return ISC_R_FAILURE;
}

static isc_result_t
xfrin_xmlrender(dns_zone_t *zone, void *arg) {
	char buf[1024 + 32]; /* sufficiently large for zone name and class */
	dns_rdataclass_t rdclass;
	const char *ztype;
	uint32_t serial;
	isc_sockaddr_t addr;
	const isc_sockaddr_t *addrp = NULL;
	char addr_buf[ISC_SOCKADDR_FORMATSIZE];
	dns_transport_type_t transport_type;
	xmlTextWriterPtr writer = arg;
	dns_zonestat_level_t statlevel;
	int xmlrc;
	dns_xfrin_t *xfr = NULL;
	bool is_firstrefresh, is_running, is_deferred, is_presoa, is_pending;
	bool needs_refresh;
	bool is_first_data_received, is_ixfr;
	unsigned int nmsg = 0;
	unsigned int nrecs = 0;
	uint64_t nbytes = 0;

	statlevel = dns_zone_getstatlevel(zone);
	if (statlevel == dns_zonestat_none) {
		return ISC_R_SUCCESS;
	}

	if (dns_zone_getxfr(zone, &xfr, &is_firstrefresh, &is_running,
			    &is_deferred, &is_presoa, &is_pending,
			    &needs_refresh) != ISC_R_SUCCESS)
	{
		/*
		 * Failed to get information about the zone's incoming transfer
		 * (if any), but we still want to continue generating the
		 * remaining parts of the output.
		 */
		return ISC_R_SUCCESS;
	}

	if (!is_running && !is_deferred && !is_presoa && !is_pending &&
	    !needs_refresh)
	{
		if (xfr != NULL) {
			dns_xfrin_detach(&xfr);
		}
		/* No ongoing/queued transfer. */
		return ISC_R_SUCCESS;
	}

	if (is_running && xfr == NULL) {
		/* The transfer is finished, and it's shutting down. */
		return ISC_R_SUCCESS;
	}

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "xfrin"));

	dns_zone_nameonly(zone, buf, sizeof(buf));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
					 ISC_XMLCHAR buf));

	rdclass = dns_zone_getclass(zone);
	dns_rdataclass_format(rdclass, buf, sizeof(buf));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "class",
					 ISC_XMLCHAR buf));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "type"));
	ztype = user_zonetype(zone);
	if (ztype != NULL) {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR ztype));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "serial"));
	if (dns_zone_getserial(zone, &serial) == ISC_R_SUCCESS) {
		TRY0(xmlTextWriterWriteFormatString(writer, "%u", serial));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "remoteserial"));
	if (is_running) {
		serial = dns_xfrin_getendserial(xfr);
		if (serial != 0) {
			TRY0(xmlTextWriterWriteFormatString(writer, "%u",
							    serial));
		} else {
			TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
		}
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "firstrefresh"));
	TRY0(xmlTextWriterWriteString(
		writer, ISC_XMLCHAR(is_firstrefresh ? "Yes" : "No")));
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "state"));
	if (is_running) {
		const char *xfr_state = NULL;

		dns_xfrin_getstate(xfr, &xfr_state, &is_first_data_received,
				   &is_ixfr);
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR xfr_state));
	} else if (is_deferred) {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "Deferred"));
	} else if (is_presoa) {
		TRY0(xmlTextWriterWriteString(writer,
					      ISC_XMLCHAR "Refresh SOA"));
	} else if (is_pending) {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "Pending"));
	} else if (needs_refresh) {
		TRY0(xmlTextWriterWriteString(writer,
					      ISC_XMLCHAR "Needs Refresh"));
	} else {
		UNREACHABLE();
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "refreshqueued"));
	TRY0(xmlTextWriterWriteString(
		writer,
		ISC_XMLCHAR(is_running && needs_refresh ? "Yes" : "No")));
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "localaddr"));
	if (is_running) {
		addrp = dns_xfrin_getsourceaddr(xfr);
		isc_sockaddr_format(addrp, addr_buf, sizeof(addr_buf));
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR addr_buf));
	} else if (is_presoa) {
		addr = dns_zone_getsourceaddr(zone);
		isc_sockaddr_format(&addr, addr_buf, sizeof(addr_buf));
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR addr_buf));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "remoteaddr"));
	if (is_running) {
		addrp = dns_xfrin_getprimaryaddr(xfr);
		isc_sockaddr_format(addrp, addr_buf, sizeof(addr_buf));
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR addr_buf));
	} else if (is_presoa) {
		addr = dns_zone_getprimaryaddr(zone);
		isc_sockaddr_format(&addr, addr_buf, sizeof(addr_buf));
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR addr_buf));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "soatransport"));
	if (is_running || is_presoa) {
		if (is_running) {
			transport_type = dns_xfrin_getsoatransporttype(xfr);
		} else {
			transport_type = dns_zone_getrequesttransporttype(zone);
		}
		if (transport_type == DNS_TRANSPORT_UDP) {
			TRY0(xmlTextWriterWriteString(writer,
						      ISC_XMLCHAR "UDP"));
		} else if (transport_type == DNS_TRANSPORT_TCP) {
			TRY0(xmlTextWriterWriteString(writer,
						      ISC_XMLCHAR "TCP"));
		} else if (transport_type == DNS_TRANSPORT_TLS) {
			TRY0(xmlTextWriterWriteString(writer,
						      ISC_XMLCHAR "TLS"));
		} else if (transport_type == DNS_TRANSPORT_NONE) {
			TRY0(xmlTextWriterWriteString(writer,
						      ISC_XMLCHAR "None"));
		} else {
			/* We don't expect any other SOA transport type. */
			TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
		}
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "transport"));
	if (is_running) {
		transport_type = dns_xfrin_gettransporttype(xfr);
		if (transport_type == DNS_TRANSPORT_TCP) {
			TRY0(xmlTextWriterWriteString(writer,
						      ISC_XMLCHAR "TCP"));
		} else if (transport_type == DNS_TRANSPORT_TLS) {
			TRY0(xmlTextWriterWriteString(writer,
						      ISC_XMLCHAR "TLS"));
		} else {
			/* We don't expect any other transport type. */
			TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
		}
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "-"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tsigkeyname"));
	if (is_running) {
		const dns_name_t *tsigkeyname = dns_xfrin_gettsigkeyname(xfr);
		char tsigkeyname_buf[DNS_NAME_FORMATSIZE];

		if (tsigkeyname != NULL) {
			dns_name_format(tsigkeyname, tsigkeyname_buf,
					sizeof(tsigkeyname_buf));
			TRY0(xmlTextWriterWriteString(
				writer, ISC_XMLCHAR tsigkeyname_buf));
		}
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "duration"));
	if (is_running || is_deferred || is_presoa || is_pending) {
		isc_time_t start = is_running ? dns_xfrin_getstarttime(xfr)
					      : dns_zone_getxfrintime(zone);
		isc_time_t now = isc_time_now();
		isc_time_t diff;
		uint32_t sec;

		isc_time_subtract(&now, &start, &diff);
		sec = isc_time_seconds(&diff);
		TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu32, sec));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR "0"));
	}
	TRY0(xmlTextWriterEndElement(writer));

	if (is_running) {
		dns_xfrin_getstats(xfr, &nmsg, &nrecs, &nbytes);
	}
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "nmsg"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%u", nmsg));
	TRY0(xmlTextWriterEndElement(writer));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "nrecs"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%u", nrecs));
	TRY0(xmlTextWriterEndElement(writer));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "nbytes"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64, nbytes));
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "ixfr"));
	if (is_running && is_first_data_received) {
		TRY0(xmlTextWriterWriteString(
			writer, ISC_XMLCHAR(is_ixfr ? "Yes" : "No")));
	} else {
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR ""));
	}
	TRY0(xmlTextWriterEndElement(writer));

	TRY0(xmlTextWriterEndElement(writer)); /* xfrin */

	if (xfr != NULL) {
		dns_xfrin_detach(&xfr);
	}

	return ISC_R_SUCCESS;

cleanup:
	if (xfr != NULL) {
		dns_xfrin_detach(&xfr);
	}

	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "Failed at xfrin_xmlrender()");

	return ISC_R_FAILURE;
}

static isc_result_t
generatexml(named_server_t *server, uint32_t flags, int *buflen,
	    xmlChar **buf) {
	char boottime[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	char configtime[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	char nowstr[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	isc_time_t now = isc_time_now();
	xmlTextWriterPtr writer = NULL;
	xmlDocPtr doc = NULL;
	int xmlrc;
	dns_view_t *view;
	stats_dumparg_t dumparg;
	dns_stats_t *cacherrstats;
	uint64_t nsstat_values[ns_statscounter_max];
	uint64_t resstat_values[dns_resstatscounter_max];
	uint64_t adbstat_values[dns_adbstats_max];
	uint64_t zonestat_values[dns_zonestatscounter_max];
	uint64_t sockstat_values[isc_sockstatscounter_max];
	uint64_t udpinsizestat_values[DNS_SIZEHISTO_MAXIN + 1];
	uint64_t udpoutsizestat_values[DNS_SIZEHISTO_MAXOUT + 1];
	uint64_t tcpinsizestat_values[DNS_SIZEHISTO_MAXIN + 1];
	uint64_t tcpoutsizestat_values[DNS_SIZEHISTO_MAXOUT + 1];
#ifdef HAVE_DNSTAP
	uint64_t dnstapstat_values[dns_dnstapcounter_max];
#endif /* ifdef HAVE_DNSTAP */
	isc_result_t result;

	isc_time_formatISO8601ms(&named_g_boottime, boottime, sizeof boottime);
	isc_time_formatISO8601ms(&named_g_configtime, configtime,
				 sizeof configtime);
	isc_time_formatISO8601ms(&now, nowstr, sizeof nowstr);

	writer = xmlNewTextWriterDoc(&doc, 0);
	if (writer == NULL) {
		goto cleanup;
	}
	TRY0(xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL));
	TRY0(xmlTextWriterWritePI(writer, ISC_XMLCHAR "xml-stylesheet",
				  ISC_XMLCHAR "type=\"text/xsl\" "
					      "href=\"/bind9.xsl\""));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "statistics"));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "version",
					 ISC_XMLCHAR STATS_XML_VERSION));

	/* Set common fields for statistics dump */
	dumparg.type = isc_statsformat_xml;
	dumparg.arg = writer;

	/* Render server information */
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "server"));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "boot-time"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR boottime));
	TRY0(xmlTextWriterEndElement(writer)); /* boot-time */
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "config-time"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR configtime));
	TRY0(xmlTextWriterEndElement(writer)); /* config-time */
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "current-time"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR nowstr));
	TRY0(xmlTextWriterEndElement(writer)); /* current-time */
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "version"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR PACKAGE_VERSION));
	TRY0(xmlTextWriterEndElement(writer)); /* version */

	if ((flags & STATS_XML_SERVER) != 0) {
		dumparg.result = ISC_R_SUCCESS;

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "opcode"));

		dns_opcodestats_dump(server->sctx->opcodestats, opcodestat_dump,
				     &dumparg, ISC_STATSDUMP_VERBOSE);
		CHECK(dumparg.result);

		TRY0(xmlTextWriterEndElement(writer));

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "rcode"));

		dns_rcodestats_dump(server->sctx->rcodestats, rcodestat_dump,
				    &dumparg, ISC_STATSDUMP_VERBOSE);
		CHECK(dumparg.result);

		TRY0(xmlTextWriterEndElement(writer));

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "qtype"));

		dumparg.result = ISC_R_SUCCESS;
		dns_rdatatypestats_dump(server->sctx->rcvquerystats,
					rdtypestat_dump, &dumparg, 0);
		CHECK(dumparg.result);

		TRY0(xmlTextWriterEndElement(writer)); /* counters */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "nsstat"));

		CHECK(dump_stats(ns_stats_get(server->sctx->nsstats),
				 isc_statsformat_xml, writer, NULL,
				 nsstats_xmldesc, ns_statscounter_max,
				 nsstats_index, nsstat_values,
				 ISC_STATSDUMP_VERBOSE));

		TRY0(xmlTextWriterEndElement(writer)); /* /nsstat */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "zonestat"));

		CHECK(dump_stats(server->zonestats, isc_statsformat_xml, writer,
				 NULL, zonestats_xmldesc,
				 dns_zonestatscounter_max, zonestats_index,
				 zonestat_values, ISC_STATSDUMP_VERBOSE));

		TRY0(xmlTextWriterEndElement(writer)); /* /zonestat */

		/*
		 * Most of the common resolver statistics entries are 0, so
		 * we don't use the verbose dump here.
		 */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "resstat"));
		CHECK(dump_stats(server->resolverstats, isc_statsformat_xml,
				 writer, NULL, resstats_xmldesc,
				 dns_resstatscounter_max, resstats_index,
				 resstat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* resstat */

#ifdef HAVE_DNSTAP
		if (server->dtenv != NULL) {
			isc_stats_t *dnstapstats = NULL;
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "counters"));
			TRY0(xmlTextWriterWriteAttribute(writer,
							 ISC_XMLCHAR "type",
							 ISC_XMLCHAR "dnstap"));
			dns_dt_getstats(named_g_server->dtenv, &dnstapstats);
			result = dump_stats(
				dnstapstats, isc_statsformat_xml, writer, NULL,
				dnstapstats_xmldesc, dns_dnstapcounter_max,
				dnstapstats_index, dnstapstat_values, 0);
			isc_stats_detach(&dnstapstats);
			CHECK(result);

			TRY0(xmlTextWriterEndElement(writer)); /* dnstap */
		}
#endif /* ifdef HAVE_DNSTAP */
	}

	if ((flags & STATS_XML_NET) != 0) {
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "sockstat"));

		CHECK(dump_stats(server->sockstats, isc_statsformat_xml, writer,
				 NULL, sockstats_xmldesc,
				 isc_sockstatscounter_max, sockstats_index,
				 sockstat_values, ISC_STATSDUMP_VERBOSE));

		TRY0(xmlTextWriterEndElement(writer)); /* /sockstat */
	}
	TRY0(xmlTextWriterEndElement(writer)); /* /server */

	if ((flags & STATS_XML_TRAFFIC) != 0) {
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "traffic"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "ipv4"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "udp"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "request-size"));

		CHECK(dump_histo(server->sctx->udpinstats4, isc_statsformat_xml,
				 writer, NULL, udpinsizestats_xmldesc,
				 dns_sizecounter_in_max, udpinsizestats_index,
				 udpinsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		CHECK(dump_histo(
			server->sctx->udpoutstats4, isc_statsformat_xml, writer,
			NULL, udpoutsizestats_xmldesc, dns_sizecounter_out_max,
			udpoutsizestats_index, udpoutsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterEndElement(writer)); /* </udp> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tcp"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "request-size"));

		CHECK(dump_histo(server->sctx->tcpinstats4, isc_statsformat_xml,
				 writer, NULL, tcpinsizestats_xmldesc,
				 dns_sizecounter_in_max, tcpinsizestats_index,
				 tcpinsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		CHECK(dump_histo(
			server->sctx->tcpoutstats4, isc_statsformat_xml, writer,
			NULL, tcpoutsizestats_xmldesc, dns_sizecounter_out_max,
			tcpoutsizestats_index, tcpoutsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterEndElement(writer)); /* </tcp> */
		TRY0(xmlTextWriterEndElement(writer)); /* </ipv4> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "ipv6"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "udp"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "request-size"));

		CHECK(dump_histo(server->sctx->udpinstats6, isc_statsformat_xml,
				 writer, NULL, udpinsizestats_xmldesc,
				 dns_sizecounter_in_max, udpinsizestats_index,
				 udpinsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		CHECK(dump_histo(
			server->sctx->udpoutstats6, isc_statsformat_xml, writer,
			NULL, udpoutsizestats_xmldesc, dns_sizecounter_out_max,
			udpoutsizestats_index, udpoutsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterEndElement(writer)); /* </udp> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tcp"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "request-size"));

		CHECK(dump_histo(server->sctx->tcpinstats6, isc_statsformat_xml,
				 writer, NULL, tcpinsizestats_xmldesc,
				 dns_sizecounter_in_max, tcpinsizestats_index,
				 tcpinsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		CHECK(dump_histo(
			server->sctx->tcpoutstats6, isc_statsformat_xml, writer,
			NULL, tcpoutsizestats_xmldesc, dns_sizecounter_out_max,
			tcpoutsizestats_index, tcpoutsizestat_values, 0));

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterEndElement(writer)); /* </tcp> */
		TRY0(xmlTextWriterEndElement(writer)); /* </ipv6> */
		TRY0(xmlTextWriterEndElement(writer)); /* </traffic> */
	}

	/*
	 * Render views.  For each view we know of, call its
	 * rendering function.
	 */
	view = ISC_LIST_HEAD(server->viewlist);
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "views"));
	while (view != NULL && ((flags & (STATS_XML_SERVER | STATS_XML_ZONES |
					  STATS_XML_XFRINS)) != 0))
	{
		isc_stats_t *istats = NULL;
		dns_stats_t *dstats = NULL;
		dns_adb_t *adb = NULL;

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "view"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
						 ISC_XMLCHAR view->name));

		if ((flags & STATS_XML_ZONES) != 0) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "zones"));
			CHECK(dns_view_apply(view, true, NULL, zone_xmlrender,
					     writer));
			TRY0(xmlTextWriterEndElement(writer)); /* /zones */
		}

		if ((flags & STATS_XML_XFRINS) != 0) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "xfrins"));
			CHECK(dns_zt_apply(view->zonetable, true, NULL,
					   xfrin_xmlrender, writer));
			TRY0(xmlTextWriterEndElement(writer)); /* /xfrins */
		}

		if ((flags & STATS_XML_SERVER) == 0) {
			TRY0(xmlTextWriterEndElement(writer)); /* /view */
			view = ISC_LIST_NEXT(view, link);
			continue;
		}

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "resqtype"));

		dns_resolver_getquerystats(view->resolver, &dstats);
		if (dstats != NULL) {
			dumparg.result = ISC_R_SUCCESS;
			dns_rdatatypestats_dump(dstats, rdtypestat_dump,
						&dumparg, 0);
			CHECK(dumparg.result);
		}
		dns_stats_detach(&dstats);
		TRY0(xmlTextWriterEndElement(writer));

		/* <resstats> */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "resstats"));
		dns_resolver_getstats(view->resolver, &istats);
		if (istats != NULL) {
			CHECK(dump_stats(istats, isc_statsformat_xml, writer,
					 NULL, resstats_xmldesc,
					 dns_resstatscounter_max,
					 resstats_index, resstat_values,
					 ISC_STATSDUMP_VERBOSE));
		}
		isc_stats_detach(&istats);
		TRY0(xmlTextWriterEndElement(writer)); /* </resstats> */

		cacherrstats = dns_db_getrrsetstats(view->cachedb);
		if (cacherrstats != NULL) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "cache"));
			TRY0(xmlTextWriterWriteAttribute(
				writer, ISC_XMLCHAR "name",
				ISC_XMLCHAR dns_cache_getname(view->cache)));
			dumparg.result = ISC_R_SUCCESS;
			dns_rdatasetstats_dump(cacherrstats, rdatasetstats_dump,
					       &dumparg, 0);
			CHECK(dumparg.result);
			TRY0(xmlTextWriterEndElement(writer)); /* cache */
		}

		/* <adbstats> */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "adbstat"));
		dns_view_getadb(view, &adb);
		if (adb != NULL) {
			result = dump_stats(dns_adb_getstats(adb),
					    isc_statsformat_xml, writer, NULL,
					    adbstats_xmldesc, dns_adbstats_max,
					    adbstats_index, adbstat_values,
					    ISC_STATSDUMP_VERBOSE);
			dns_adb_detach(&adb);
			CHECK(result);
		}
		TRY0(xmlTextWriterEndElement(writer)); /* </adbstats> */

		/* <cachestats> */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "cachestats"));
		TRY0(dns_cache_renderxml(view->cache, writer));
		TRY0(xmlTextWriterEndElement(writer)); /* </cachestats> */

		TRY0(xmlTextWriterEndElement(writer)); /* view */

		view = ISC_LIST_NEXT(view, link);
	}
	TRY0(xmlTextWriterEndElement(writer)); /* /views */

	if ((flags & STATS_XML_MEM) != 0) {
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "memory"));
		TRY0(isc_mem_renderxml(writer));
		TRY0(xmlTextWriterEndElement(writer)); /* /memory */
	}

	TRY0(xmlTextWriterEndElement(writer)); /* /statistics */
	TRY0(xmlTextWriterEndDocument(writer));

	xmlDocDumpFormatMemoryEnc(doc, buf, buflen, "UTF-8", 0);
	if (*buf == NULL) {
		goto cleanup;
	}

	xmlFreeTextWriter(writer);
	xmlFreeDoc(doc);
	return ISC_R_SUCCESS;

cleanup:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed generating XML response");
	if (writer != NULL) {
		xmlFreeTextWriter(writer);
	}
	if (doc != NULL) {
		xmlFreeDoc(doc);
	}
	return ISC_R_FAILURE;
}

static void
wrap_xmlfree(isc_buffer_t *buffer, void *arg) {
	UNUSED(arg);

	xmlFree(isc_buffer_base(buffer));
}

static isc_result_t
render_xml(uint32_t flags, void *arg, unsigned int *retcode,
	   const char **retmsg, const char **mimetype, isc_buffer_t *b,
	   isc_httpdfree_t **freecb, void **freecb_args) {
	unsigned char *msg = NULL;
	int msglen;
	named_server_t *server = arg;
	isc_result_t result;

	result = generatexml(server, flags, &msglen, &msg);

	if (result == ISC_R_SUCCESS) {
		*retcode = 200;
		*retmsg = "OK";
		*mimetype = "text/xml";
		isc_buffer_reinit(b, msg, msglen);
		isc_buffer_add(b, msglen);
		*freecb = wrap_xmlfree;
		*freecb_args = NULL;
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "failed at rendering XML()");
	}

	return result;
}

static isc_result_t
render_xml_all(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
	       void *arg, unsigned int *retcode, const char **retmsg,
	       const char **mimetype, isc_buffer_t *b, isc_httpdfree_t **freecb,
	       void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_ALL, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

static isc_result_t
render_xml_status(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		  void *arg, unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_STATUS, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

static isc_result_t
render_xml_server(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		  void *arg, unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_SERVER, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

static isc_result_t
render_xml_zones(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		 void *arg, unsigned int *retcode, const char **retmsg,
		 const char **mimetype, isc_buffer_t *b,
		 isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_ZONES, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

static isc_result_t
render_xml_xfrins(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		  void *arg, unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_XFRINS, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

static isc_result_t
render_xml_net(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
	       void *arg, unsigned int *retcode, const char **retmsg,
	       const char **mimetype, isc_buffer_t *b, isc_httpdfree_t **freecb,
	       void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_NET, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

static isc_result_t
render_xml_mem(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
	       void *arg, unsigned int *retcode, const char **retmsg,
	       const char **mimetype, isc_buffer_t *b, isc_httpdfree_t **freecb,
	       void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_MEM, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

static isc_result_t
render_xml_traffic(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		   void *arg, unsigned int *retcode, const char **retmsg,
		   const char **mimetype, isc_buffer_t *b,
		   isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_xml(STATS_XML_TRAFFIC, arg, retcode, retmsg, mimetype, b,
			  freecb, freecb_args);
}

#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
/*
 * Which statistics to include when rendering to JSON
 */
#define STATS_JSON_STATUS  0x00 /* display only common statistics */
#define STATS_JSON_SERVER  0x01
#define STATS_JSON_ZONES   0x02
#define STATS_JSON_XFRINS  0x04
#define STATS_JSON_NET	   0x08
#define STATS_JSON_MEM	   0x10
#define STATS_JSON_TRAFFIC 0x20
#define STATS_JSON_ALL	   0xff

#define CHECKMEM(m)                              \
	do {                                     \
		if (m == NULL) {                 \
			result = ISC_R_NOMEMORY; \
			goto cleanup;            \
		}                                \
	} while (0)

static void
wrap_jsonfree(isc_buffer_t *buffer, void *arg) {
	json_object_put(isc_buffer_base(buffer));
	if (arg != NULL) {
		json_object_put((json_object *)arg);
	}
}

static json_object *
addzone(char *name, char *classname, const char *ztype, uint32_t serial,
	bool add_serial) {
	json_object *node = json_object_new_object();

	if (node == NULL) {
		return NULL;
	}

	json_object_object_add(node, "name", json_object_new_string(name));
	json_object_object_add(node, "class",
			       json_object_new_string(classname));
	if (add_serial) {
		json_object_object_add(node, "serial",
				       json_object_new_int64(serial));
	}
	if (ztype != NULL) {
		json_object_object_add(node, "type",
				       json_object_new_string(ztype));
	}
	return node;
}

static isc_result_t
zone_jsonrender(dns_zone_t *zone, void *arg) {
	isc_result_t result = ISC_R_SUCCESS;
	char buf[1024 + 32]; /* sufficiently large for zone name and class */
	char classbuf[64];   /* sufficiently large for class */
	char *zone_name_only = NULL;
	char *class_only = NULL;
	dns_rdataclass_t rdclass;
	uint32_t serial;
	json_object *zonearray = (json_object *)arg;
	json_object *zoneobj = NULL;
	dns_zonestat_level_t statlevel;
	isc_time_t timestamp;

	statlevel = dns_zone_getstatlevel(zone);
	if (statlevel == dns_zonestat_none) {
		return ISC_R_SUCCESS;
	}

	dns_zone_nameonly(zone, buf, sizeof(buf));
	zone_name_only = buf;

	rdclass = dns_zone_getclass(zone);
	dns_rdataclass_format(rdclass, classbuf, sizeof(classbuf));
	class_only = classbuf;

	if (dns_zone_getserial(zone, &serial) != ISC_R_SUCCESS) {
		zoneobj = addzone(zone_name_only, class_only,
				  user_zonetype(zone), 0, false);
	} else {
		zoneobj = addzone(zone_name_only, class_only,
				  user_zonetype(zone), serial, true);
	}

	if (zoneobj == NULL) {
		return ISC_R_NOMEMORY;
	}

	/*
	 * Export zone timers to the statistics channel in JSON format.
	 * For primary zones, only include the loaded time.  For secondary
	 * zones, also include the expire and refresh times.
	 */

	CHECK(dns_zone_getloadtime(zone, &timestamp));

	isc_time_formatISO8601(&timestamp, buf, 64);
	json_object_object_add(zoneobj, "loaded", json_object_new_string(buf));

	if (dns_zone_gettype(zone) == dns_zone_secondary) {
		CHECK(dns_zone_getexpiretime(zone, &timestamp));
		isc_time_formatISO8601(&timestamp, buf, 64);
		json_object_object_add(zoneobj, "expires",
				       json_object_new_string(buf));

		CHECK(dns_zone_getrefreshtime(zone, &timestamp));
		isc_time_formatISO8601(&timestamp, buf, 64);
		json_object_object_add(zoneobj, "refresh",
				       json_object_new_string(buf));
	}

	if (statlevel == dns_zonestat_full) {
		isc_stats_t *zonestats;
		isc_stats_t *gluecachestats;
		dns_stats_t *rcvquerystats;
		dns_stats_t *dnssecsignstats;
		uint64_t nsstat_values[ns_statscounter_max];
		uint64_t gluecachestats_values[dns_gluecachestatscounter_max];

		zonestats = dns_zone_getrequeststats(zone);
		if (zonestats != NULL) {
			json_object *counters = json_object_new_object();
			if (counters == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}

			result = dump_stats(zonestats, isc_statsformat_json,
					    counters, NULL, nsstats_xmldesc,
					    ns_statscounter_max, nsstats_index,
					    nsstat_values, 0);
			if (result != ISC_R_SUCCESS) {
				json_object_put(counters);
				goto cleanup;
			}

			if (json_object_get_object(counters)->count != 0) {
				json_object_object_add(zoneobj, "rcodes",
						       counters);
			} else {
				json_object_put(counters);
			}
		}

		gluecachestats = dns_zone_getgluecachestats(zone);
		if (gluecachestats != NULL) {
			json_object *counters = json_object_new_object();
			if (counters == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}

			result = dump_stats(
				gluecachestats, isc_statsformat_json, counters,
				NULL, gluecachestats_xmldesc,
				dns_gluecachestatscounter_max,
				gluecachestats_index, gluecachestats_values, 0);
			if (result != ISC_R_SUCCESS) {
				json_object_put(counters);
				goto cleanup;
			}

			if (json_object_get_object(counters)->count != 0) {
				json_object_object_add(zoneobj, "gluecache",
						       counters);
			} else {
				json_object_put(counters);
			}
		}

		rcvquerystats = dns_zone_getrcvquerystats(zone);
		if (rcvquerystats != NULL) {
			stats_dumparg_t dumparg;
			json_object *counters = json_object_new_object();
			CHECKMEM(counters);

			dumparg.type = isc_statsformat_json;
			dumparg.arg = counters;
			dumparg.result = ISC_R_SUCCESS;
			dns_rdatatypestats_dump(rcvquerystats, rdtypestat_dump,
						&dumparg, 0);
			if (dumparg.result != ISC_R_SUCCESS) {
				json_object_put(counters);
				goto cleanup;
			}

			if (json_object_get_object(counters)->count != 0) {
				json_object_object_add(zoneobj, "qtypes",
						       counters);
			} else {
				json_object_put(counters);
			}
		}

		dnssecsignstats = dns_zone_getdnssecsignstats(zone);
		if (dnssecsignstats != NULL) {
			stats_dumparg_t dumparg;
			json_object *sign_counters = json_object_new_object();
			CHECKMEM(sign_counters);

			dumparg.type = isc_statsformat_json;
			dumparg.arg = sign_counters;
			dumparg.result = ISC_R_SUCCESS;
			dns_dnssecsignstats_dump(
				dnssecsignstats, dns_dnssecsignstats_sign,
				dnssecsignstat_dump, &dumparg, 0);
			if (dumparg.result != ISC_R_SUCCESS) {
				json_object_put(sign_counters);
				goto cleanup;
			}

			if (json_object_get_object(sign_counters)->count != 0) {
				json_object_object_add(zoneobj, "dnssec-sign",
						       sign_counters);
			} else {
				json_object_put(sign_counters);
			}

			json_object *refresh_counters =
				json_object_new_object();
			CHECKMEM(refresh_counters);

			dumparg.type = isc_statsformat_json;
			dumparg.arg = refresh_counters;
			dumparg.result = ISC_R_SUCCESS;
			dns_dnssecsignstats_dump(
				dnssecsignstats, dns_dnssecsignstats_refresh,
				dnssecsignstat_dump, &dumparg, 0);
			if (dumparg.result != ISC_R_SUCCESS) {
				json_object_put(refresh_counters);
				goto cleanup;
			}

			if (json_object_get_object(refresh_counters)->count !=
			    0)
			{
				json_object_object_add(zoneobj,
						       "dnssec-refresh",
						       refresh_counters);
			} else {
				json_object_put(refresh_counters);
			}
		}
	}

	json_object_array_add(zonearray, zoneobj);
	zoneobj = NULL;
	result = ISC_R_SUCCESS;

cleanup:
	if (zoneobj != NULL) {
		json_object_put(zoneobj);
	}
	return result;
}

static isc_result_t
xfrin_jsonrender(dns_zone_t *zone, void *arg) {
	isc_result_t result;
	char buf[1024 + 32]; /* sufficiently large for zone name and class */
	char classbuf[64];   /* sufficiently large for class */
	char *zone_name_only = NULL;
	char *class_only = NULL;
	dns_rdataclass_t rdclass;
	uint32_t serial;
	json_object *xfrinarray = (json_object *)arg;
	json_object *xfrinobj = NULL;
	isc_sockaddr_t addr;
	const isc_sockaddr_t *addrp = NULL;
	char addr_buf[ISC_SOCKADDR_FORMATSIZE];
	dns_transport_type_t transport_type;
	dns_zonestat_level_t statlevel;
	dns_xfrin_t *xfr = NULL;
	bool is_firstrefresh, is_running, is_deferred, is_presoa, is_pending;
	bool needs_refresh;
	bool is_first_data_received, is_ixfr;
	unsigned int nmsg = 0;
	unsigned int nrecs = 0;
	uint64_t nbytes = 0;

	statlevel = dns_zone_getstatlevel(zone);
	if (statlevel == dns_zonestat_none) {
		return ISC_R_SUCCESS;
	}

	dns_zone_nameonly(zone, buf, sizeof(buf));
	zone_name_only = buf;

	rdclass = dns_zone_getclass(zone);
	dns_rdataclass_format(rdclass, classbuf, sizeof(classbuf));
	class_only = classbuf;

	if (dns_zone_getserial(zone, &serial) != ISC_R_SUCCESS) {
		xfrinobj = addzone(zone_name_only, class_only,
				   user_zonetype(zone), 0, false);
	} else {
		xfrinobj = addzone(zone_name_only, class_only,
				   user_zonetype(zone), serial, true);
	}

	if (xfrinobj == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	result = dns_zone_getxfr(zone, &xfr, &is_firstrefresh, &is_running,
				 &is_deferred, &is_presoa, &is_pending,
				 &needs_refresh);
	if (result != ISC_R_SUCCESS) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	if (!is_running && !is_deferred && !is_presoa && !is_pending &&
	    !needs_refresh)
	{
		/* No ongoing/queued transfer. */
		goto cleanup;
	}

	if (is_running && xfr == NULL) {
		/* The transfer is finished, and it's shutting down. */
		goto cleanup;
	}

	if (is_running) {
		serial = dns_xfrin_getendserial(xfr);
		if (serial != 0) {
			json_object_object_add(xfrinobj, "remoteserial",
					       json_object_new_int64(serial));
		}
	}

	json_object_object_add(
		xfrinobj, "firstrefresh",
		json_object_new_string(is_firstrefresh ? "Yes" : "No"));

	if (is_running) {
		const char *xfr_state = NULL;

		dns_xfrin_getstate(xfr, &xfr_state, &is_first_data_received,
				   &is_ixfr);
		json_object_object_add(xfrinobj, "state",
				       json_object_new_string(xfr_state));
	} else if (is_deferred) {
		json_object_object_add(xfrinobj, "state",
				       json_object_new_string("Deferred"));
	} else if (is_presoa) {
		json_object_object_add(xfrinobj, "state",
				       json_object_new_string("Refresh SOA"));
	} else if (is_pending) {
		json_object_object_add(xfrinobj, "state",
				       json_object_new_string("Pending"));
	} else if (needs_refresh) {
		json_object_object_add(xfrinobj, "state",
				       json_object_new_string("Needs Refresh"));
	} else {
		UNREACHABLE();
	}

	json_object_object_add(
		xfrinobj, "refreshqueued",
		json_object_new_string(is_running && needs_refresh ? "Yes"
								   : "No"));

	if (is_running) {
		addrp = dns_xfrin_getsourceaddr(xfr);
		isc_sockaddr_format(addrp, addr_buf, sizeof(addr_buf));
		json_object_object_add(xfrinobj, "localaddr",
				       json_object_new_string(addr_buf));
	} else if (is_presoa) {
		addr = dns_zone_getsourceaddr(zone);
		isc_sockaddr_format(&addr, addr_buf, sizeof(addr_buf));
		json_object_object_add(xfrinobj, "localaddr",
				       json_object_new_string(addr_buf));
	} else {
		json_object_object_add(xfrinobj, "localaddr",
				       json_object_new_string("-"));
	}

	if (is_running) {
		addrp = dns_xfrin_getprimaryaddr(xfr);
		isc_sockaddr_format(addrp, addr_buf, sizeof(addr_buf));
		json_object_object_add(xfrinobj, "remoteaddr",
				       json_object_new_string(addr_buf));
	} else if (is_presoa) {
		addr = dns_zone_getprimaryaddr(zone);
		isc_sockaddr_format(&addr, addr_buf, sizeof(addr_buf));
		json_object_object_add(xfrinobj, "remoteaddr",
				       json_object_new_string(addr_buf));
	} else {
		json_object_object_add(xfrinobj, "remoteaddr",
				       json_object_new_string("-"));
	}

	if (is_running || is_presoa) {
		if (is_running) {
			transport_type = dns_xfrin_getsoatransporttype(xfr);
		} else {
			transport_type = dns_zone_getrequesttransporttype(zone);
		}

		if (transport_type == DNS_TRANSPORT_UDP) {
			json_object_object_add(xfrinobj, "soatransport",
					       json_object_new_string("UDP"));
		} else if (transport_type == DNS_TRANSPORT_TCP) {
			json_object_object_add(xfrinobj, "soatransport",
					       json_object_new_string("TCP"));
		} else if (transport_type == DNS_TRANSPORT_TLS) {
			json_object_object_add(xfrinobj, "soatransport",
					       json_object_new_string("TLS"));
		} else if (transport_type == DNS_TRANSPORT_NONE) {
			json_object_object_add(xfrinobj, "soatransport",
					       json_object_new_string("None"));
		} else {
			/* We don't expect any other SOA transport type. */
			json_object_object_add(xfrinobj, "soatransport",
					       json_object_new_string("-"));
		}
	} else {
		json_object_object_add(xfrinobj, "soatransport",
				       json_object_new_string("-"));
	}

	if (is_running) {
		transport_type = dns_xfrin_gettransporttype(xfr);
		if (transport_type == DNS_TRANSPORT_TCP) {
			json_object_object_add(xfrinobj, "transport",
					       json_object_new_string("TCP"));
		} else if (transport_type == DNS_TRANSPORT_TLS) {
			json_object_object_add(xfrinobj, "transport",
					       json_object_new_string("TLS"));
		} else {
			/* We don't expect any other transport type. */
			json_object_object_add(xfrinobj, "transport",
					       json_object_new_string("-"));
		}
	} else {
		json_object_object_add(xfrinobj, "transport",
				       json_object_new_string("-"));
	}

	if (is_running) {
		const dns_name_t *tsigkeyname = dns_xfrin_gettsigkeyname(xfr);
		char tsigkeyname_buf[DNS_NAME_FORMATSIZE];

		if (tsigkeyname != NULL) {
			dns_name_format(tsigkeyname, tsigkeyname_buf,
					sizeof(tsigkeyname_buf));
			json_object_object_add(
				xfrinobj, "tsigkeyname",
				json_object_new_string(tsigkeyname_buf));
		} else {
			json_object_object_add(xfrinobj, "tsigkeyname", NULL);
		}
	} else {
		json_object_object_add(xfrinobj, "tsigkeyname", NULL);
	}

	if (is_running || is_deferred || is_presoa || is_pending) {
		isc_time_t start = is_running ? dns_xfrin_getstarttime(xfr)
					      : dns_zone_getxfrintime(zone);
		isc_time_t now = isc_time_now();
		isc_time_t diff;
		uint32_t sec;

		isc_time_subtract(&now, &start, &diff);
		sec = isc_time_seconds(&diff);
		json_object_object_add(xfrinobj, "duration",
				       json_object_new_int64((int64_t)sec));
	} else {
		json_object_object_add(xfrinobj, "duration",
				       json_object_new_int64(0));
	}

	if (is_running) {
		dns_xfrin_getstats(xfr, &nmsg, &nrecs, &nbytes);
	}
	json_object_object_add(xfrinobj, "nmsg",
			       json_object_new_int64((int64_t)nmsg));
	json_object_object_add(xfrinobj, "nrecs",
			       json_object_new_int64((int64_t)nrecs));
	json_object_object_add(
		xfrinobj, "nbytes",
		json_object_new_int64(nbytes > INT64_MAX ? INT64_MAX
							 : (int64_t)nbytes));

	if (is_running && is_first_data_received) {
		json_object_object_add(
			xfrinobj, "ixfr",
			json_object_new_string(is_ixfr ? "Yes" : "No"));
	} else {
		json_object_object_add(xfrinobj, "ixfr",
				       json_object_new_string(""));
	}

	json_object_array_add(xfrinarray, xfrinobj);
	xfrinobj = NULL;
	result = ISC_R_SUCCESS;

cleanup:
	if (xfr != NULL) {
		dns_xfrin_detach(&xfr);
	}
	if (xfrinobj != NULL) {
		json_object_put(xfrinobj);
	}
	return result;
}

static isc_result_t
generatejson(named_server_t *server, size_t *msglen, const char **msg,
	     json_object **rootp, uint32_t flags) {
	dns_view_t *view;
	isc_result_t result = ISC_R_SUCCESS;
	json_object *bindstats, *viewlist, *counters, *obj;
	json_object *traffic = NULL;
	json_object *udpreq4 = NULL, *udpresp4 = NULL;
	json_object *tcpreq4 = NULL, *tcpresp4 = NULL;
	json_object *udpreq6 = NULL, *udpresp6 = NULL;
	json_object *tcpreq6 = NULL, *tcpresp6 = NULL;
	uint64_t nsstat_values[ns_statscounter_max];
	uint64_t resstat_values[dns_resstatscounter_max];
	uint64_t adbstat_values[dns_adbstats_max];
	uint64_t zonestat_values[dns_zonestatscounter_max];
	uint64_t sockstat_values[isc_sockstatscounter_max];
	uint64_t udpinsizestat_values[dns_sizecounter_in_max];
	uint64_t udpoutsizestat_values[dns_sizecounter_out_max];
	uint64_t tcpinsizestat_values[dns_sizecounter_in_max];
	uint64_t tcpoutsizestat_values[dns_sizecounter_out_max];
#ifdef HAVE_DNSTAP
	uint64_t dnstapstat_values[dns_dnstapcounter_max];
#endif /* ifdef HAVE_DNSTAP */
	stats_dumparg_t dumparg;
	char boottime[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	char configtime[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	char nowstr[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	isc_time_t now;

	REQUIRE(msglen != NULL);
	REQUIRE(msg != NULL && *msg == NULL);
	REQUIRE(rootp == NULL || *rootp == NULL);

	bindstats = json_object_new_object();
	if (bindstats == NULL) {
		return ISC_R_NOMEMORY;
	}

	/*
	 * These statistics are included no matter which URL we use.
	 */
	obj = json_object_new_string(STATS_JSON_VERSION);
	CHECKMEM(obj);
	json_object_object_add(bindstats, "json-stats-version", obj);

	now = isc_time_now();
	isc_time_formatISO8601ms(&named_g_boottime, boottime, sizeof(boottime));
	isc_time_formatISO8601ms(&named_g_configtime, configtime,
				 sizeof configtime);
	isc_time_formatISO8601ms(&now, nowstr, sizeof(nowstr));

	obj = json_object_new_string(boottime);
	CHECKMEM(obj);
	json_object_object_add(bindstats, "boot-time", obj);

	obj = json_object_new_string(configtime);
	CHECKMEM(obj);
	json_object_object_add(bindstats, "config-time", obj);

	obj = json_object_new_string(nowstr);
	CHECKMEM(obj);
	json_object_object_add(bindstats, "current-time", obj);
	obj = json_object_new_string(PACKAGE_VERSION);
	CHECKMEM(obj);
	json_object_object_add(bindstats, "version", obj);

	if ((flags & STATS_JSON_SERVER) != 0) {
		/* OPCODE counters */
		counters = json_object_new_object();

		dumparg.result = ISC_R_SUCCESS;
		dumparg.type = isc_statsformat_json;
		dumparg.arg = counters;

		dns_opcodestats_dump(server->sctx->opcodestats, opcodestat_dump,
				     &dumparg, ISC_STATSDUMP_VERBOSE);
		if (dumparg.result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto cleanup;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "opcodes", counters);
		} else {
			json_object_put(counters);
		}

		/* OPCODE counters */
		counters = json_object_new_object();

		dumparg.type = isc_statsformat_json;
		dumparg.arg = counters;

		dns_rcodestats_dump(server->sctx->rcodestats, rcodestat_dump,
				    &dumparg, ISC_STATSDUMP_VERBOSE);
		if (dumparg.result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto cleanup;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "rcodes", counters);
		} else {
			json_object_put(counters);
		}

		/* QTYPE counters */
		counters = json_object_new_object();

		dumparg.result = ISC_R_SUCCESS;
		dumparg.arg = counters;

		dns_rdatatypestats_dump(server->sctx->rcvquerystats,
					rdtypestat_dump, &dumparg, 0);
		if (dumparg.result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto cleanup;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "qtypes", counters);
		} else {
			json_object_put(counters);
		}

		/* server stat counters */
		counters = json_object_new_object();

		dumparg.result = ISC_R_SUCCESS;
		dumparg.arg = counters;

		result = dump_stats(ns_stats_get(server->sctx->nsstats),
				    isc_statsformat_json, counters, NULL,
				    nsstats_xmldesc, ns_statscounter_max,
				    nsstats_index, nsstat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto cleanup;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "nsstats", counters);
		} else {
			json_object_put(counters);
		}

		/* zone stat counters */
		counters = json_object_new_object();

		dumparg.result = ISC_R_SUCCESS;
		dumparg.arg = counters;

		result = dump_stats(server->zonestats, isc_statsformat_json,
				    counters, NULL, zonestats_xmldesc,
				    dns_zonestatscounter_max, zonestats_index,
				    zonestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto cleanup;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "zonestats",
					       counters);
		} else {
			json_object_put(counters);
		}

		/* resolver stat counters */
		counters = json_object_new_object();

		dumparg.result = ISC_R_SUCCESS;
		dumparg.arg = counters;

		result = dump_stats(server->resolverstats, isc_statsformat_json,
				    counters, NULL, resstats_xmldesc,
				    dns_resstatscounter_max, resstats_index,
				    resstat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto cleanup;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "resstats", counters);
		} else {
			json_object_put(counters);
		}

#ifdef HAVE_DNSTAP
		/* dnstap stat counters */
		if (named_g_server->dtenv != NULL) {
			isc_stats_t *dnstapstats = NULL;
			dns_dt_getstats(named_g_server->dtenv, &dnstapstats);
			counters = json_object_new_object();
			dumparg.result = ISC_R_SUCCESS;
			dumparg.arg = counters;
			result = dump_stats(dnstapstats, isc_statsformat_json,
					    counters, NULL, dnstapstats_xmldesc,
					    dns_dnstapcounter_max,
					    dnstapstats_index,
					    dnstapstat_values, 0);
			isc_stats_detach(&dnstapstats);
			if (result != ISC_R_SUCCESS) {
				json_object_put(counters);
				goto cleanup;
			}

			if (json_object_get_object(counters)->count != 0) {
				json_object_object_add(bindstats, "dnstapstats",
						       counters);
			} else {
				json_object_put(counters);
			}
		}
#endif /* ifdef HAVE_DNSTAP */
	}

	if ((flags &
	     (STATS_JSON_SERVER | STATS_JSON_ZONES | STATS_JSON_XFRINS)) != 0)
	{
		viewlist = json_object_new_object();
		CHECKMEM(viewlist);

		json_object_object_add(bindstats, "views", viewlist);

		view = ISC_LIST_HEAD(server->viewlist);
		while (view != NULL) {
			json_object *za, *xa, *v = json_object_new_object();
			dns_adb_t *adb = NULL;

			CHECKMEM(v);
			json_object_object_add(viewlist, view->name, v);

			za = json_object_new_array();
			CHECKMEM(za);

			if ((flags & STATS_JSON_ZONES) != 0) {
				CHECK(dns_view_apply(view, true, NULL,
						     zone_jsonrender, za));
			}

			if (json_object_array_length(za) != 0) {
				json_object_object_add(v, "zones", za);
			} else {
				json_object_put(za);
			}

			xa = json_object_new_array();
			CHECKMEM(xa);

			if ((flags & STATS_JSON_XFRINS) != 0) {
				CHECK(dns_zt_apply(view->zonetable, true, NULL,
						   xfrin_jsonrender, xa));
			}

			if (json_object_array_length(xa) != 0) {
				json_object_object_add(v, "xfrins", xa);
			} else {
				json_object_put(xa);
			}

			if ((flags & STATS_JSON_SERVER) != 0) {
				json_object *res = NULL;
				dns_stats_t *dstats = NULL;
				isc_stats_t *istats = NULL;

				res = json_object_new_object();
				CHECKMEM(res);
				json_object_object_add(v, "resolver", res);

				dns_resolver_getstats(view->resolver, &istats);
				if (istats != NULL) {
					counters = json_object_new_object();
					CHECKMEM(counters);

					result = dump_stats(
						istats, isc_statsformat_json,
						counters, NULL,
						resstats_xmldesc,
						dns_resstatscounter_max,
						resstats_index, resstat_values,
						0);
					if (result != ISC_R_SUCCESS) {
						json_object_put(counters);
						result = dumparg.result;
						goto cleanup;
					}

					json_object_object_add(res, "stats",
							       counters);
					isc_stats_detach(&istats);
				}

				dns_resolver_getquerystats(view->resolver,
							   &dstats);
				if (dstats != NULL) {
					counters = json_object_new_object();
					CHECKMEM(counters);

					dumparg.arg = counters;
					dumparg.result = ISC_R_SUCCESS;
					dns_rdatatypestats_dump(dstats,
								rdtypestat_dump,
								&dumparg, 0);
					if (dumparg.result != ISC_R_SUCCESS) {
						json_object_put(counters);
						result = dumparg.result;
						goto cleanup;
					}

					json_object_object_add(res, "qtypes",
							       counters);
					dns_stats_detach(&dstats);
				}

				dstats = dns_db_getrrsetstats(view->cachedb);
				if (dstats != NULL) {
					counters = json_object_new_object();
					CHECKMEM(counters);

					dumparg.arg = counters;
					dumparg.result = ISC_R_SUCCESS;
					dns_rdatasetstats_dump(
						dstats, rdatasetstats_dump,
						&dumparg, 0);
					if (dumparg.result != ISC_R_SUCCESS) {
						json_object_put(counters);
						result = dumparg.result;
						goto cleanup;
					}

					json_object_object_add(res, "cache",
							       counters);
				}

				counters = json_object_new_object();
				CHECKMEM(counters);

				result = dns_cache_renderjson(view->cache,
							      counters);
				if (result != ISC_R_SUCCESS) {
					json_object_put(counters);
					goto cleanup;
				}

				json_object_object_add(res, "cachestats",
						       counters);

				dns_view_getadb(view, &adb);
				if (adb != NULL) {
					istats = dns_adb_getstats(adb);
					dns_adb_detach(&adb);
				}
				if (istats != NULL) {
					counters = json_object_new_object();
					CHECKMEM(counters);

					result = dump_stats(
						istats, isc_statsformat_json,
						counters, NULL,
						adbstats_xmldesc,
						dns_adbstats_max,
						adbstats_index, adbstat_values,
						0);
					if (result != ISC_R_SUCCESS) {
						json_object_put(counters);
						result = dumparg.result;
						goto cleanup;
					}

					json_object_object_add(res, "adb",
							       counters);
				}
			}

			view = ISC_LIST_NEXT(view, link);
		}
	}

	if ((flags & STATS_JSON_NET) != 0) {
		/* socket stat counters */
		counters = json_object_new_object();

		dumparg.result = ISC_R_SUCCESS;
		dumparg.arg = counters;

		result = dump_stats(server->sockstats, isc_statsformat_json,
				    counters, NULL, sockstats_xmldesc,
				    isc_sockstatscounter_max, sockstats_index,
				    sockstat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto cleanup;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "sockstats",
					       counters);
		} else {
			json_object_put(counters);
		}
	}

	if ((flags & STATS_JSON_MEM) != 0) {
		json_object *memory = json_object_new_object();
		CHECKMEM(memory);

		result = isc_mem_renderjson(memory);
		if (result != ISC_R_SUCCESS) {
			json_object_put(memory);
			goto cleanup;
		}

		json_object_object_add(bindstats, "memory", memory);
	}

	if ((flags & STATS_JSON_TRAFFIC) != 0) {
		traffic = json_object_new_object();
		CHECKMEM(traffic);

		udpreq4 = json_object_new_object();
		CHECKMEM(udpreq4);

		udpresp4 = json_object_new_object();
		CHECKMEM(udpresp4);

		tcpreq4 = json_object_new_object();
		CHECKMEM(tcpreq4);

		tcpresp4 = json_object_new_object();
		CHECKMEM(tcpresp4);

		udpreq6 = json_object_new_object();
		CHECKMEM(udpreq6);

		udpresp6 = json_object_new_object();
		CHECKMEM(udpresp6);

		tcpreq6 = json_object_new_object();
		CHECKMEM(tcpreq6);

		tcpresp6 = json_object_new_object();
		CHECKMEM(tcpresp6);

		CHECK(dump_histo(server->sctx->udpinstats4,
				 isc_statsformat_json, udpreq4, NULL,
				 udpinsizestats_xmldesc, dns_sizecounter_in_max,
				 udpinsizestats_index, udpinsizestat_values,
				 0));

		CHECK(dump_histo(server->sctx->udpoutstats4,
				 isc_statsformat_json, udpresp4, NULL,
				 udpoutsizestats_xmldesc,
				 dns_sizecounter_out_max, udpoutsizestats_index,
				 udpoutsizestat_values, 0));

		CHECK(dump_histo(server->sctx->tcpinstats4,
				 isc_statsformat_json, tcpreq4, NULL,
				 tcpinsizestats_xmldesc, dns_sizecounter_in_max,
				 tcpinsizestats_index, tcpinsizestat_values,
				 0));

		CHECK(dump_histo(server->sctx->tcpoutstats4,
				 isc_statsformat_json, tcpresp4, NULL,
				 tcpoutsizestats_xmldesc,
				 dns_sizecounter_out_max, tcpoutsizestats_index,
				 tcpoutsizestat_values, 0));

		CHECK(dump_histo(server->sctx->udpinstats6,
				 isc_statsformat_json, udpreq6, NULL,
				 udpinsizestats_xmldesc, dns_sizecounter_in_max,
				 udpinsizestats_index, udpinsizestat_values,
				 0));

		CHECK(dump_histo(server->sctx->udpoutstats6,
				 isc_statsformat_json, udpresp6, NULL,
				 udpoutsizestats_xmldesc,
				 dns_sizecounter_out_max, udpoutsizestats_index,
				 udpoutsizestat_values, 0));

		CHECK(dump_histo(server->sctx->tcpinstats6,
				 isc_statsformat_json, tcpreq6, NULL,
				 tcpinsizestats_xmldesc, dns_sizecounter_in_max,
				 tcpinsizestats_index, tcpinsizestat_values,
				 0));

		CHECK(dump_histo(server->sctx->tcpoutstats6,
				 isc_statsformat_json, tcpresp6, NULL,
				 tcpoutsizestats_xmldesc,
				 dns_sizecounter_out_max, tcpoutsizestats_index,
				 tcpoutsizestat_values, 0));

		json_object_object_add(traffic,
				       "dns-udp-requests-sizes-received-ipv4",
				       udpreq4);
		json_object_object_add(
			traffic, "dns-udp-responses-sizes-sent-ipv4", udpresp4);
		json_object_object_add(traffic,
				       "dns-tcp-requests-sizes-received-ipv4",
				       tcpreq4);
		json_object_object_add(
			traffic, "dns-tcp-responses-sizes-sent-ipv4", tcpresp4);
		json_object_object_add(traffic,
				       "dns-udp-requests-sizes-received-ipv6",
				       udpreq6);
		json_object_object_add(
			traffic, "dns-udp-responses-sizes-sent-ipv6", udpresp6);
		json_object_object_add(traffic,
				       "dns-tcp-requests-sizes-received-ipv6",
				       tcpreq6);
		json_object_object_add(
			traffic, "dns-tcp-responses-sizes-sent-ipv6", tcpresp6);
		json_object_object_add(bindstats, "traffic", traffic);
		udpreq4 = NULL;
		udpresp4 = NULL;
		tcpreq4 = NULL;
		tcpresp4 = NULL;
		udpreq6 = NULL;
		udpresp6 = NULL;
		tcpreq6 = NULL;
		tcpresp6 = NULL;
		traffic = NULL;
	}

	*msg = json_object_to_json_string_ext(bindstats,
					      JSON_C_TO_STRING_PRETTY);
	*msglen = strlen(*msg);

	if (rootp != NULL) {
		*rootp = bindstats;
		bindstats = NULL;
	}

	result = ISC_R_SUCCESS;

cleanup:
	if (udpreq4 != NULL) {
		json_object_put(udpreq4);
	}
	if (udpresp4 != NULL) {
		json_object_put(udpresp4);
	}
	if (tcpreq4 != NULL) {
		json_object_put(tcpreq4);
	}
	if (tcpresp4 != NULL) {
		json_object_put(tcpresp4);
	}
	if (udpreq6 != NULL) {
		json_object_put(udpreq6);
	}
	if (udpresp6 != NULL) {
		json_object_put(udpresp6);
	}
	if (tcpreq6 != NULL) {
		json_object_put(tcpreq6);
	}
	if (tcpresp6 != NULL) {
		json_object_put(tcpresp6);
	}
	if (traffic != NULL) {
		json_object_put(traffic);
	}
	if (bindstats != NULL) {
		json_object_put(bindstats);
	}

	return result;
}

static isc_result_t
render_json(uint32_t flags, void *arg, unsigned int *retcode,
	    const char **retmsg, const char **mimetype, isc_buffer_t *b,
	    isc_httpdfree_t **freecb, void **freecb_args) {
	isc_result_t result;
	json_object *bindstats = NULL;
	named_server_t *server = arg;
	const char *msg = NULL;
	size_t msglen = 0;
	char *p;

	result = generatejson(server, &msglen, &msg, &bindstats, flags);
	if (result == ISC_R_SUCCESS) {
		*retcode = 200;
		*retmsg = "OK";
		*mimetype = "application/json";
		p = UNCONST(msg);
		isc_buffer_reinit(b, p, msglen);
		isc_buffer_add(b, msglen);
		*freecb = wrap_jsonfree;
		*freecb_args = bindstats;
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "failed at rendering JSON()");
	}

	return result;
}

static isc_result_t
render_json_all(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		void *arg, unsigned int *retcode, const char **retmsg,
		const char **mimetype, isc_buffer_t *b,
		isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_ALL, arg, retcode, retmsg, mimetype, b,
			   freecb, freecb_args);
}

static isc_result_t
render_json_status(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		   void *arg, unsigned int *retcode, const char **retmsg,
		   const char **mimetype, isc_buffer_t *b,
		   isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_STATUS, arg, retcode, retmsg, mimetype, b,
			   freecb, freecb_args);
}

static isc_result_t
render_json_server(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		   void *arg, unsigned int *retcode, const char **retmsg,
		   const char **mimetype, isc_buffer_t *b,
		   isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_SERVER, arg, retcode, retmsg, mimetype, b,
			   freecb, freecb_args);
}

static isc_result_t
render_json_zones(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		  void *arg, unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_ZONES, arg, retcode, retmsg, mimetype, b,
			   freecb, freecb_args);
}

static isc_result_t
render_json_xfrins(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		   void *arg, unsigned int *retcode, const char **retmsg,
		   const char **mimetype, isc_buffer_t *b,
		   isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_XFRINS, arg, retcode, retmsg, mimetype, b,
			   freecb, freecb_args);
}

static isc_result_t
render_json_mem(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		void *arg, unsigned int *retcode, const char **retmsg,
		const char **mimetype, isc_buffer_t *b,
		isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_MEM, arg, retcode, retmsg, mimetype, b,
			   freecb, freecb_args);
}

static isc_result_t
render_json_net(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		void *arg, unsigned int *retcode, const char **retmsg,
		const char **mimetype, isc_buffer_t *b,
		isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_NET, arg, retcode, retmsg, mimetype, b,
			   freecb, freecb_args);
}

static isc_result_t
render_json_traffic(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo,
		    void *arg, unsigned int *retcode, const char **retmsg,
		    const char **mimetype, isc_buffer_t *b,
		    isc_httpdfree_t **freecb, void **freecb_args) {
	UNUSED(httpd);
	UNUSED(urlinfo);
	return render_json(STATS_JSON_TRAFFIC, arg, retcode, retmsg, mimetype,
			   b, freecb, freecb_args);
}

#endif /* HAVE_JSON_C */

#if HAVE_LIBXML2
/*
 * This is only needed if we have libxml2 and was confusingly returned if
 * neither of libxml2 or json-c is configured.
 */
static isc_result_t
render_xsl(const isc_httpd_t *httpd, const isc_httpdurl_t *urlinfo, void *args,
	   unsigned int *retcode, const char **retmsg, const char **mimetype,
	   isc_buffer_t *b, isc_httpdfree_t **freecb, void **freecb_args) {
	isc_result_t result;
	char *p = NULL;

	UNUSED(httpd);
	UNUSED(args);

	*freecb = NULL;
	*freecb_args = NULL;
	*mimetype = "text/xslt+xml";

	if (isc_httpdurl_isstatic(urlinfo)) {
		time_t t1, t2;
		const isc_time_t *when;
		const isc_time_t *loadtime;

		when = isc_httpd_if_modified_since(httpd);

		if (isc_time_isepoch(when)) {
			goto send;
		}

		result = isc_time_secondsastimet(when, &t1);
		if (result != ISC_R_SUCCESS) {
			goto send;
		}

		loadtime = isc_httpdurl_loadtime(urlinfo);

		result = isc_time_secondsastimet(loadtime, &t2);
		if (result != ISC_R_SUCCESS) {
			goto send;
		}

		if (t1 < t2) {
			goto send;
		}

		*retcode = 304;
		*retmsg = "Not modified";
		goto end;
	}

send:
	*retcode = 200;
	*retmsg = "OK";
	p = UNCONST(xslmsg);
	isc_buffer_reinit(b, p, strlen(xslmsg));
	isc_buffer_add(b, strlen(xslmsg));
end:
	return ISC_R_SUCCESS;
}
#endif

static void
shutdown_listener(named_statschannel_t *listener) {
	char socktext[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_format(&listener->address, socktext, sizeof(socktext));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_NOTICE,
		      "stopping statistics channel on %s", socktext);

	isc_httpdmgr_shutdown(&listener->httpdmgr);
}

#if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C)
static bool
client_ok(const isc_sockaddr_t *fromaddr, void *arg) {
	named_statschannel_t *listener = arg;
	dns_aclenv_t *env =
		ns_interfacemgr_getaclenv(named_g_server->interfacemgr);
	isc_netaddr_t netaddr;
	char socktext[ISC_SOCKADDR_FORMATSIZE];
	int match;

	REQUIRE(listener != NULL);

	isc_netaddr_fromsockaddr(&netaddr, fromaddr);

	LOCK(&listener->lock);
	if ((dns_acl_match(&netaddr, NULL, listener->acl, env, &match, NULL) ==
	     ISC_R_SUCCESS) &&
	    match > 0)
	{
		UNLOCK(&listener->lock);
		return true;
	}
	UNLOCK(&listener->lock);

	isc_sockaddr_format(fromaddr, socktext, sizeof(socktext));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
		      "rejected statistics connection from %s", socktext);

	return false;
}
#endif

#if defined(HAVE_LIBXML2) || defined(HAVE_JSON_C)
static void
destroy_listener(void *arg) {
	named_statschannel_t *listener = (named_statschannel_t *)arg;

	REQUIRE(listener != NULL);
	REQUIRE(!ISC_LINK_LINKED(listener, link));

	/* We don't have to acquire the lock here since it's already unlinked */
	dns_acl_detach(&listener->acl);

	isc_mutex_destroy(&listener->lock);
	isc_mem_putanddetach(&listener->mctx, listener, sizeof(*listener));
}
#endif

static isc_result_t
add_listener(named_server_t *server, named_statschannel_t **listenerp,
	     const cfg_obj_t *listen_params, const cfg_obj_t *config,
	     isc_sockaddr_t *addr, cfg_aclconfctx_t *aclconfctx,
	     const char *socktext) {
#if !defined(HAVE_LIBXML2) && !defined(HAVE_JSON_C)
	UNUSED(server);
	UNUSED(listenerp);
	UNUSED(listen_params);
	UNUSED(config);
	UNUSED(addr);
	UNUSED(aclconfctx);
	UNUSED(socktext);

	return ISC_R_NOTIMPLEMENTED;
#else
	isc_result_t result;
	named_statschannel_t *listener = NULL;
	const cfg_obj_t *allow = NULL;
	dns_acl_t *new_acl = NULL;
	int pf;

	listener = isc_mem_get(server->mctx, sizeof(*listener));
	*listener = (named_statschannel_t){ .address = *addr };
	ISC_LINK_INIT(listener, link);
	isc_mutex_init(&listener->lock);
	isc_mem_attach(server->mctx, &listener->mctx);

	allow = cfg_tuple_get(listen_params, "allow");
	if (allow != NULL && cfg_obj_islist(allow)) {
		result = cfg_acl_fromconfig(allow, config, named_g_lctx,
					    aclconfctx, listener->mctx, 0,
					    &new_acl);
	} else {
		result = dns_acl_any(listener->mctx, &new_acl);
	}
	CHECK(result);

	dns_acl_attach(new_acl, &listener->acl);
	dns_acl_detach(&new_acl);

	pf = isc_sockaddr_pf(&listener->address);
	if ((pf == AF_INET && isc_net_probeipv4() != ISC_R_SUCCESS) ||
	    (pf == AF_INET6 && isc_net_probeipv6() != ISC_R_SUCCESS))
	{
		CHECK(ISC_R_FAMILYNOSUPPORT);
	}

	CHECK(isc_httpdmgr_create(named_g_netmgr, server->mctx, addr, client_ok,
				  destroy_listener, listener,
				  &listener->httpdmgr));

#ifdef HAVE_LIBXML2
	isc_httpdmgr_addurl(listener->httpdmgr, "/", false, render_xml_all,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml", false, render_xml_all,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR, false,
			    render_xml_all, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR "/status", false,
			    render_xml_status, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR "/server", false,
			    render_xml_server, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR "/zones", false,
			    render_xml_zones, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR "/xfrins", false,
			    render_xml_xfrins, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR "/net", false,
			    render_xml_net, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR "/mem", false,
			    render_xml_mem, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/xml/v" STATS_XML_VERSION_MAJOR "/traffic", false,
			    render_xml_traffic, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/bind9.xsl", true, render_xsl,
			    server);
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	isc_httpdmgr_addurl(listener->httpdmgr, "/json", false, render_json_all,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR, false,
			    render_json_all, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR "/status", false,
			    render_json_status, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR "/server", false,
			    render_json_server, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR "/zones", false,
			    render_json_zones, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR "/xfrins", false,
			    render_json_xfrins, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR "/net", false,
			    render_json_net, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR "/mem", false,
			    render_json_mem, server);
	isc_httpdmgr_addurl(listener->httpdmgr,
			    "/json/v" STATS_JSON_VERSION_MAJOR "/traffic",
			    false, render_json_traffic, server);
#endif /* ifdef HAVE_JSON_C */

	*listenerp = listener;
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_NOTICE,
		      "statistics channel listening on %s", socktext);

	return ISC_R_SUCCESS;

cleanup:
	if (listener->acl != NULL) {
		dns_acl_detach(&listener->acl);
	}
	isc_mutex_destroy(&listener->lock);
	isc_mem_putanddetach(&listener->mctx, listener, sizeof(*listener));

	return result;
#endif
}

static void
update_listener(named_server_t *server, named_statschannel_t **listenerp,
		const cfg_obj_t *listen_params, const cfg_obj_t *config,
		isc_sockaddr_t *addr, cfg_aclconfctx_t *aclconfctx,
		const char *socktext) {
	named_statschannel_t *listener;
	const cfg_obj_t *allow = NULL;
	dns_acl_t *new_acl = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	for (listener = ISC_LIST_HEAD(server->statschannels); listener != NULL;
	     listener = ISC_LIST_NEXT(listener, link))
	{
		if (isc_sockaddr_equal(addr, &listener->address)) {
			break;
		}
	}

	if (listener == NULL) {
		*listenerp = NULL;
		return;
	}

	/*
	 * Now, keep the old access list unless a new one can be made.
	 */
	allow = cfg_tuple_get(listen_params, "allow");
	if (allow != NULL && cfg_obj_islist(allow)) {
		result = cfg_acl_fromconfig(allow, config, named_g_lctx,
					    aclconfctx, listener->mctx, 0,
					    &new_acl);
	} else {
		result = dns_acl_any(listener->mctx, &new_acl);
	}

	if (result == ISC_R_SUCCESS) {
		LOCK(&listener->lock);

		dns_acl_detach(&listener->acl);
		dns_acl_attach(new_acl, &listener->acl);
		dns_acl_detach(&new_acl);

		UNLOCK(&listener->lock);
	} else {
		cfg_obj_log(listen_params, named_g_lctx, ISC_LOG_WARNING,
			    "couldn't install new acl for "
			    "statistics channel %s: %s",
			    socktext, isc_result_totext(result));
	}

	*listenerp = listener;
}

isc_result_t
named_statschannels_configure(named_server_t *server, const cfg_obj_t *config,
			      cfg_aclconfctx_t *aclconfctx) {
	named_statschannel_t *listener, *listener_next;
	named_statschannellist_t new_listeners;
	const cfg_obj_t *statschannellist = NULL;
	const cfg_listelt_t *element, *element2;
	char socktext[ISC_SOCKADDR_FORMATSIZE];

	isc_once_do(&once, init_desc);

	ISC_LIST_INIT(new_listeners);

	/*
	 * Get the list of named.conf 'statistics-channels' statements.
	 */
	(void)cfg_map_get(config, "statistics-channels", &statschannellist);

	/*
	 * Run through the new address/port list, noting sockets that are
	 * already being listened on and moving them to the new list.
	 *
	 * Identifying duplicate addr/port combinations is left to either
	 * the underlying config code, or to the bind attempt getting an
	 * address-in-use error.
	 */
	if (statschannellist != NULL) {
#ifndef EXTENDED_STATS
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "statistics-channels specified but not effective "
			      "due to missing XML and/or JSON library");
#else /* EXTENDED_STATS */
#ifndef HAVE_LIBXML2
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "statistics-channels: XML library missing, "
			      "only JSON stats will be available");
#endif /* !HAVE_LIBXML2 */
#ifndef HAVE_JSON_C
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
			      "statistics-channels: JSON library missing, "
			      "only XML stats will be available");
#endif /* !HAVE_JSON_C */
#endif /* EXTENDED_STATS */

		for (element = cfg_list_first(statschannellist);
		     element != NULL; element = cfg_list_next(element))
		{
			const cfg_obj_t *statschannel;
			const cfg_obj_t *listenercfg = NULL;

			statschannel = cfg_listelt_value(element);
			(void)cfg_map_get(statschannel, "inet", &listenercfg);
			if (listenercfg == NULL) {
				continue;
			}

			for (element2 = cfg_list_first(listenercfg);
			     element2 != NULL;
			     element2 = cfg_list_next(element2))
			{
				const cfg_obj_t *listen_params;
				const cfg_obj_t *obj;
				isc_sockaddr_t addr;

				listen_params = cfg_listelt_value(element2);

				obj = cfg_tuple_get(listen_params, "address");
				addr = *cfg_obj_assockaddr(obj);
				if (isc_sockaddr_getport(&addr) == 0) {
					isc_sockaddr_setport(
						&addr,
						NAMED_STATSCHANNEL_HTTPPORT);
				}

				isc_sockaddr_format(&addr, socktext,
						    sizeof(socktext));

				isc_log_write(named_g_lctx,
					      NAMED_LOGCATEGORY_GENERAL,
					      NAMED_LOGMODULE_SERVER,
					      ISC_LOG_DEBUG(9),
					      "processing statistics "
					      "channel %s",
					      socktext);

				update_listener(server, &listener,
						listen_params, config, &addr,
						aclconfctx, socktext);

				if (listener != NULL) {
					/*
					 * Remove the listener from the old
					 * list, so it won't be shut down.
					 */
					ISC_LIST_UNLINK(server->statschannels,
							listener, link);
				} else {
					/*
					 * This is a new listener.
					 */
					isc_result_t r;

					r = add_listener(server, &listener,
							 listen_params, config,
							 &addr, aclconfctx,
							 socktext);
					if (r != ISC_R_SUCCESS) {
						cfg_obj_log(
							listen_params,
							named_g_lctx,
							ISC_LOG_WARNING,
							"couldn't allocate "
							"statistics channel"
							" %s: %s",
							socktext,
							isc_result_totext(r));
					}
				}

				if (listener != NULL) {
					ISC_LIST_APPEND(new_listeners, listener,
							link);
				}
			}
		}
	}

	for (listener = ISC_LIST_HEAD(server->statschannels); listener != NULL;
	     listener = listener_next)
	{
		listener_next = ISC_LIST_NEXT(listener, link);
		ISC_LIST_UNLINK(server->statschannels, listener, link);
		shutdown_listener(listener);
	}

	ISC_LIST_APPENDLIST(server->statschannels, new_listeners, link);
	return ISC_R_SUCCESS;
}

void
named_statschannels_shutdown(named_server_t *server) {
	named_statschannel_t *listener;

	while ((listener = ISC_LIST_HEAD(server->statschannels)) != NULL) {
		ISC_LIST_UNLINK(server->statschannels, listener, link);
		shutdown_listener(listener);
	}
}

isc_result_t
named_stats_dump(named_server_t *server, FILE *fp) {
	isc_result_t result;
	dns_view_t *view;
	dns_zone_t *zone, *next;
	stats_dumparg_t dumparg;
	uint64_t nsstat_values[ns_statscounter_max];
	uint64_t resstat_values[dns_resstatscounter_max];
	uint64_t adbstat_values[dns_adbstats_max];
	uint64_t zonestat_values[dns_zonestatscounter_max];
	uint64_t sockstat_values[isc_sockstatscounter_max];
	uint64_t gluecachestats_values[dns_gluecachestatscounter_max];
	isc_stdtime_t now = isc_stdtime_now();

	isc_once_do(&once, init_desc);

	/* Set common fields */
	dumparg.type = isc_statsformat_file;
	dumparg.arg = fp;

	fprintf(fp, "+++ Statistics Dump +++ (%lu)\n", (unsigned long)now);

	fprintf(fp, "++ Incoming Requests ++\n");
	dns_opcodestats_dump(server->sctx->opcodestats, opcodestat_dump,
			     &dumparg, 0);

	fprintf(fp, "++ Incoming Queries ++\n");
	dns_rdatatypestats_dump(server->sctx->rcvquerystats, rdtypestat_dump,
				&dumparg, 0);

	fprintf(fp, "++ Outgoing Rcodes ++\n");
	dns_rcodestats_dump(server->sctx->rcodestats, rcodestat_dump, &dumparg,
			    0);

	fprintf(fp, "++ Outgoing Queries ++\n");
	for (view = ISC_LIST_HEAD(server->viewlist); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		dns_stats_t *dstats = NULL;
		dns_resolver_getquerystats(view->resolver, &dstats);
		if (dstats == NULL) {
			continue;
		}
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s]\n", view->name);
		}
		dns_rdatatypestats_dump(dstats, rdtypestat_dump, &dumparg, 0);
		dns_stats_detach(&dstats);
	}

	fprintf(fp, "++ Name Server Statistics ++\n");
	(void)dump_stats(ns_stats_get(server->sctx->nsstats),
			 isc_statsformat_file, fp, NULL, nsstats_desc,
			 ns_statscounter_max, nsstats_index, nsstat_values, 0);

	fprintf(fp, "++ Zone Maintenance Statistics ++\n");
	(void)dump_stats(server->zonestats, isc_statsformat_file, fp, NULL,
			 zonestats_desc, dns_zonestatscounter_max,
			 zonestats_index, zonestat_values, 0);

	fprintf(fp, "++ Resolver Statistics ++\n");
	fprintf(fp, "[Common]\n");
	(void)dump_stats(server->resolverstats, isc_statsformat_file, fp, NULL,
			 resstats_desc, dns_resstatscounter_max, resstats_index,
			 resstat_values, 0);
	for (view = ISC_LIST_HEAD(server->viewlist); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		isc_stats_t *istats = NULL;
		dns_resolver_getstats(view->resolver, &istats);
		if (istats == NULL) {
			continue;
		}
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s]\n", view->name);
		}
		(void)dump_stats(istats, isc_statsformat_file, fp, NULL,
				 resstats_desc, dns_resstatscounter_max,
				 resstats_index, resstat_values, 0);
		isc_stats_detach(&istats);
	}

	fprintf(fp, "++ Cache Statistics ++\n");
	for (view = ISC_LIST_HEAD(server->viewlist); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s (Cache: %s)]\n", view->name,
				dns_cache_getname(view->cache));
		}
		/*
		 * Avoid dumping redundant statistics when the cache is shared.
		 */
		if (dns_view_iscacheshared(view)) {
			continue;
		}
		dns_cache_dumpstats(view->cache, fp);
	}

	fprintf(fp, "++ Cache DB RRsets ++\n");
	for (view = ISC_LIST_HEAD(server->viewlist); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		dns_stats_t *cacherrstats;

		cacherrstats = dns_db_getrrsetstats(view->cachedb);
		if (cacherrstats == NULL) {
			continue;
		}
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s (Cache: %s)]\n", view->name,
				dns_cache_getname(view->cache));
		}
		if (dns_view_iscacheshared(view)) {
			/*
			 * Avoid dumping redundant statistics when the cache is
			 * shared.
			 */
			continue;
		}
		dns_rdatasetstats_dump(cacherrstats, rdatasetstats_dump,
				       &dumparg, 0);
	}

	fprintf(fp, "++ ADB stats ++\n");
	for (view = ISC_LIST_HEAD(server->viewlist); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		dns_adb_t *adb = NULL;
		isc_stats_t *adbstats = NULL;

		dns_view_getadb(view, &adb);
		if (adb != NULL) {
			adbstats = dns_adb_getstats(adb);
			dns_adb_detach(&adb);
		}
		if (adbstats == NULL) {
			continue;
		}
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s]\n", view->name);
		}
		(void)dump_stats(adbstats, isc_statsformat_file, fp, NULL,
				 adbstats_desc, dns_adbstats_max,
				 adbstats_index, adbstat_values, 0);
	}

	fprintf(fp, "++ Socket I/O Statistics ++\n");
	(void)dump_stats(server->sockstats, isc_statsformat_file, fp, NULL,
			 sockstats_desc, isc_sockstatscounter_max,
			 sockstats_index, sockstat_values, 0);

	fprintf(fp, "++ Per Zone Query Statistics ++\n");
	zone = NULL;
	for (result = dns_zone_first(server->zonemgr, &zone);
	     result == ISC_R_SUCCESS;
	     next = NULL, result = dns_zone_next(zone, &next), zone = next)
	{
		isc_stats_t *zonestats = dns_zone_getrequeststats(zone);
		if (zonestats != NULL) {
			char zonename[DNS_NAME_FORMATSIZE];

			view = dns_zone_getview(zone);
			if (view == NULL) {
				continue;
			}

			dns_name_format(dns_zone_getorigin(zone), zonename,
					sizeof(zonename));
			fprintf(fp, "[%s", zonename);
			if (strcmp(view->name, "_default") != 0) {
				fprintf(fp, " (view: %s)", view->name);
			}
			fprintf(fp, "]\n");

			(void)dump_stats(zonestats, isc_statsformat_file, fp,
					 NULL, nsstats_desc,
					 ns_statscounter_max, nsstats_index,
					 nsstat_values, 0);
		}
	}

	fprintf(fp, "++ Per Zone Glue Cache Statistics ++\n");
	zone = NULL;
	for (result = dns_zone_first(server->zonemgr, &zone);
	     result == ISC_R_SUCCESS;
	     next = NULL, result = dns_zone_next(zone, &next), zone = next)
	{
		isc_stats_t *gluecachestats = dns_zone_getgluecachestats(zone);
		if (gluecachestats != NULL) {
			char zonename[DNS_NAME_FORMATSIZE];

			view = dns_zone_getview(zone);
			if (view == NULL) {
				continue;
			}

			dns_name_format(dns_zone_getorigin(zone), zonename,
					sizeof(zonename));
			fprintf(fp, "[%s", zonename);
			if (strcmp(view->name, "_default") != 0) {
				fprintf(fp, " (view: %s)", view->name);
			}
			fprintf(fp, "]\n");

			(void)dump_stats(gluecachestats, isc_statsformat_file,
					 fp, NULL, gluecachestats_desc,
					 dns_gluecachestatscounter_max,
					 gluecachestats_index,
					 gluecachestats_values, 0);
		}
	}

	fprintf(fp, "--- Statistics Dump --- (%lu)\n", (unsigned long)now);

	return ISC_R_SUCCESS; /* this function currently always succeeds */
}
