/*	$NetBSD: statschannel.c,v 1.13 2023/01/25 21:43:23 christos Exp $	*/

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
#include <isc/print.h>
#include <isc/socket.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/cache.h>
#include <dns/db.h>
#include <dns/opcode.h>
#include <dns/rcode.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/stats.h>
#include <dns/view.h>
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

#include "bind9.xsl.h"

#define CHECK(m)                             \
	do {                                 \
		result = (m);                \
		if (result != ISC_R_SUCCESS) \
			goto error;          \
	} while (0)

struct named_statschannel {
	/* Unlocked */
	isc_httpdmgr_t *httpdmgr;
	isc_sockaddr_t address;
	isc_mem_t *mctx;

	/*
	 * Locked by channel lock: can be referenced and modified by both
	 * the server task and the channel task.
	 */
	isc_mutex_t lock;
	dns_acl_t *acl;

	/* Locked by server task */
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
			{ dns_zone_primary, "master" },
			{ dns_zone_secondary, "slave" },
			{ dns_zone_mirror, "mirror" },
			{ dns_zone_stub, "stub" },
			{ dns_zone_staticstub, "static-stub" },
			{ dns_zone_key, "key" },
			{ dns_zone_dlz, "dlz" },
			{ dns_zone_redirect, "redirect" },
			{ 0, NULL } };
	const struct zt *tp;

	if ((dns_zone_getoptions(zone) & DNS_ZONEOPT_AUTOEMPTY) != 0) {
		return ("builtin");
	}

	view = dns_zone_getview(zone);
	if (view != NULL && strcmp(view->name, "_bind") == 0) {
		return ("builtin");
	}

	ztype = dns_zone_gettype(zone);
	for (tp = typemap; tp->string != NULL && tp->type != ztype; tp++) {
		/* empty */
	}
	return (tp->string);
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

#define TRY0(a)                     \
	do {                        \
		xmlrc = (a);        \
		if (xmlrc < 0)      \
			goto error; \
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
	SET_NSSTATDESC(nodatasynth, "syththesized a no-data response",
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
	SET_SOCKSTATDESC(unixopen, "Unix domain sockets opened", "UnixOpen");
	SET_SOCKSTATDESC(rawopen, "Raw sockets opened", "RawOpen");
	SET_SOCKSTATDESC(udp4openfail, "UDP/IPv4 socket open failures",
			 "UDP4OpenFail");
	SET_SOCKSTATDESC(udp6openfail, "UDP/IPv6 socket open failures",
			 "UDP6OpenFail");
	SET_SOCKSTATDESC(tcp4openfail, "TCP/IPv4 socket open failures",
			 "TCP4OpenFail");
	SET_SOCKSTATDESC(tcp6openfail, "TCP/IPv6 socket open failures",
			 "TCP6OpenFail");
	SET_SOCKSTATDESC(unixopenfail, "Unix domain socket open failures",
			 "UnixOpenFail");
	SET_SOCKSTATDESC(rawopenfail, "Raw socket open failures",
			 "RawOpenFail");
	SET_SOCKSTATDESC(udp4close, "UDP/IPv4 sockets closed", "UDP4Close");
	SET_SOCKSTATDESC(udp6close, "UDP/IPv6 sockets closed", "UDP6Close");
	SET_SOCKSTATDESC(tcp4close, "TCP/IPv4 sockets closed", "TCP4Close");
	SET_SOCKSTATDESC(tcp6close, "TCP/IPv6 sockets closed", "TCP6Close");
	SET_SOCKSTATDESC(unixclose, "Unix domain sockets closed", "UnixClose");
	SET_SOCKSTATDESC(fdwatchclose, "FDwatch sockets closed",
			 "FDWatchClose");
	SET_SOCKSTATDESC(rawclose, "Raw sockets closed", "RawClose");
	SET_SOCKSTATDESC(udp4bindfail, "UDP/IPv4 socket bind failures",
			 "UDP4BindFail");
	SET_SOCKSTATDESC(udp6bindfail, "UDP/IPv6 socket bind failures",
			 "UDP6BindFail");
	SET_SOCKSTATDESC(tcp4bindfail, "TCP/IPv4 socket bind failures",
			 "TCP4BindFail");
	SET_SOCKSTATDESC(tcp6bindfail, "TCP/IPv6 socket bind failures",
			 "TCP6BindFail");
	SET_SOCKSTATDESC(unixbindfail, "Unix domain socket bind failures",
			 "UnixBindFail");
	SET_SOCKSTATDESC(fdwatchbindfail, "FDwatch socket bind failures",
			 "FdwatchBindFail");
	SET_SOCKSTATDESC(udp4connectfail, "UDP/IPv4 socket connect failures",
			 "UDP4ConnFail");
	SET_SOCKSTATDESC(udp6connectfail, "UDP/IPv6 socket connect failures",
			 "UDP6ConnFail");
	SET_SOCKSTATDESC(tcp4connectfail, "TCP/IPv4 socket connect failures",
			 "TCP4ConnFail");
	SET_SOCKSTATDESC(tcp6connectfail, "TCP/IPv6 socket connect failures",
			 "TCP6ConnFail");
	SET_SOCKSTATDESC(unixconnectfail, "Unix domain socket connect failures",
			 "UnixConnFail");
	SET_SOCKSTATDESC(fdwatchconnectfail, "FDwatch socket connect failures",
			 "FDwatchConnFail");
	SET_SOCKSTATDESC(udp4connect, "UDP/IPv4 connections established",
			 "UDP4Conn");
	SET_SOCKSTATDESC(udp6connect, "UDP/IPv6 connections established",
			 "UDP6Conn");
	SET_SOCKSTATDESC(tcp4connect, "TCP/IPv4 connections established",
			 "TCP4Conn");
	SET_SOCKSTATDESC(tcp6connect, "TCP/IPv6 connections established",
			 "TCP6Conn");
	SET_SOCKSTATDESC(unixconnect, "Unix domain connections established",
			 "UnixConn");
	SET_SOCKSTATDESC(fdwatchconnect,
			 "FDwatch domain connections established",
			 "FDwatchConn");
	SET_SOCKSTATDESC(tcp4acceptfail, "TCP/IPv4 connection accept failures",
			 "TCP4AcceptFail");
	SET_SOCKSTATDESC(tcp6acceptfail, "TCP/IPv6 connection accept failures",
			 "TCP6AcceptFail");
	SET_SOCKSTATDESC(unixacceptfail,
			 "Unix domain connection accept failures",
			 "UnixAcceptFail");
	SET_SOCKSTATDESC(tcp4accept, "TCP/IPv4 connections accepted",
			 "TCP4Accept");
	SET_SOCKSTATDESC(tcp6accept, "TCP/IPv6 connections accepted",
			 "TCP6Accept");
	SET_SOCKSTATDESC(unixaccept, "Unix domain connections accepted",
			 "UnixAccept");
	SET_SOCKSTATDESC(udp4sendfail, "UDP/IPv4 send errors", "UDP4SendErr");
	SET_SOCKSTATDESC(udp6sendfail, "UDP/IPv6 send errors", "UDP6SendErr");
	SET_SOCKSTATDESC(tcp4sendfail, "TCP/IPv4 send errors", "TCP4SendErr");
	SET_SOCKSTATDESC(tcp6sendfail, "TCP/IPv6 send errors", "TCP6SendErr");
	SET_SOCKSTATDESC(unixsendfail, "Unix domain send errors",
			 "UnixSendErr");
	SET_SOCKSTATDESC(fdwatchsendfail, "FDwatch send errors",
			 "FDwatchSendErr");
	SET_SOCKSTATDESC(udp4recvfail, "UDP/IPv4 recv errors", "UDP4RecvErr");
	SET_SOCKSTATDESC(udp6recvfail, "UDP/IPv6 recv errors", "UDP6RecvErr");
	SET_SOCKSTATDESC(tcp4recvfail, "TCP/IPv4 recv errors", "TCP4RecvErr");
	SET_SOCKSTATDESC(tcp6recvfail, "TCP/IPv6 recv errors", "TCP6RecvErr");
	SET_SOCKSTATDESC(unixrecvfail, "Unix domain recv errors",
			 "UnixRecvErr");
	SET_SOCKSTATDESC(fdwatchrecvfail, "FDwatch recv errors",
			 "FDwatchRecvErr");
	SET_SOCKSTATDESC(rawrecvfail, "Raw recv errors", "RawRecvErr");
	SET_SOCKSTATDESC(udp4active, "UDP/IPv4 sockets active", "UDP4Active");
	SET_SOCKSTATDESC(udp6active, "UDP/IPv6 sockets active", "UDP6Active");
	SET_SOCKSTATDESC(tcp4active, "TCP/IPv4 sockets active", "TCP4Active");
	SET_SOCKSTATDESC(tcp6active, "TCP/IPv6 sockets active", "TCP6Active");
	SET_SOCKSTATDESC(unixactive, "Unix domain sockets active",
			 "UnixActive");
	SET_SOCKSTATDESC(rawactive, "Raw sockets active", "RawActive");
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
	for (i = 0; i < dns_sizecounter_in_max; i++) {
		udpinsizestats_desc[i] = NULL;
		tcpinsizestats_desc[i] = NULL;
#if defined(EXTENDED_STATS)
		udpinsizestats_xmldesc[i] = NULL;
		tcpinsizestats_xmldesc[i] = NULL;
#endif /* if defined(EXTENDED_STATS) */
	}
	for (i = 0; i < dns_sizecounter_out_max; i++) {
		udpoutsizestats_desc[i] = NULL;
		tcpoutsizestats_desc[i] = NULL;
#if defined(EXTENDED_STATS)
		udpoutsizestats_xmldesc[i] = NULL;
		tcpoutsizestats_xmldesc[i] = NULL;
#endif /* if defined(EXTENDED_STATS) */
	}

#define SET_SIZESTATDESC(counterid, desc, xmldesc, inout)       \
	do {                                                    \
		set_desc(dns_sizecounter_##inout##_##counterid, \
			 dns_sizecounter_##inout##_max, desc,   \
			 udp##inout##sizestats_desc, xmldesc,   \
			 udp##inout##sizestats_xmldesc);        \
		set_desc(dns_sizecounter_##inout##_##counterid, \
			 dns_sizecounter_##inout##_max, desc,   \
			 tcp##inout##sizestats_desc, xmldesc,   \
			 tcp##inout##sizestats_xmldesc);        \
		udp##inout##sizestats_index[i] =                \
			dns_sizecounter_##inout##_##counterid;  \
		tcp##inout##sizestats_index[i] =                \
			dns_sizecounter_##inout##_##counterid;  \
		i++;                                            \
	} while (0)

	i = 0;
	SET_SIZESTATDESC(0, "requests received 0-15 bytes", "0-15", in);
	SET_SIZESTATDESC(16, "requests received 16-31 bytes", "16-31", in);
	SET_SIZESTATDESC(32, "requests received 32-47 bytes", "32-47", in);
	SET_SIZESTATDESC(48, "requests received 48-63 bytes", "48-63", in);
	SET_SIZESTATDESC(64, "requests received 64-79 bytes", "64-79", in);
	SET_SIZESTATDESC(80, "requests received 80-95 bytes", "80-95", in);
	SET_SIZESTATDESC(96, "requests received 96-111 bytes", "96-111", in);
	SET_SIZESTATDESC(112, "requests received 112-127 bytes", "112-127", in);
	SET_SIZESTATDESC(128, "requests received 128-143 bytes", "128-143", in);
	SET_SIZESTATDESC(144, "requests received 144-159 bytes", "144-159", in);
	SET_SIZESTATDESC(160, "requests received 160-175 bytes", "160-175", in);
	SET_SIZESTATDESC(176, "requests received 176-191 bytes", "176-191", in);
	SET_SIZESTATDESC(192, "requests received 192-207 bytes", "192-207", in);
	SET_SIZESTATDESC(208, "requests received 208-223 bytes", "208-223", in);
	SET_SIZESTATDESC(224, "requests received 224-239 bytes", "224-239", in);
	SET_SIZESTATDESC(240, "requests received 240-255 bytes", "240-255", in);
	SET_SIZESTATDESC(256, "requests received 256-271 bytes", "256-271", in);
	SET_SIZESTATDESC(272, "requests received 272-287 bytes", "272-287", in);
	SET_SIZESTATDESC(288, "requests received 288+ bytes", "288+", in);
	INSIST(i == dns_sizecounter_in_max);

	i = 0;
	SET_SIZESTATDESC(0, "responses sent 0-15 bytes", "0-15", out);
	SET_SIZESTATDESC(16, "responses sent 16-31 bytes", "16-31", out);
	SET_SIZESTATDESC(32, "responses sent 32-47 bytes", "32-47", out);
	SET_SIZESTATDESC(48, "responses sent 48-63 bytes", "48-63", out);
	SET_SIZESTATDESC(64, "responses sent 64-79 bytes", "64-79", out);
	SET_SIZESTATDESC(80, "responses sent 80-95 bytes", "80-95", out);
	SET_SIZESTATDESC(96, "responses sent 96-111 bytes", "96-111", out);
	SET_SIZESTATDESC(112, "responses sent 112-127 bytes", "112-127", out);
	SET_SIZESTATDESC(128, "responses sent 128-143 bytes", "128-143", out);
	SET_SIZESTATDESC(144, "responses sent 144-159 bytes", "144-159", out);
	SET_SIZESTATDESC(160, "responses sent 160-175 bytes", "160-175", out);
	SET_SIZESTATDESC(176, "responses sent 176-191 bytes", "176-191", out);
	SET_SIZESTATDESC(192, "responses sent 192-207 bytes", "192-207", out);
	SET_SIZESTATDESC(208, "responses sent 208-223 bytes", "208-223", out);
	SET_SIZESTATDESC(224, "responses sent 224-239 bytes", "224-239", out);
	SET_SIZESTATDESC(240, "responses sent 240-255 bytes", "240-255", out);
	SET_SIZESTATDESC(256, "responses sent 256-271 bytes", "256-271", out);
	SET_SIZESTATDESC(272, "responses sent 272-287 bytes", "272-287", out);
	SET_SIZESTATDESC(288, "responses sent 288-303 bytes", "288-303", out);
	SET_SIZESTATDESC(304, "responses sent 304-319 bytes", "304-319", out);
	SET_SIZESTATDESC(320, "responses sent 320-335 bytes", "320-335", out);
	SET_SIZESTATDESC(336, "responses sent 336-351 bytes", "336-351", out);
	SET_SIZESTATDESC(352, "responses sent 352-367 bytes", "352-367", out);
	SET_SIZESTATDESC(368, "responses sent 368-383 bytes", "368-383", out);
	SET_SIZESTATDESC(384, "responses sent 384-399 bytes", "384-399", out);
	SET_SIZESTATDESC(400, "responses sent 400-415 bytes", "400-415", out);
	SET_SIZESTATDESC(416, "responses sent 416-431 bytes", "416-431", out);
	SET_SIZESTATDESC(432, "responses sent 432-447 bytes", "432-447", out);
	SET_SIZESTATDESC(448, "responses sent 448-463 bytes", "448-463", out);
	SET_SIZESTATDESC(464, "responses sent 464-479 bytes", "464-479", out);
	SET_SIZESTATDESC(480, "responses sent 480-495 bytes", "480-495", out);
	SET_SIZESTATDESC(496, "responses sent 496-511 bytes", "496-511", out);
	SET_SIZESTATDESC(512, "responses sent 512-527 bytes", "512-527", out);
	SET_SIZESTATDESC(528, "responses sent 528-543 bytes", "528-543", out);
	SET_SIZESTATDESC(544, "responses sent 544-559 bytes", "544-559", out);
	SET_SIZESTATDESC(560, "responses sent 560-575 bytes", "560-575", out);
	SET_SIZESTATDESC(576, "responses sent 576-591 bytes", "576-591", out);
	SET_SIZESTATDESC(592, "responses sent 592-607 bytes", "592-607", out);
	SET_SIZESTATDESC(608, "responses sent 608-623 bytes", "608-623", out);
	SET_SIZESTATDESC(624, "responses sent 624-639 bytes", "624-639", out);
	SET_SIZESTATDESC(640, "responses sent 640-655 bytes", "640-655", out);
	SET_SIZESTATDESC(656, "responses sent 656-671 bytes", "656-671", out);
	SET_SIZESTATDESC(672, "responses sent 672-687 bytes", "672-687", out);
	SET_SIZESTATDESC(688, "responses sent 688-703 bytes", "688-703", out);
	SET_SIZESTATDESC(704, "responses sent 704-719 bytes", "704-719", out);
	SET_SIZESTATDESC(720, "responses sent 720-735 bytes", "720-735", out);
	SET_SIZESTATDESC(736, "responses sent 736-751 bytes", "736-751", out);
	SET_SIZESTATDESC(752, "responses sent 752-767 bytes", "752-767", out);
	SET_SIZESTATDESC(768, "responses sent 768-783 bytes", "768-783", out);
	SET_SIZESTATDESC(784, "responses sent 784-799 bytes", "784-799", out);
	SET_SIZESTATDESC(800, "responses sent 800-815 bytes", "800-815", out);
	SET_SIZESTATDESC(816, "responses sent 816-831 bytes", "816-831", out);
	SET_SIZESTATDESC(832, "responses sent 832-847 bytes", "832-847", out);
	SET_SIZESTATDESC(848, "responses sent 848-863 bytes", "848-863", out);
	SET_SIZESTATDESC(864, "responses sent 864-879 bytes", "864-879", out);
	SET_SIZESTATDESC(880, "responses sent 880-895 bytes", "880-895", out);
	SET_SIZESTATDESC(896, "responses sent 896-911 bytes", "896-911", out);
	SET_SIZESTATDESC(912, "responses sent 912-927 bytes", "912-927", out);
	SET_SIZESTATDESC(928, "responses sent 928-943 bytes", "928-943", out);
	SET_SIZESTATDESC(944, "responses sent 944-959 bytes", "944-959", out);
	SET_SIZESTATDESC(960, "responses sent 960-975 bytes", "960-975", out);
	SET_SIZESTATDESC(976, "responses sent 976-991 bytes", "976-991", out);
	SET_SIZESTATDESC(992, "responses sent 992-1007 bytes", "992-1007", out);
	SET_SIZESTATDESC(1008, "responses sent 1008-1023 bytes", "1008-1023",
			 out);
	SET_SIZESTATDESC(1024, "responses sent 1024-1039 bytes", "1024-1039",
			 out);
	SET_SIZESTATDESC(1040, "responses sent 1040-1055 bytes", "1040-1055",
			 out);
	SET_SIZESTATDESC(1056, "responses sent 1056-1071 bytes", "1056-1071",
			 out);
	SET_SIZESTATDESC(1072, "responses sent 1072-1087 bytes", "1072-1087",
			 out);
	SET_SIZESTATDESC(1088, "responses sent 1088-1103 bytes", "1088-1103",
			 out);
	SET_SIZESTATDESC(1104, "responses sent 1104-1119 bytes", "1104-1119",
			 out);
	SET_SIZESTATDESC(1120, "responses sent 1120-1135 bytes", "1120-1135",
			 out);
	SET_SIZESTATDESC(1136, "responses sent 1136-1151 bytes", "1136-1151",
			 out);
	SET_SIZESTATDESC(1152, "responses sent 1152-1167 bytes", "1152-1167",
			 out);
	SET_SIZESTATDESC(1168, "responses sent 1168-1183 bytes", "1168-1183",
			 out);
	SET_SIZESTATDESC(1184, "responses sent 1184-1199 bytes", "1184-1199",
			 out);
	SET_SIZESTATDESC(1200, "responses sent 1200-1215 bytes", "1200-1215",
			 out);
	SET_SIZESTATDESC(1216, "responses sent 1216-1231 bytes", "1216-1231",
			 out);
	SET_SIZESTATDESC(1232, "responses sent 1232-1247 bytes", "1232-1247",
			 out);
	SET_SIZESTATDESC(1248, "responses sent 1248-1263 bytes", "1248-1263",
			 out);
	SET_SIZESTATDESC(1264, "responses sent 1264-1279 bytes", "1264-1279",
			 out);
	SET_SIZESTATDESC(1280, "responses sent 1280-1295 bytes", "1280-1295",
			 out);
	SET_SIZESTATDESC(1296, "responses sent 1296-1311 bytes", "1296-1311",
			 out);
	SET_SIZESTATDESC(1312, "responses sent 1312-1327 bytes", "1312-1327",
			 out);
	SET_SIZESTATDESC(1328, "responses sent 1328-1343 bytes", "1328-1343",
			 out);
	SET_SIZESTATDESC(1344, "responses sent 1344-1359 bytes", "1344-1359",
			 out);
	SET_SIZESTATDESC(1360, "responses sent 1360-1375 bytes", "1360-1375",
			 out);
	SET_SIZESTATDESC(1376, "responses sent 1376-1391 bytes", "1376-1391",
			 out);
	SET_SIZESTATDESC(1392, "responses sent 1392-1407 bytes", "1392-1407",
			 out);
	SET_SIZESTATDESC(1408, "responses sent 1408-1423 bytes", "1408-1423",
			 out);
	SET_SIZESTATDESC(1424, "responses sent 1424-1439 bytes", "1424-1439",
			 out);
	SET_SIZESTATDESC(1440, "responses sent 1440-1455 bytes", "1440-1455",
			 out);
	SET_SIZESTATDESC(1456, "responses sent 1456-1471 bytes", "1456-1471",
			 out);
	SET_SIZESTATDESC(1472, "responses sent 1472-1487 bytes", "1472-1487",
			 out);
	SET_SIZESTATDESC(1488, "responses sent 1488-1503 bytes", "1488-1503",
			 out);
	SET_SIZESTATDESC(1504, "responses sent 1504-1519 bytes", "1504-1519",
			 out);
	SET_SIZESTATDESC(1520, "responses sent 1520-1535 bytes", "1520-1535",
			 out);
	SET_SIZESTATDESC(1536, "responses sent 1536-1551 bytes", "1536-1551",
			 out);
	SET_SIZESTATDESC(1552, "responses sent 1552-1567 bytes", "1552-1567",
			 out);
	SET_SIZESTATDESC(1568, "responses sent 1568-1583 bytes", "1568-1583",
			 out);
	SET_SIZESTATDESC(1584, "responses sent 1584-1599 bytes", "1584-1599",
			 out);
	SET_SIZESTATDESC(1600, "responses sent 1600-1615 bytes", "1600-1615",
			 out);
	SET_SIZESTATDESC(1616, "responses sent 1616-1631 bytes", "1616-1631",
			 out);
	SET_SIZESTATDESC(1632, "responses sent 1632-1647 bytes", "1632-1647",
			 out);
	SET_SIZESTATDESC(1648, "responses sent 1648-1663 bytes", "1648-1663",
			 out);
	SET_SIZESTATDESC(1664, "responses sent 1664-1679 bytes", "1664-1679",
			 out);
	SET_SIZESTATDESC(1680, "responses sent 1680-1695 bytes", "1680-1695",
			 out);
	SET_SIZESTATDESC(1696, "responses sent 1696-1711 bytes", "1696-1711",
			 out);
	SET_SIZESTATDESC(1712, "responses sent 1712-1727 bytes", "1712-1727",
			 out);
	SET_SIZESTATDESC(1728, "responses sent 1728-1743 bytes", "1728-1743",
			 out);
	SET_SIZESTATDESC(1744, "responses sent 1744-1759 bytes", "1744-1759",
			 out);
	SET_SIZESTATDESC(1760, "responses sent 1760-1775 bytes", "1760-1775",
			 out);
	SET_SIZESTATDESC(1776, "responses sent 1776-1791 bytes", "1776-1791",
			 out);
	SET_SIZESTATDESC(1792, "responses sent 1792-1807 bytes", "1792-1807",
			 out);
	SET_SIZESTATDESC(1808, "responses sent 1808-1823 bytes", "1808-1823",
			 out);
	SET_SIZESTATDESC(1824, "responses sent 1824-1839 bytes", "1824-1839",
			 out);
	SET_SIZESTATDESC(1840, "responses sent 1840-1855 bytes", "1840-1855",
			 out);
	SET_SIZESTATDESC(1856, "responses sent 1856-1871 bytes", "1856-1871",
			 out);
	SET_SIZESTATDESC(1872, "responses sent 1872-1887 bytes", "1872-1887",
			 out);
	SET_SIZESTATDESC(1888, "responses sent 1888-1903 bytes", "1888-1903",
			 out);
	SET_SIZESTATDESC(1904, "responses sent 1904-1919 bytes", "1904-1919",
			 out);
	SET_SIZESTATDESC(1920, "responses sent 1920-1935 bytes", "1920-1935",
			 out);
	SET_SIZESTATDESC(1936, "responses sent 1936-1951 bytes", "1936-1951",
			 out);
	SET_SIZESTATDESC(1952, "responses sent 1952-1967 bytes", "1952-1967",
			 out);
	SET_SIZESTATDESC(1968, "responses sent 1968-1983 bytes", "1968-1983",
			 out);
	SET_SIZESTATDESC(1984, "responses sent 1984-1999 bytes", "1984-1999",
			 out);
	SET_SIZESTATDESC(2000, "responses sent 2000-2015 bytes", "2000-2015",
			 out);
	SET_SIZESTATDESC(2016, "responses sent 2016-2031 bytes", "2016-2031",
			 out);
	SET_SIZESTATDESC(2032, "responses sent 2032-2047 bytes", "2032-2047",
			 out);
	SET_SIZESTATDESC(2048, "responses sent 2048-2063 bytes", "2048-2063",
			 out);
	SET_SIZESTATDESC(2064, "responses sent 2064-2079 bytes", "2064-2079",
			 out);
	SET_SIZESTATDESC(2080, "responses sent 2080-2095 bytes", "2080-2095",
			 out);
	SET_SIZESTATDESC(2096, "responses sent 2096-2111 bytes", "2096-2111",
			 out);
	SET_SIZESTATDESC(2112, "responses sent 2112-2127 bytes", "2112-2127",
			 out);
	SET_SIZESTATDESC(2128, "responses sent 2128-2143 bytes", "2128-2143",
			 out);
	SET_SIZESTATDESC(2144, "responses sent 2144-2159 bytes", "2144-2159",
			 out);
	SET_SIZESTATDESC(2160, "responses sent 2160-2175 bytes", "2160-2175",
			 out);
	SET_SIZESTATDESC(2176, "responses sent 2176-2191 bytes", "2176-2191",
			 out);
	SET_SIZESTATDESC(2192, "responses sent 2192-2207 bytes", "2192-2207",
			 out);
	SET_SIZESTATDESC(2208, "responses sent 2208-2223 bytes", "2208-2223",
			 out);
	SET_SIZESTATDESC(2224, "responses sent 2224-2239 bytes", "2224-2239",
			 out);
	SET_SIZESTATDESC(2240, "responses sent 2240-2255 bytes", "2240-2255",
			 out);
	SET_SIZESTATDESC(2256, "responses sent 2256-2271 bytes", "2256-2271",
			 out);
	SET_SIZESTATDESC(2272, "responses sent 2272-2287 bytes", "2272-2287",
			 out);
	SET_SIZESTATDESC(2288, "responses sent 2288-2303 bytes", "2288-2303",
			 out);
	SET_SIZESTATDESC(2304, "responses sent 2304-2319 bytes", "2304-2319",
			 out);
	SET_SIZESTATDESC(2320, "responses sent 2320-2335 bytes", "2320-2335",
			 out);
	SET_SIZESTATDESC(2336, "responses sent 2336-2351 bytes", "2336-2351",
			 out);
	SET_SIZESTATDESC(2352, "responses sent 2352-2367 bytes", "2352-2367",
			 out);
	SET_SIZESTATDESC(2368, "responses sent 2368-2383 bytes", "2368-2383",
			 out);
	SET_SIZESTATDESC(2384, "responses sent 2384-2399 bytes", "2384-2399",
			 out);
	SET_SIZESTATDESC(2400, "responses sent 2400-2415 bytes", "2400-2415",
			 out);
	SET_SIZESTATDESC(2416, "responses sent 2416-2431 bytes", "2416-2431",
			 out);
	SET_SIZESTATDESC(2432, "responses sent 2432-2447 bytes", "2432-2447",
			 out);
	SET_SIZESTATDESC(2448, "responses sent 2448-2463 bytes", "2448-2463",
			 out);
	SET_SIZESTATDESC(2464, "responses sent 2464-2479 bytes", "2464-2479",
			 out);
	SET_SIZESTATDESC(2480, "responses sent 2480-2495 bytes", "2480-2495",
			 out);
	SET_SIZESTATDESC(2496, "responses sent 2496-2511 bytes", "2496-2511",
			 out);
	SET_SIZESTATDESC(2512, "responses sent 2512-2527 bytes", "2512-2527",
			 out);
	SET_SIZESTATDESC(2528, "responses sent 2528-2543 bytes", "2528-2543",
			 out);
	SET_SIZESTATDESC(2544, "responses sent 2544-2559 bytes", "2544-2559",
			 out);
	SET_SIZESTATDESC(2560, "responses sent 2560-2575 bytes", "2560-2575",
			 out);
	SET_SIZESTATDESC(2576, "responses sent 2576-2591 bytes", "2576-2591",
			 out);
	SET_SIZESTATDESC(2592, "responses sent 2592-2607 bytes", "2592-2607",
			 out);
	SET_SIZESTATDESC(2608, "responses sent 2608-2623 bytes", "2608-2623",
			 out);
	SET_SIZESTATDESC(2624, "responses sent 2624-2639 bytes", "2624-2639",
			 out);
	SET_SIZESTATDESC(2640, "responses sent 2640-2655 bytes", "2640-2655",
			 out);
	SET_SIZESTATDESC(2656, "responses sent 2656-2671 bytes", "2656-2671",
			 out);
	SET_SIZESTATDESC(2672, "responses sent 2672-2687 bytes", "2672-2687",
			 out);
	SET_SIZESTATDESC(2688, "responses sent 2688-2703 bytes", "2688-2703",
			 out);
	SET_SIZESTATDESC(2704, "responses sent 2704-2719 bytes", "2704-2719",
			 out);
	SET_SIZESTATDESC(2720, "responses sent 2720-2735 bytes", "2720-2735",
			 out);
	SET_SIZESTATDESC(2736, "responses sent 2736-2751 bytes", "2736-2751",
			 out);
	SET_SIZESTATDESC(2752, "responses sent 2752-2767 bytes", "2752-2767",
			 out);
	SET_SIZESTATDESC(2768, "responses sent 2768-2783 bytes", "2768-2783",
			 out);
	SET_SIZESTATDESC(2784, "responses sent 2784-2799 bytes", "2784-2799",
			 out);
	SET_SIZESTATDESC(2800, "responses sent 2800-2815 bytes", "2800-2815",
			 out);
	SET_SIZESTATDESC(2816, "responses sent 2816-2831 bytes", "2816-2831",
			 out);
	SET_SIZESTATDESC(2832, "responses sent 2832-2847 bytes", "2832-2847",
			 out);
	SET_SIZESTATDESC(2848, "responses sent 2848-2863 bytes", "2848-2863",
			 out);
	SET_SIZESTATDESC(2864, "responses sent 2864-2879 bytes", "2864-2879",
			 out);
	SET_SIZESTATDESC(2880, "responses sent 2880-2895 bytes", "2880-2895",
			 out);
	SET_SIZESTATDESC(2896, "responses sent 2896-2911 bytes", "2896-2911",
			 out);
	SET_SIZESTATDESC(2912, "responses sent 2912-2927 bytes", "2912-2927",
			 out);
	SET_SIZESTATDESC(2928, "responses sent 2928-2943 bytes", "2928-2943",
			 out);
	SET_SIZESTATDESC(2944, "responses sent 2944-2959 bytes", "2944-2959",
			 out);
	SET_SIZESTATDESC(2960, "responses sent 2960-2975 bytes", "2960-2975",
			 out);
	SET_SIZESTATDESC(2976, "responses sent 2976-2991 bytes", "2976-2991",
			 out);
	SET_SIZESTATDESC(2992, "responses sent 2992-3007 bytes", "2992-3007",
			 out);
	SET_SIZESTATDESC(3008, "responses sent 3008-3023 bytes", "3008-3023",
			 out);
	SET_SIZESTATDESC(3024, "responses sent 3024-3039 bytes", "3024-3039",
			 out);
	SET_SIZESTATDESC(3040, "responses sent 3040-3055 bytes", "3040-3055",
			 out);
	SET_SIZESTATDESC(3056, "responses sent 3056-3071 bytes", "3056-3071",
			 out);
	SET_SIZESTATDESC(3072, "responses sent 3072-3087 bytes", "3072-3087",
			 out);
	SET_SIZESTATDESC(3088, "responses sent 3088-3103 bytes", "3088-3103",
			 out);
	SET_SIZESTATDESC(3104, "responses sent 3104-3119 bytes", "3104-3119",
			 out);
	SET_SIZESTATDESC(3120, "responses sent 3120-3135 bytes", "3120-3135",
			 out);
	SET_SIZESTATDESC(3136, "responses sent 3136-3151 bytes", "3136-3151",
			 out);
	SET_SIZESTATDESC(3152, "responses sent 3152-3167 bytes", "3152-3167",
			 out);
	SET_SIZESTATDESC(3168, "responses sent 3168-3183 bytes", "3168-3183",
			 out);
	SET_SIZESTATDESC(3184, "responses sent 3184-3199 bytes", "3184-3199",
			 out);
	SET_SIZESTATDESC(3200, "responses sent 3200-3215 bytes", "3200-3215",
			 out);
	SET_SIZESTATDESC(3216, "responses sent 3216-3231 bytes", "3216-3231",
			 out);
	SET_SIZESTATDESC(3232, "responses sent 3232-3247 bytes", "3232-3247",
			 out);
	SET_SIZESTATDESC(3248, "responses sent 3248-3263 bytes", "3248-3263",
			 out);
	SET_SIZESTATDESC(3264, "responses sent 3264-3279 bytes", "3264-3279",
			 out);
	SET_SIZESTATDESC(3280, "responses sent 3280-3295 bytes", "3280-3295",
			 out);
	SET_SIZESTATDESC(3296, "responses sent 3296-3311 bytes", "3296-3311",
			 out);
	SET_SIZESTATDESC(3312, "responses sent 3312-3327 bytes", "3312-3327",
			 out);
	SET_SIZESTATDESC(3328, "responses sent 3328-3343 bytes", "3328-3343",
			 out);
	SET_SIZESTATDESC(3344, "responses sent 3344-3359 bytes", "3344-3359",
			 out);
	SET_SIZESTATDESC(3360, "responses sent 3360-3375 bytes", "3360-3375",
			 out);
	SET_SIZESTATDESC(3376, "responses sent 3376-3391 bytes", "3376-3391",
			 out);
	SET_SIZESTATDESC(3392, "responses sent 3392-3407 bytes", "3392-3407",
			 out);
	SET_SIZESTATDESC(3408, "responses sent 3408-3423 bytes", "3408-3423",
			 out);
	SET_SIZESTATDESC(3424, "responses sent 3424-3439 bytes", "3424-3439",
			 out);
	SET_SIZESTATDESC(3440, "responses sent 3440-3455 bytes", "3440-3455",
			 out);
	SET_SIZESTATDESC(3456, "responses sent 3456-3471 bytes", "3456-3471",
			 out);
	SET_SIZESTATDESC(3472, "responses sent 3472-3487 bytes", "3472-3487",
			 out);
	SET_SIZESTATDESC(3488, "responses sent 3488-3503 bytes", "3488-3503",
			 out);
	SET_SIZESTATDESC(3504, "responses sent 3504-3519 bytes", "3504-3519",
			 out);
	SET_SIZESTATDESC(3520, "responses sent 3520-3535 bytes", "3520-3535",
			 out);
	SET_SIZESTATDESC(3536, "responses sent 3536-3551 bytes", "3536-3551",
			 out);
	SET_SIZESTATDESC(3552, "responses sent 3552-3567 bytes", "3552-3567",
			 out);
	SET_SIZESTATDESC(3568, "responses sent 3568-3583 bytes", "3568-3583",
			 out);
	SET_SIZESTATDESC(3584, "responses sent 3584-3599 bytes", "3584-3599",
			 out);
	SET_SIZESTATDESC(3600, "responses sent 3600-3615 bytes", "3600-3615",
			 out);
	SET_SIZESTATDESC(3616, "responses sent 3616-3631 bytes", "3616-3631",
			 out);
	SET_SIZESTATDESC(3632, "responses sent 3632-3647 bytes", "3632-3647",
			 out);
	SET_SIZESTATDESC(3648, "responses sent 3648-3663 bytes", "3648-3663",
			 out);
	SET_SIZESTATDESC(3664, "responses sent 3664-3679 bytes", "3664-3679",
			 out);
	SET_SIZESTATDESC(3680, "responses sent 3680-3695 bytes", "3680-3695",
			 out);
	SET_SIZESTATDESC(3696, "responses sent 3696-3711 bytes", "3696-3711",
			 out);
	SET_SIZESTATDESC(3712, "responses sent 3712-3727 bytes", "3712-3727",
			 out);
	SET_SIZESTATDESC(3728, "responses sent 3728-3743 bytes", "3728-3743",
			 out);
	SET_SIZESTATDESC(3744, "responses sent 3744-3759 bytes", "3744-3759",
			 out);
	SET_SIZESTATDESC(3760, "responses sent 3760-3775 bytes", "3760-3775",
			 out);
	SET_SIZESTATDESC(3776, "responses sent 3776-3791 bytes", "3776-3791",
			 out);
	SET_SIZESTATDESC(3792, "responses sent 3792-3807 bytes", "3792-3807",
			 out);
	SET_SIZESTATDESC(3808, "responses sent 3808-3823 bytes", "3808-3823",
			 out);
	SET_SIZESTATDESC(3824, "responses sent 3824-3839 bytes", "3824-3839",
			 out);
	SET_SIZESTATDESC(3840, "responses sent 3840-3855 bytes", "3840-3855",
			 out);
	SET_SIZESTATDESC(3856, "responses sent 3856-3871 bytes", "3856-3871",
			 out);
	SET_SIZESTATDESC(3872, "responses sent 3872-3887 bytes", "3872-3887",
			 out);
	SET_SIZESTATDESC(3888, "responses sent 3888-3903 bytes", "3888-3903",
			 out);
	SET_SIZESTATDESC(3904, "responses sent 3904-3919 bytes", "3904-3919",
			 out);
	SET_SIZESTATDESC(3920, "responses sent 3920-3935 bytes", "3920-3935",
			 out);
	SET_SIZESTATDESC(3936, "responses sent 3936-3951 bytes", "3936-3951",
			 out);
	SET_SIZESTATDESC(3952, "responses sent 3952-3967 bytes", "3952-3967",
			 out);
	SET_SIZESTATDESC(3968, "responses sent 3968-3983 bytes", "3968-3983",
			 out);
	SET_SIZESTATDESC(3984, "responses sent 3984-3999 bytes", "3984-3999",
			 out);
	SET_SIZESTATDESC(4000, "responses sent 4000-4015 bytes", "4000-4015",
			 out);
	SET_SIZESTATDESC(4016, "responses sent 4016-4031 bytes", "4016-4031",
			 out);
	SET_SIZESTATDESC(4032, "responses sent 4032-4047 bytes", "4032-4047",
			 out);
	SET_SIZESTATDESC(4048, "responses sent 4048-4063 bytes", "4048-4063",
			 out);
	SET_SIZESTATDESC(4064, "responses sent 4064-4079 bytes", "4064-4079",
			 out);
	SET_SIZESTATDESC(4080, "responses sent 4080-4095 bytes", "4080-4095",
			 out);
	SET_SIZESTATDESC(4096, "responses sent 4096+ bytes", "4096+", out);
	INSIST(i == dns_sizecounter_out_max);

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
	for (i = 0; i < dns_sizecounter_in_max; i++) {
		INSIST(udpinsizestats_desc[i] != NULL);
		INSIST(tcpinsizestats_desc[i] != NULL);
	}
	for (i = 0; i < dns_sizecounter_out_max; i++) {
		INSIST(udpoutsizestats_desc[i] != NULL);
		INSIST(tcpoutsizestats_desc[i] != NULL);
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
	for (i = 0; i < dns_sizecounter_in_max; i++) {
		INSIST(udpinsizestats_xmldesc[i] != NULL);
		INSIST(tcpinsizestats_xmldesc[i] != NULL);
	}
	for (i = 0; i < dns_sizecounter_out_max; i++) {
		INSIST(udpoutsizestats_xmldesc[i] != NULL);
		INSIST(tcpoutsizestats_xmldesc[i] != NULL);
	}
#endif /* if defined(EXTENDED_STATS) */
}

/*%
 * Dump callback functions.
 */
static void
generalstat_dump(isc_statscounter_t counter, uint64_t val, void *arg) {
	stats_dumparg_t *dumparg = arg;

	REQUIRE(counter < dumparg->ncounters);
	dumparg->countervalues[counter] = val;
}

static isc_result_t
dump_counters(isc_stats_t *stats, isc_statsformat_t type, void *arg,
	      const char *category, const char **desc, int ncounters,
	      int *indices, uint64_t *values, int options) {
	int i, idx;
	uint64_t value;
	stats_dumparg_t dumparg;
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

	dumparg.type = type;
	dumparg.ncounters = ncounters;
	dumparg.counterindices = indices;
	dumparg.countervalues = values;

	memset(values, 0, sizeof(values[0]) * ncounters);
	isc_stats_dump(stats, generalstat_dump, &dumparg, options);

#ifdef HAVE_JSON_C
	cat = job = (json_object *)arg;
	if (ncounters > 0 && type == isc_statsformat_json) {
		if (category != NULL) {
			cat = json_object_new_object();
			if (cat == NULL) {
				return (ISC_R_NOMEMORY);
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

		switch (dumparg.type) {
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
				return (ISC_R_NOMEMORY);
			}
			json_object_object_add(cat, desc[idx], counter);
#endif /* ifdef HAVE_JSON_C */
			break;
		}
	}
	return (ISC_R_SUCCESS);
#ifdef HAVE_LIBXML2
error:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at dump_counters()");
	return (ISC_R_FAILURE);
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
error:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at rdtypestat_dump()");
	dumparg->result = ISC_R_FAILURE;
	return;
#endif /* ifdef HAVE_LIBXML2 */
}

static bool
rdatastatstype_attr(dns_rdatastatstype_t type, unsigned int attr) {
	return ((DNS_RDATASTATSTYPE_ATTR(type) & attr) != 0);
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
error:
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
error:
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
error:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed at rcodestat_dump()");
	dumparg->result = ISC_R_FAILURE;
	return;
#endif /* ifdef HAVE_LIBXML2 */
}

#if defined(EXTENDED_STATS)
static void
dnssecsignstat_dump(dns_keytag_t tag, uint64_t val, void *arg) {
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

	snprintf(tagbuf, sizeof(tagbuf), "%u", tag);

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
error:
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
#define STATS_XML_TASKS	  0x04
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

	statlevel = dns_zone_getstatlevel(zone);
	if (statlevel == dns_zonestat_none) {
		return (ISC_R_SUCCESS);
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
	 * master zones, only include the loaded time.  For slave zones, also
	 * include the expires and refresh times.
	 */
	isc_time_t timestamp;

	result = dns_zone_getloadtime(zone, &timestamp);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	isc_time_formatISO8601(&timestamp, buf, 64);
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "loaded"));
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR buf));
	TRY0(xmlTextWriterEndElement(writer));

	if (dns_zone_gettype(zone) == dns_zone_secondary) {
		result = dns_zone_getexpiretime(zone, &timestamp);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}
		isc_time_formatISO8601(&timestamp, buf, 64);
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "expires"));
		TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR buf));
		TRY0(xmlTextWriterEndElement(writer));

		result = dns_zone_getrefreshtime(zone, &timestamp);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}
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

			result = dump_counters(zonestats, isc_statsformat_xml,
					       writer, NULL, nsstats_xmldesc,
					       ns_statscounter_max,
					       nsstats_index, nsstat_values,
					       ISC_STATSDUMP_VERBOSE);
			if (result != ISC_R_SUCCESS) {
				goto error;
			}
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

			result = dump_counters(
				gluecachestats, isc_statsformat_xml, writer,
				NULL, gluecachestats_xmldesc,
				dns_gluecachestatscounter_max,
				gluecachestats_index, gluecachestats_values,
				ISC_STATSDUMP_VERBOSE);
			if (result != ISC_R_SUCCESS) {
				goto error;
			}
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
			if (dumparg.result != ISC_R_SUCCESS) {
				goto error;
			}

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
			if (dumparg.result != ISC_R_SUCCESS) {
				goto error;
			}

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
			if (dumparg.result != ISC_R_SUCCESS) {
				goto error;
			}

			/* counters type="dnssec-refresh"*/
			TRY0(xmlTextWriterEndElement(writer));
		}
	}

	TRY0(xmlTextWriterEndElement(writer)); /* zone */

	return (ISC_R_SUCCESS);
error:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "Failed at zone_xmlrender()");
	return (ISC_R_FAILURE);
}

static isc_result_t
generatexml(named_server_t *server, uint32_t flags, int *buflen,
	    xmlChar **buf) {
	char boottime[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	char configtime[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	char nowstr[sizeof "yyyy-mm-ddThh:mm:ss.sssZ"];
	isc_time_t now;
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
	uint64_t udpinsizestat_values[dns_sizecounter_in_max];
	uint64_t udpoutsizestat_values[dns_sizecounter_out_max];
	uint64_t tcpinsizestat_values[dns_sizecounter_in_max];
	uint64_t tcpoutsizestat_values[dns_sizecounter_out_max];
#ifdef HAVE_DNSTAP
	uint64_t dnstapstat_values[dns_dnstapcounter_max];
#endif /* ifdef HAVE_DNSTAP */
	isc_result_t result;

	isc_time_now(&now);
	isc_time_formatISO8601ms(&named_g_boottime, boottime, sizeof boottime);
	isc_time_formatISO8601ms(&named_g_configtime, configtime,
				 sizeof configtime);
	isc_time_formatISO8601ms(&now, nowstr, sizeof nowstr);

	writer = xmlNewTextWriterDoc(&doc, 0);
	if (writer == NULL) {
		goto error;
	}
	TRY0(xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL));
	TRY0(xmlTextWriterWritePI(writer, ISC_XMLCHAR "xml-stylesheet",
				  ISC_XMLCHAR "type=\"text/xsl\" "
					      "href=\"/bind9.xsl\""));
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "statistics"));
	TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "version",
					 ISC_XMLCHAR "3.11.1"));

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
	TRY0(xmlTextWriterWriteString(writer, ISC_XMLCHAR named_g_version));
	TRY0(xmlTextWriterEndElement(writer)); /* version */

	if ((flags & STATS_XML_SERVER) != 0) {
		dumparg.result = ISC_R_SUCCESS;

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "opcode"));

		dns_opcodestats_dump(server->sctx->opcodestats, opcodestat_dump,
				     &dumparg, ISC_STATSDUMP_VERBOSE);
		if (dumparg.result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer));

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "rcode"));

		dns_rcodestats_dump(server->sctx->rcodestats, rcodestat_dump,
				    &dumparg, ISC_STATSDUMP_VERBOSE);
		if (dumparg.result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer));

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "qtype"));

		dumparg.result = ISC_R_SUCCESS;
		dns_rdatatypestats_dump(server->sctx->rcvquerystats,
					rdtypestat_dump, &dumparg, 0);
		if (dumparg.result != ISC_R_SUCCESS) {
			goto error;
		}
		TRY0(xmlTextWriterEndElement(writer)); /* counters */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "nsstat"));

		result = dump_counters(ns_stats_get(server->sctx->nsstats),
				       isc_statsformat_xml, writer, NULL,
				       nsstats_xmldesc, ns_statscounter_max,
				       nsstats_index, nsstat_values,
				       ISC_STATSDUMP_VERBOSE);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* /nsstat */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "zonestat"));

		result = dump_counters(server->zonestats, isc_statsformat_xml,
				       writer, NULL, zonestats_xmldesc,
				       dns_zonestatscounter_max,
				       zonestats_index, zonestat_values,
				       ISC_STATSDUMP_VERBOSE);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* /zonestat */

		/*
		 * Most of the common resolver statistics entries are 0, so
		 * we don't use the verbose dump here.
		 */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "resstat"));
		result = dump_counters(
			server->resolverstats, isc_statsformat_xml, writer,
			NULL, resstats_xmldesc, dns_resstatscounter_max,
			resstats_index, resstat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}
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
			result = dump_counters(
				dnstapstats, isc_statsformat_xml, writer, NULL,
				dnstapstats_xmldesc, dns_dnstapcounter_max,
				dnstapstats_index, dnstapstat_values, 0);
			isc_stats_detach(&dnstapstats);
			if (result != ISC_R_SUCCESS) {
				goto error;
			}
			TRY0(xmlTextWriterEndElement(writer)); /* dnstap */
		}
#endif /* ifdef HAVE_DNSTAP */
	}

	if ((flags & STATS_XML_NET) != 0) {
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "sockstat"));

		result = dump_counters(server->sockstats, isc_statsformat_xml,
				       writer, NULL, sockstats_xmldesc,
				       isc_sockstatscounter_max,
				       sockstats_index, sockstat_values,
				       ISC_STATSDUMP_VERBOSE);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

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

		result = dump_counters(
			server->sctx->udpinstats4, isc_statsformat_xml, writer,
			NULL, udpinsizestats_xmldesc, dns_sizecounter_in_max,
			udpinsizestats_index, udpinsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		result = dump_counters(
			server->sctx->udpoutstats4, isc_statsformat_xml, writer,
			NULL, udpoutsizestats_xmldesc, dns_sizecounter_out_max,
			udpoutsizestats_index, udpoutsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterEndElement(writer)); /* </udp> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tcp"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "request-size"));

		result = dump_counters(
			server->sctx->tcpinstats4, isc_statsformat_xml, writer,
			NULL, tcpinsizestats_xmldesc, dns_sizecounter_in_max,
			tcpinsizestats_index, tcpinsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		result = dump_counters(
			server->sctx->tcpoutstats4, isc_statsformat_xml, writer,
			NULL, tcpoutsizestats_xmldesc, dns_sizecounter_out_max,
			tcpoutsizestats_index, tcpoutsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterEndElement(writer)); /* </tcp> */
		TRY0(xmlTextWriterEndElement(writer)); /* </ipv4> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "ipv6"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "udp"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "request-size"));

		result = dump_counters(
			server->sctx->udpinstats6, isc_statsformat_xml, writer,
			NULL, udpinsizestats_xmldesc, dns_sizecounter_in_max,
			udpinsizestats_index, udpinsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		result = dump_counters(
			server->sctx->udpoutstats6, isc_statsformat_xml, writer,
			NULL, udpoutsizestats_xmldesc, dns_sizecounter_out_max,
			udpoutsizestats_index, udpoutsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */
		TRY0(xmlTextWriterEndElement(writer)); /* </udp> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "tcp"));
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "request-size"));

		result = dump_counters(
			server->sctx->tcpinstats6, isc_statsformat_xml, writer,
			NULL, tcpinsizestats_xmldesc, dns_sizecounter_in_max,
			tcpinsizestats_index, tcpinsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

		TRY0(xmlTextWriterEndElement(writer)); /* </counters> */

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "response-size"));

		result = dump_counters(
			server->sctx->tcpoutstats6, isc_statsformat_xml, writer,
			NULL, tcpoutsizestats_xmldesc, dns_sizecounter_out_max,
			tcpoutsizestats_index, tcpoutsizestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}

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
	while (view != NULL &&
	       ((flags & (STATS_XML_SERVER | STATS_XML_ZONES)) != 0))
	{
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "view"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "name",
						 ISC_XMLCHAR view->name));

		if ((flags & STATS_XML_ZONES) != 0) {
			TRY0(xmlTextWriterStartElement(writer,
						       ISC_XMLCHAR "zones"));
			CHECK(dns_zt_apply(view->zonetable, isc_rwlocktype_read,
					   true, NULL, zone_xmlrender, writer));
			TRY0(xmlTextWriterEndElement(writer)); /* /zones */
		}

		if ((flags & STATS_XML_SERVER) == 0) {
			TRY0(xmlTextWriterEndElement(writer)); /* /view */
			view = ISC_LIST_NEXT(view, link);
			continue;
		}

		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "resqtype"));

		if (view->resquerystats != NULL) {
			dumparg.result = ISC_R_SUCCESS;
			dns_rdatatypestats_dump(view->resquerystats,
						rdtypestat_dump, &dumparg, 0);
			if (dumparg.result != ISC_R_SUCCESS) {
				goto error;
			}
		}
		TRY0(xmlTextWriterEndElement(writer));

		/* <resstats> */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "resstats"));
		if (view->resstats != NULL) {
			result = dump_counters(
				view->resstats, isc_statsformat_xml, writer,
				NULL, resstats_xmldesc, dns_resstatscounter_max,
				resstats_index, resstat_values,
				ISC_STATSDUMP_VERBOSE);
			if (result != ISC_R_SUCCESS) {
				goto error;
			}
		}
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
			if (dumparg.result != ISC_R_SUCCESS) {
				goto error;
			}
			TRY0(xmlTextWriterEndElement(writer)); /* cache */
		}

		/* <adbstats> */
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "counters"));
		TRY0(xmlTextWriterWriteAttribute(writer, ISC_XMLCHAR "type",
						 ISC_XMLCHAR "adbstat"));
		if (view->adbstats != NULL) {
			result = dump_counters(
				view->adbstats, isc_statsformat_xml, writer,
				NULL, adbstats_xmldesc, dns_adbstats_max,
				adbstats_index, adbstat_values,
				ISC_STATSDUMP_VERBOSE);
			if (result != ISC_R_SUCCESS) {
				goto error;
			}
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

	if ((flags & STATS_XML_NET) != 0) {
		TRY0(xmlTextWriterStartElement(writer,
					       ISC_XMLCHAR "socketmgr"));
		TRY0(isc_socketmgr_renderxml(named_g_socketmgr, writer));
		TRY0(xmlTextWriterEndElement(writer)); /* /socketmgr */
	}

	if ((flags & STATS_XML_TASKS) != 0) {
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "taskmgr"));
		TRY0(isc_taskmgr_renderxml(named_g_taskmgr, writer));
		TRY0(xmlTextWriterEndElement(writer)); /* /taskmgr */
	}

	if ((flags & STATS_XML_MEM) != 0) {
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "memory"));
		TRY0(isc_mem_renderxml(writer));
		TRY0(xmlTextWriterEndElement(writer)); /* /memory */
	}

	TRY0(xmlTextWriterEndElement(writer)); /* /statistics */
	TRY0(xmlTextWriterEndDocument(writer));

	xmlDocDumpFormatMemoryEnc(doc, buf, buflen, "UTF-8", 0);
	if (*buf == NULL) {
		goto error;
	}

	xmlFreeTextWriter(writer);
	xmlFreeDoc(doc);
	return (ISC_R_SUCCESS);

error:
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
		      "failed generating XML response");
	if (writer != NULL) {
		xmlFreeTextWriter(writer);
	}
	if (doc != NULL) {
		xmlFreeDoc(doc);
	}
	return (ISC_R_FAILURE);
}

static void
wrap_xmlfree(isc_buffer_t *buffer, void *arg) {
	UNUSED(arg);

	xmlFree(isc_buffer_base(buffer));
}

static isc_result_t
render_xml(uint32_t flags, const char *url, isc_httpdurl_t *urlinfo,
	   const char *querystring, const char *headers, void *arg,
	   unsigned int *retcode, const char **retmsg, const char **mimetype,
	   isc_buffer_t *b, isc_httpdfree_t **freecb, void **freecb_args) {
	unsigned char *msg = NULL;
	int msglen;
	named_server_t *server = arg;
	isc_result_t result;

	UNUSED(url);
	UNUSED(urlinfo);
	UNUSED(headers);
	UNUSED(querystring);

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

	return (result);
}

static isc_result_t
render_xml_all(const char *url, isc_httpdurl_t *urlinfo,
	       const char *querystring, const char *headers, void *arg,
	       unsigned int *retcode, const char **retmsg,
	       const char **mimetype, isc_buffer_t *b, isc_httpdfree_t **freecb,
	       void **freecb_args) {
	return (render_xml(STATS_XML_ALL, url, urlinfo, querystring, headers,
			   arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

static isc_result_t
render_xml_status(const char *url, isc_httpdurl_t *urlinfo,
		  const char *querystring, const char *headers, void *arg,
		  unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_xml(STATS_XML_STATUS, url, urlinfo, querystring, headers,
			   arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

static isc_result_t
render_xml_server(const char *url, isc_httpdurl_t *urlinfo,
		  const char *querystring, const char *headers, void *arg,
		  unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_xml(STATS_XML_SERVER, url, urlinfo, querystring, headers,
			   arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

static isc_result_t
render_xml_zones(const char *url, isc_httpdurl_t *urlinfo,
		 const char *querystring, const char *headers, void *arg,
		 unsigned int *retcode, const char **retmsg,
		 const char **mimetype, isc_buffer_t *b,
		 isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_xml(STATS_XML_ZONES, url, urlinfo, querystring, headers,
			   arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

static isc_result_t
render_xml_net(const char *url, isc_httpdurl_t *urlinfo,
	       const char *querystring, const char *headers, void *arg,
	       unsigned int *retcode, const char **retmsg,
	       const char **mimetype, isc_buffer_t *b, isc_httpdfree_t **freecb,
	       void **freecb_args) {
	return (render_xml(STATS_XML_NET, url, urlinfo, querystring, headers,
			   arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

static isc_result_t
render_xml_tasks(const char *url, isc_httpdurl_t *urlinfo,
		 const char *querystring, const char *headers, void *arg,
		 unsigned int *retcode, const char **retmsg,
		 const char **mimetype, isc_buffer_t *b,
		 isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_xml(STATS_XML_TASKS, url, urlinfo, querystring, headers,
			   arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

static isc_result_t
render_xml_mem(const char *url, isc_httpdurl_t *urlinfo,
	       const char *querystring, const char *headers, void *arg,
	       unsigned int *retcode, const char **retmsg,
	       const char **mimetype, isc_buffer_t *b, isc_httpdfree_t **freecb,
	       void **freecb_args) {
	return (render_xml(STATS_XML_MEM, url, urlinfo, querystring, headers,
			   arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

static isc_result_t
render_xml_traffic(const char *url, isc_httpdurl_t *urlinfo,
		   const char *querystring, const char *headers, void *arg,
		   unsigned int *retcode, const char **retmsg,
		   const char **mimetype, isc_buffer_t *b,
		   isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_xml(STATS_XML_TRAFFIC, url, urlinfo, querystring,
			   headers, arg, retcode, retmsg, mimetype, b, freecb,
			   freecb_args));
}

#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
/*
 * Which statistics to include when rendering to JSON
 */
#define STATS_JSON_STATUS  0x00 /* display only common statistics */
#define STATS_JSON_SERVER  0x01
#define STATS_JSON_ZONES   0x02
#define STATS_JSON_TASKS   0x04
#define STATS_JSON_NET	   0x08
#define STATS_JSON_MEM	   0x10
#define STATS_JSON_TRAFFIC 0x20
#define STATS_JSON_ALL	   0xff

#define CHECKMEM(m)                              \
	do {                                     \
		if (m == NULL) {                 \
			result = ISC_R_NOMEMORY; \
			goto error;              \
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
		return (NULL);
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
	return (node);
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

	statlevel = dns_zone_getstatlevel(zone);
	if (statlevel == dns_zonestat_none) {
		return (ISC_R_SUCCESS);
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
		return (ISC_R_NOMEMORY);
	}

	/*
	 * Export zone timers to the statistics channel in JSON format.  For
	 * master zones, only include the loaded time.  For slave zones, also
	 * include the expires and refresh times.
	 */

	isc_time_t timestamp;

	result = dns_zone_getloadtime(zone, &timestamp);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}

	isc_time_formatISO8601(&timestamp, buf, 64);
	json_object_object_add(zoneobj, "loaded", json_object_new_string(buf));

	if (dns_zone_gettype(zone) == dns_zone_secondary) {
		result = dns_zone_getexpiretime(zone, &timestamp);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}
		isc_time_formatISO8601(&timestamp, buf, 64);
		json_object_object_add(zoneobj, "expires",
				       json_object_new_string(buf));

		result = dns_zone_getrefreshtime(zone, &timestamp);
		if (result != ISC_R_SUCCESS) {
			goto error;
		}
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
				goto error;
			}

			result = dump_counters(zonestats, isc_statsformat_json,
					       counters, NULL, nsstats_xmldesc,
					       ns_statscounter_max,
					       nsstats_index, nsstat_values, 0);
			if (result != ISC_R_SUCCESS) {
				json_object_put(counters);
				goto error;
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
				goto error;
			}

			result = dump_counters(
				gluecachestats, isc_statsformat_json, counters,
				NULL, gluecachestats_xmldesc,
				dns_gluecachestatscounter_max,
				gluecachestats_index, gluecachestats_values, 0);
			if (result != ISC_R_SUCCESS) {
				json_object_put(counters);
				goto error;
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
				goto error;
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
				goto error;
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
				goto error;
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

error:
	if (zoneobj != NULL) {
		json_object_put(zoneobj);
	}
	return (result);
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
		return (ISC_R_NOMEMORY);
	}

	/*
	 * These statistics are included no matter which URL we use.
	 */
	obj = json_object_new_string("1.5.1");
	CHECKMEM(obj);
	json_object_object_add(bindstats, "json-stats-version", obj);

	isc_time_now(&now);
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
	obj = json_object_new_string(named_g_version);
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
			goto error;
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
			goto error;
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
			goto error;
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

		result = dump_counters(ns_stats_get(server->sctx->nsstats),
				       isc_statsformat_json, counters, NULL,
				       nsstats_xmldesc, ns_statscounter_max,
				       nsstats_index, nsstat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto error;
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

		result = dump_counters(server->zonestats, isc_statsformat_json,
				       counters, NULL, zonestats_xmldesc,
				       dns_zonestatscounter_max,
				       zonestats_index, zonestat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto error;
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

		result = dump_counters(
			server->resolverstats, isc_statsformat_json, counters,
			NULL, resstats_xmldesc, dns_resstatscounter_max,
			resstats_index, resstat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto error;
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
			result = dump_counters(
				dnstapstats, isc_statsformat_json, counters,
				NULL, dnstapstats_xmldesc,
				dns_dnstapcounter_max, dnstapstats_index,
				dnstapstat_values, 0);
			isc_stats_detach(&dnstapstats);
			if (result != ISC_R_SUCCESS) {
				json_object_put(counters);
				goto error;
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

	if ((flags & (STATS_JSON_ZONES | STATS_JSON_SERVER)) != 0) {
		viewlist = json_object_new_object();
		CHECKMEM(viewlist);

		json_object_object_add(bindstats, "views", viewlist);

		view = ISC_LIST_HEAD(server->viewlist);
		while (view != NULL) {
			json_object *za, *v = json_object_new_object();

			CHECKMEM(v);
			json_object_object_add(viewlist, view->name, v);

			za = json_object_new_array();
			CHECKMEM(za);

			if ((flags & STATS_JSON_ZONES) != 0) {
				CHECK(dns_zt_apply(view->zonetable,
						   isc_rwlocktype_read, true,
						   NULL, zone_jsonrender, za));
			}

			if (json_object_array_length(za) != 0) {
				json_object_object_add(v, "zones", za);
			} else {
				json_object_put(za);
			}

			if ((flags & STATS_JSON_SERVER) != 0) {
				json_object *res;
				dns_stats_t *dstats;
				isc_stats_t *istats;

				res = json_object_new_object();
				CHECKMEM(res);
				json_object_object_add(v, "resolver", res);

				istats = view->resstats;
				if (istats != NULL) {
					counters = json_object_new_object();
					CHECKMEM(counters);

					result = dump_counters(
						istats, isc_statsformat_json,
						counters, NULL,
						resstats_xmldesc,
						dns_resstatscounter_max,
						resstats_index, resstat_values,
						0);
					if (result != ISC_R_SUCCESS) {
						json_object_put(counters);
						result = dumparg.result;
						goto error;
					}

					json_object_object_add(res, "stats",
							       counters);
				}

				dstats = view->resquerystats;
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
						goto error;
					}

					json_object_object_add(res, "qtypes",
							       counters);
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
						goto error;
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
					goto error;
				}

				json_object_object_add(res, "cachestats",
						       counters);

				istats = view->adbstats;
				if (istats != NULL) {
					counters = json_object_new_object();
					CHECKMEM(counters);

					result = dump_counters(
						istats, isc_statsformat_json,
						counters, NULL,
						adbstats_xmldesc,
						dns_adbstats_max,
						adbstats_index, adbstat_values,
						0);
					if (result != ISC_R_SUCCESS) {
						json_object_put(counters);
						result = dumparg.result;
						goto error;
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
		json_object *sockets;
		counters = json_object_new_object();

		dumparg.result = ISC_R_SUCCESS;
		dumparg.arg = counters;

		result = dump_counters(server->sockstats, isc_statsformat_json,
				       counters, NULL, sockstats_xmldesc,
				       isc_sockstatscounter_max,
				       sockstats_index, sockstat_values, 0);
		if (result != ISC_R_SUCCESS) {
			json_object_put(counters);
			goto error;
		}

		if (json_object_get_object(counters)->count != 0) {
			json_object_object_add(bindstats, "sockstats",
					       counters);
		} else {
			json_object_put(counters);
		}

		sockets = json_object_new_object();
		CHECKMEM(sockets);

		result = isc_socketmgr_renderjson(named_g_socketmgr, sockets);
		if (result != ISC_R_SUCCESS) {
			json_object_put(sockets);
			goto error;
		}

		json_object_object_add(bindstats, "socketmgr", sockets);
	}

	if ((flags & STATS_JSON_TASKS) != 0) {
		json_object *tasks = json_object_new_object();
		CHECKMEM(tasks);

		result = isc_taskmgr_renderjson(named_g_taskmgr, tasks);
		if (result != ISC_R_SUCCESS) {
			json_object_put(tasks);
			goto error;
		}

		json_object_object_add(bindstats, "taskmgr", tasks);
	}

	if ((flags & STATS_JSON_MEM) != 0) {
		json_object *memory = json_object_new_object();
		CHECKMEM(memory);

		result = isc_mem_renderjson(memory);
		if (result != ISC_R_SUCCESS) {
			json_object_put(memory);
			goto error;
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

		CHECK(dump_counters(
			server->sctx->udpinstats4, isc_statsformat_json,
			udpreq4, NULL, udpinsizestats_xmldesc,
			dns_sizecounter_in_max, udpinsizestats_index,
			udpinsizestat_values, 0));

		CHECK(dump_counters(
			server->sctx->udpoutstats4, isc_statsformat_json,
			udpresp4, NULL, udpoutsizestats_xmldesc,
			dns_sizecounter_out_max, udpoutsizestats_index,
			udpoutsizestat_values, 0));

		CHECK(dump_counters(
			server->sctx->tcpinstats4, isc_statsformat_json,
			tcpreq4, NULL, tcpinsizestats_xmldesc,
			dns_sizecounter_in_max, tcpinsizestats_index,
			tcpinsizestat_values, 0));

		CHECK(dump_counters(
			server->sctx->tcpoutstats4, isc_statsformat_json,
			tcpresp4, NULL, tcpoutsizestats_xmldesc,
			dns_sizecounter_out_max, tcpoutsizestats_index,
			tcpoutsizestat_values, 0));

		CHECK(dump_counters(
			server->sctx->udpinstats6, isc_statsformat_json,
			udpreq6, NULL, udpinsizestats_xmldesc,
			dns_sizecounter_in_max, udpinsizestats_index,
			udpinsizestat_values, 0));

		CHECK(dump_counters(
			server->sctx->udpoutstats6, isc_statsformat_json,
			udpresp6, NULL, udpoutsizestats_xmldesc,
			dns_sizecounter_out_max, udpoutsizestats_index,
			udpoutsizestat_values, 0));

		CHECK(dump_counters(
			server->sctx->tcpinstats6, isc_statsformat_json,
			tcpreq6, NULL, tcpinsizestats_xmldesc,
			dns_sizecounter_in_max, tcpinsizestats_index,
			tcpinsizestat_values, 0));

		CHECK(dump_counters(
			server->sctx->tcpoutstats6, isc_statsformat_json,
			tcpresp6, NULL, tcpoutsizestats_xmldesc,
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

error:
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

	return (result);
}

static isc_result_t
render_json(uint32_t flags, const char *url, isc_httpdurl_t *urlinfo,
	    const char *querystring, const char *headers, void *arg,
	    unsigned int *retcode, const char **retmsg, const char **mimetype,
	    isc_buffer_t *b, isc_httpdfree_t **freecb, void **freecb_args) {
	isc_result_t result;
	json_object *bindstats = NULL;
	named_server_t *server = arg;
	const char *msg = NULL;
	size_t msglen = 0;
	char *p;

	UNUSED(url);
	UNUSED(urlinfo);
	UNUSED(headers);
	UNUSED(querystring);

	result = generatejson(server, &msglen, &msg, &bindstats, flags);
	if (result == ISC_R_SUCCESS) {
		*retcode = 200;
		*retmsg = "OK";
		*mimetype = "application/json";
		DE_CONST(msg, p);
		isc_buffer_reinit(b, p, msglen);
		isc_buffer_add(b, msglen);
		*freecb = wrap_jsonfree;
		*freecb_args = bindstats;
	} else {
		isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
			      NAMED_LOGMODULE_SERVER, ISC_LOG_ERROR,
			      "failed at rendering JSON()");
	}

	return (result);
}

static isc_result_t
render_json_all(const char *url, isc_httpdurl_t *urlinfo,
		const char *querystring, const char *headers, void *arg,
		unsigned int *retcode, const char **retmsg,
		const char **mimetype, isc_buffer_t *b,
		isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_ALL, url, urlinfo, querystring, headers,
			    arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

static isc_result_t
render_json_status(const char *url, isc_httpdurl_t *urlinfo,
		   const char *querystring, const char *headers, void *arg,
		   unsigned int *retcode, const char **retmsg,
		   const char **mimetype, isc_buffer_t *b,
		   isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_STATUS, url, urlinfo, querystring,
			    headers, arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

static isc_result_t
render_json_server(const char *url, isc_httpdurl_t *urlinfo,
		   const char *querystring, const char *headers, void *arg,
		   unsigned int *retcode, const char **retmsg,
		   const char **mimetype, isc_buffer_t *b,
		   isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_SERVER, url, urlinfo, querystring,
			    headers, arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

static isc_result_t
render_json_zones(const char *url, isc_httpdurl_t *urlinfo,
		  const char *querystring, const char *headers, void *arg,
		  unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_ZONES, url, urlinfo, querystring,
			    headers, arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

static isc_result_t
render_json_mem(const char *url, isc_httpdurl_t *urlinfo,
		const char *querystring, const char *headers, void *arg,
		unsigned int *retcode, const char **retmsg,
		const char **mimetype, isc_buffer_t *b,
		isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_MEM, url, urlinfo, querystring, headers,
			    arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

static isc_result_t
render_json_tasks(const char *url, isc_httpdurl_t *urlinfo,
		  const char *querystring, const char *headers, void *arg,
		  unsigned int *retcode, const char **retmsg,
		  const char **mimetype, isc_buffer_t *b,
		  isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_TASKS, url, urlinfo, querystring,
			    headers, arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

static isc_result_t
render_json_net(const char *url, isc_httpdurl_t *urlinfo,
		const char *querystring, const char *headers, void *arg,
		unsigned int *retcode, const char **retmsg,
		const char **mimetype, isc_buffer_t *b,
		isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_NET, url, urlinfo, querystring, headers,
			    arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

static isc_result_t
render_json_traffic(const char *url, isc_httpdurl_t *urlinfo,
		    const char *querystring, const char *headers, void *arg,
		    unsigned int *retcode, const char **retmsg,
		    const char **mimetype, isc_buffer_t *b,
		    isc_httpdfree_t **freecb, void **freecb_args) {
	return (render_json(STATS_JSON_TRAFFIC, url, urlinfo, querystring,
			    headers, arg, retcode, retmsg, mimetype, b, freecb,
			    freecb_args));
}

#endif /* HAVE_JSON_C */

static isc_result_t
render_xsl(const char *url, isc_httpdurl_t *urlinfo, const char *querystring,
	   const char *headers, void *args, unsigned int *retcode,
	   const char **retmsg, const char **mimetype, isc_buffer_t *b,
	   isc_httpdfree_t **freecb, void **freecb_args) {
	isc_result_t result;
	char *_headers = NULL;

	UNUSED(url);
	UNUSED(querystring);
	UNUSED(args);

	*freecb = NULL;
	*freecb_args = NULL;
	*mimetype = "text/xslt+xml";

	if (urlinfo->isstatic) {
		isc_time_t when;
		char *line, *saveptr;
		const char *if_modified_since = "If-Modified-Since: ";
		_headers = strdup(headers);

		if (_headers == NULL) {
			goto send;
		}

		saveptr = NULL;
		for (line = strtok_r(_headers, "\n", &saveptr); line;
		     line = strtok_r(NULL, "\n", &saveptr))
		{
			if (strncasecmp(line, if_modified_since,
					strlen(if_modified_since)) == 0)
			{
				time_t t1, t2;
				line += strlen(if_modified_since);
				result = isc_time_parsehttptimestamp(line,
								     &when);
				if (result != ISC_R_SUCCESS) {
					goto send;
				}

				result = isc_time_secondsastimet(&when, &t1);
				if (result != ISC_R_SUCCESS) {
					goto send;
				}

				result = isc_time_secondsastimet(
					&urlinfo->loadtime, &t2);
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
		}
	}

send:
	*retcode = 200;
	*retmsg = "OK";
	isc_buffer_reinit(b, xslmsg, strlen(xslmsg));
	isc_buffer_add(b, strlen(xslmsg));
end:
	free(_headers);
	return (ISC_R_SUCCESS);
}

static void
shutdown_listener(named_statschannel_t *listener) {
	char socktext[ISC_SOCKADDR_FORMATSIZE];
	isc_sockaddr_format(&listener->address, socktext, sizeof(socktext));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_NOTICE,
		      "stopping statistics channel on %s", socktext);

	isc_httpdmgr_shutdown(&listener->httpdmgr);
}

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
		return (true);
	}
	UNLOCK(&listener->lock);

	isc_sockaddr_format(fromaddr, socktext, sizeof(socktext));
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_WARNING,
		      "rejected statistics connection from %s", socktext);

	return (false);
}

static void
destroy_listener(void *arg) {
	named_statschannel_t *listener = arg;

	REQUIRE(listener != NULL);
	REQUIRE(!ISC_LINK_LINKED(listener, link));

	/* We don't have to acquire the lock here since it's already unlinked */
	dns_acl_detach(&listener->acl);

	isc_mutex_destroy(&listener->lock);
	isc_mem_putanddetach(&listener->mctx, listener, sizeof(*listener));
}

static isc_result_t
add_listener(named_server_t *server, named_statschannel_t **listenerp,
	     const cfg_obj_t *listen_params, const cfg_obj_t *config,
	     isc_sockaddr_t *addr, cfg_aclconfctx_t *aclconfctx,
	     const char *socktext) {
	isc_result_t result;
	named_statschannel_t *listener;
	isc_task_t *task = NULL;
	isc_socket_t *sock = NULL;
	const cfg_obj_t *allow;
	dns_acl_t *new_acl = NULL;

	listener = isc_mem_get(server->mctx, sizeof(*listener));

	listener->httpdmgr = NULL;
	listener->address = *addr;
	listener->acl = NULL;
	listener->mctx = NULL;
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
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	dns_acl_attach(new_acl, &listener->acl);
	dns_acl_detach(&new_acl);

	result = isc_task_create(named_g_taskmgr, 0, &task);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	isc_task_setname(task, "statchannel", NULL);

	result = isc_socket_create(named_g_socketmgr, isc_sockaddr_pf(addr),
				   isc_sockettype_tcp, &sock);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	isc_socket_setname(sock, "statchannel", NULL);

#ifndef ISC_ALLOW_MAPPED
	isc_socket_ipv6only(sock, true);
#endif /* ifndef ISC_ALLOW_MAPPED */

	result = isc_socket_bind(sock, addr, ISC_SOCKET_REUSEADDRESS);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = isc_httpdmgr_create(server->mctx, sock, task, client_ok,
				     destroy_listener, listener,
				     named_g_timermgr, &listener->httpdmgr);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

#ifdef HAVE_LIBXML2
	isc_httpdmgr_addurl(listener->httpdmgr, "/", render_xml_all, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml", render_xml_all, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3", render_xml_all,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3/status",
			    render_xml_status, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3/server",
			    render_xml_server, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3/zones",
			    render_xml_zones, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3/net", render_xml_net,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3/tasks",
			    render_xml_tasks, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3/mem", render_xml_mem,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/xml/v3/traffic",
			    render_xml_traffic, server);
#endif /* ifdef HAVE_LIBXML2 */
#ifdef HAVE_JSON_C
	isc_httpdmgr_addurl(listener->httpdmgr, "/json", render_json_all,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1", render_json_all,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1/status",
			    render_json_status, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1/server",
			    render_json_server, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1/zones",
			    render_json_zones, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1/tasks",
			    render_json_tasks, server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1/net", render_json_net,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1/mem", render_json_mem,
			    server);
	isc_httpdmgr_addurl(listener->httpdmgr, "/json/v1/traffic",
			    render_json_traffic, server);
#endif /* ifdef HAVE_JSON_C */
	isc_httpdmgr_addurl2(listener->httpdmgr, "/bind9.xsl", true, render_xsl,
			     server);

	*listenerp = listener;
	isc_log_write(named_g_lctx, NAMED_LOGCATEGORY_GENERAL,
		      NAMED_LOGMODULE_SERVER, ISC_LOG_NOTICE,
		      "statistics channel listening on %s", socktext);

cleanup:
	if (result != ISC_R_SUCCESS) {
		if (listener->acl != NULL) {
			dns_acl_detach(&listener->acl);
		}
		isc_mutex_destroy(&listener->lock);
		isc_mem_putanddetach(&listener->mctx, listener,
				     sizeof(*listener));
	}
	if (task != NULL) {
		isc_task_detach(&task);
	}
	if (sock != NULL) {
		isc_socket_detach(&sock);
	}

	return (result);
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

	RUNTIME_CHECK(isc_once_do(&once, init_desc) == ISC_R_SUCCESS);

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
	return (ISC_R_SUCCESS);
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
	isc_stdtime_t now;
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

	RUNTIME_CHECK(isc_once_do(&once, init_desc) == ISC_R_SUCCESS);

	/* Set common fields */
	dumparg.type = isc_statsformat_file;
	dumparg.arg = fp;

	isc_stdtime_get(&now);
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
		if (view->resquerystats == NULL) {
			continue;
		}
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s]\n", view->name);
		}
		dns_rdatatypestats_dump(view->resquerystats, rdtypestat_dump,
					&dumparg, 0);
	}

	fprintf(fp, "++ Name Server Statistics ++\n");
	(void)dump_counters(ns_stats_get(server->sctx->nsstats),
			    isc_statsformat_file, fp, NULL, nsstats_desc,
			    ns_statscounter_max, nsstats_index, nsstat_values,
			    0);

	fprintf(fp, "++ Zone Maintenance Statistics ++\n");
	(void)dump_counters(server->zonestats, isc_statsformat_file, fp, NULL,
			    zonestats_desc, dns_zonestatscounter_max,
			    zonestats_index, zonestat_values, 0);

	fprintf(fp, "++ Resolver Statistics ++\n");
	fprintf(fp, "[Common]\n");
	(void)dump_counters(server->resolverstats, isc_statsformat_file, fp,
			    NULL, resstats_desc, dns_resstatscounter_max,
			    resstats_index, resstat_values, 0);
	for (view = ISC_LIST_HEAD(server->viewlist); view != NULL;
	     view = ISC_LIST_NEXT(view, link))
	{
		if (view->resstats == NULL) {
			continue;
		}
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s]\n", view->name);
		}
		(void)dump_counters(view->resstats, isc_statsformat_file, fp,
				    NULL, resstats_desc,
				    dns_resstatscounter_max, resstats_index,
				    resstat_values, 0);
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
		if (view->adbstats == NULL) {
			continue;
		}
		if (strcmp(view->name, "_default") == 0) {
			fprintf(fp, "[View: default]\n");
		} else {
			fprintf(fp, "[View: %s]\n", view->name);
		}
		(void)dump_counters(view->adbstats, isc_statsformat_file, fp,
				    NULL, adbstats_desc, dns_adbstats_max,
				    adbstats_index, adbstat_values, 0);
	}

	fprintf(fp, "++ Socket I/O Statistics ++\n");
	(void)dump_counters(server->sockstats, isc_statsformat_file, fp, NULL,
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

			(void)dump_counters(zonestats, isc_statsformat_file, fp,
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

			(void)dump_counters(
				gluecachestats, isc_statsformat_file, fp, NULL,
				gluecachestats_desc,
				dns_gluecachestatscounter_max,
				gluecachestats_index, gluecachestats_values, 0);
		}
	}

	fprintf(fp, "--- Statistics Dump --- (%lu)\n", (unsigned long)now);

	return (ISC_R_SUCCESS); /* this function currently always succeeds */
}
