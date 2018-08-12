/*	$NetBSD: query.c,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#include <string.h>

#include <isc/hex.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/rwlock.h>
#include <isc/serial.h>
#include <isc/stats.h>
#include <isc/string.h>
#include <isc/thread.h>
#include <isc/util.h>

#include <dns/adb.h>
#include <dns/badcache.h>
#include <dns/byaddr.h>
#include <dns/cache.h>
#include <dns/db.h>
#include <dns/dlz.h>
#include <dns/dns64.h>
#include <dns/dnsrps.h>
#include <dns/dnssec.h>
#include <dns/events.h>
#include <dns/keytable.h>
#include <dns/message.h>
#include <dns/ncache.h>
#include <dns/nsec.h>
#include <dns/nsec3.h>
#include <dns/order.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/stats.h>
#include <dns/tkey.h>
#include <dns/types.h>
#include <dns/view.h>
#include <dns/zone.h>
#include <dns/zt.h>

#include <ns/client.h>
#include <ns/interfacemgr.h>
#include <ns/log.h>
#include <ns/server.h>
#include <ns/sortlist.h>
#include <ns/stats.h>
#include <ns/xfrout.h>

#include "hooks.h"

#if 0
/*
 * It has been recommended that DNS64 be changed to return excluded
 * AAAA addresses if DNS64 synthesis does not occur.  This minimises
 * the impact on the lookup results.  While most DNS AAAA lookups are
 * done to send IP packets to a host, not all of them are and filtering
 * excluded addresses has a negative impact on those uses.
 */
#define dns64_bis_return_excluded_addresses 1
#endif

/*%
 * Maximum number of chained queries before we give up
 * to prevent CNAME loops.
 */
#define MAX_RESTARTS 16

#define QUERY_ERROR(qctx, r) \
do { \
	qctx->result = r; \
	qctx->want_restart = ISC_FALSE; \
	qctx->line = __LINE__; \
} while (0)

#define RECURSE_ERROR(qctx, r) \
do { \
	if ((r) == DNS_R_DUPLICATE || (r) == DNS_R_DROP) \
		QUERY_ERROR(qctx, r); \
	else \
		QUERY_ERROR(qctx, DNS_R_SERVFAIL); \
} while (0)

/*% Partial answer? */
#define PARTIALANSWER(c)	(((c)->query.attributes & \
				  NS_QUERYATTR_PARTIALANSWER) != 0)
/*% Use Cache? */
#define USECACHE(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_CACHEOK) != 0)
/*% Recursion OK? */
#define RECURSIONOK(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_RECURSIONOK) != 0)
/*% Recursing? */
#define RECURSING(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_RECURSING) != 0)
/*% Cache glue ok? */
#define CACHEGLUEOK(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_CACHEGLUEOK) != 0)
/*% Want Recursion? */
#define WANTRECURSION(c)	(((c)->query.attributes & \
				  NS_QUERYATTR_WANTRECURSION) != 0)
/*% Is TCP? */
#define TCP(c)			(((c)->attributes & NS_CLIENTATTR_TCP) != 0)

/*% Want DNSSEC? */
#define WANTDNSSEC(c)		(((c)->attributes & \
				  NS_CLIENTATTR_WANTDNSSEC) != 0)
/*% Want WANTAD? */
#define WANTAD(c)		(((c)->attributes & \
				  NS_CLIENTATTR_WANTAD) != 0)
/*% Client presented a valid COOKIE. */
#define HAVECOOKIE(c)		(((c)->attributes & \
				  NS_CLIENTATTR_HAVECOOKIE) != 0)
/*% Client presented a COOKIE. */
#define WANTCOOKIE(c)		(((c)->attributes & \
				  NS_CLIENTATTR_WANTCOOKIE) != 0)
/*% Client presented a CLIENT-SUBNET option. */
#define HAVEECS(c)		(((c)->attributes & \
				  NS_CLIENTATTR_HAVEECS) != 0)
/*% No authority? */
#define NOAUTHORITY(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_NOAUTHORITY) != 0)
/*% No additional? */
#define NOADDITIONAL(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_NOADDITIONAL) != 0)
/*% Secure? */
#define SECURE(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_SECURE) != 0)
/*% DNS64 A lookup? */
#define DNS64(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_DNS64) != 0)

#define DNS64EXCLUDE(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_DNS64EXCLUDE) != 0)

#define REDIRECT(c)		(((c)->query.attributes & \
				  NS_QUERYATTR_REDIRECT) != 0)

/*% Does the rdataset 'r' have an attached 'No QNAME Proof'? */
#define NOQNAME(r)		(((r)->attributes & \
				  DNS_RDATASETATTR_NOQNAME) != 0)

/*% Does the rdataset 'r' contains a stale answer? */
#define STALE(r)		(((r)->attributes & \
				  DNS_RDATASETATTR_STALE) != 0)

#ifdef WANT_QUERYTRACE
static inline void
client_trace(ns_client_t *client, int level, const char *message) {
	if (client != NULL && client->query.qname != NULL) {
		if (isc_log_wouldlog(ns_lctx, level)) {
			char qbuf[DNS_NAME_FORMATSIZE];
			char tbuf[DNS_RDATATYPE_FORMATSIZE];
			dns_name_format(client->query.qname,
					qbuf, sizeof(qbuf));
			dns_rdatatype_format(client->query.qtype,
					     tbuf, sizeof(tbuf));
			isc_log_write(ns_lctx,
				      NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_QUERY, level,
				      "query client=%p thread=0x%lx "
				      "(%s/%s): %s",
				      client,
				      (unsigned long) isc_thread_self(),
				      qbuf, tbuf, message);
		}
	 } else {
		isc_log_write(ns_lctx,
			      NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_QUERY, level,
			      "query client=%p thread=0x%lx "
			      "(<unknown-query>): %s",
			      client,
			      (unsigned long) isc_thread_self(),
			      message);
	}
}
#define CTRACE(l,m)	  client_trace(client, l, m)
#define CCTRACE(l,m)	  client_trace(qctx->client, l, m)
#else
#define CTRACE(l,m) ((void)m)
#define CCTRACE(l,m) ((void)m)
#endif /* WANT_QUERYTRACE */


#define DNS_GETDB_NOEXACT 0x01U
#define DNS_GETDB_NOLOG 0x02U
#define DNS_GETDB_PARTIAL 0x04U
#define DNS_GETDB_IGNOREACL 0x08U

#define PENDINGOK(x)	(((x) & DNS_DBFIND_PENDINGOK) != 0)

#define SFCACHE_CDFLAG 0x1

/*
 * These have the same semantics as:
 *
 * 	foo_attach(b, a);
 *	foo_detach(&a);
 *
 * without the locking and magic testing.
 *
 * We use SAVE and RESTORE as that shows the operation being performed.
 */
#define SAVE(a, b) do { INSIST(a == NULL); a = b; b = NULL; } while (0)
#define RESTORE(a, b) SAVE(a, b)

static isc_boolean_t
validate(ns_client_t *client, dns_db_t *db, dns_name_t *name,
	 dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset);

static void
query_findclosestnsec3(dns_name_t *qname, dns_db_t *db,
		       dns_dbversion_t *version, ns_client_t *client,
		       dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		       dns_name_t *fname, isc_boolean_t exact,
		       dns_name_t *found);

static inline void
log_queryerror(ns_client_t *client, isc_result_t result, int line, int level);

static void
rpz_st_clear(ns_client_t *client);

static isc_boolean_t
rpz_ck_dnssec(ns_client_t *client, isc_result_t qresult,
	      dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset);

static void
log_noexistnodata(void *val, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

#ifdef NS_HOOKS_ENABLE

LIBNS_EXTERNAL_DATA ns_hook_t *ns__hook_table = NULL;

#define PROCESS_HOOK(...) \
	NS_PROCESS_HOOK(ns__hook_table, __VA_ARGS__)
#define PROCESS_HOOK_VOID(...) \
	NS_PROCESS_HOOK_VOID(ns__hook_table, __VA_ARGS__)

#else

#define PROCESS_HOOK(...)	do {} while (0)
#define PROCESS_HOOK_VOID(...)	do {} while (0)

#endif

/*
 * The functions defined below implement the query logic that previously lived
 * in the single very complex function query_find().  The query_ctx_t structure
 * defined in <ns/query.h> maintains state from function to function.  The call
 * flow for the general query processing algorithm is described below:
 *
 * 1. Set up query context and other resources for a client
 *    query (query_setup())
 *
 * 2. Start the search (ns__query_start())
 *
 * 3. Identify authoritative data sources which may have an answer;
 *    search them (query_lookup()). If an answer is found, go to 7.
 *
 * 4. If recursion or cache access are allowed, search the cache
 *    (query_lookup() again, using the cache database) to find a better
 *    answer. If an answer is found, go to 7.
 *
 * 5. If recursion is allowed, begin recursion (query_recurse()).
 *    Go to 15 to clean up this phase of the query. When recursion
 *    is complete, processing will resume at 6.
 *
 * 6. Resume from recursion; set up query context for resumed processing.
 *
 * 7. Determine what sort of answer we've found (query_gotanswer())
 *    and call other functions accordingly:
 *      - not found (auth or cache), go to 8
 *      - delegation, go to 9
 *      - no such domain (auth), go to 10
 *      - empty answer (auth), go to 11
 *      - negative response (cache), go to 12
 *      - answer found, go to 13
 *
 * 8. The answer was not found in the database (query_notfound().
 *    Set up a referral and go to 9.
 *
 * 9. Handle a delegation response (query_delegation()). If we are
 *    allowed to recurse, go to 5, otherwise go to 15 to clean up and
 *    return the delegation to the client.
 *
 * 10. No such domain (query_nxdomain()). Attempt redirection; if
 *     unsuccessful, add authority section records (query_addsoa(),
 *     query_addauth()), then go to 15 to return NXDOMAIN to client.
 *
 * 11. Empty answer (query_nodata()). Add authority section records
 *     (query_addsoa(), query_addauth()) and signatures if authoritative
 *     (query_sign_nodata()) then go to 15 and return
 *     NOERROR/ANCOUNT=0 to client.
 *
 * 12. No such domain or empty answer returned from cache (query_ncache()).
 *     Set response code appropriately, go to 11.
 *
 * 13. Prepare a response (query_prepresponse()) and then fill it
 *     appropriately (query_respond(), or for type ANY,
 *     query_respond_any()).
 *
 * 14. If a restart is needed due to CNAME/DNAME chaining, go to 2.
 *
 * 15. Clean up resources. If recursing, stop and wait for the event
 *     handler to be called back (step 6).  If an answer is ready,
 *     return it to the client.
 *
 * (XXX: This description omits several special cases including
 * DNS64, filter-aaaa, RPZ, RRL, and the SERVFAIL cache.)
 */

static void
query_trace(query_ctx_t *qctx);

static void
qctx_init(ns_client_t *client, dns_fetchevent_t *event,
	  dns_rdatatype_t qtype, query_ctx_t *qctx);

static isc_result_t
query_setup(ns_client_t *client, dns_rdatatype_t qtype);

static isc_result_t
query_lookup(query_ctx_t *qctx);

static void
fetch_callback(isc_task_t *task, isc_event_t *event);

static void
recparam_update(ns_query_recparam_t *param, dns_rdatatype_t qtype,
		const dns_name_t *qname, const dns_name_t *qdomain);

static isc_result_t
query_recurse(ns_client_t *client, dns_rdatatype_t qtype, dns_name_t *qname,
	      dns_name_t *qdomain, dns_rdataset_t *nameservers,
	      isc_boolean_t resuming);

static isc_result_t
query_resume(query_ctx_t *qctx);

static isc_result_t
query_checkrrl(query_ctx_t *qctx, isc_result_t result);

static isc_result_t
query_checkrpz(query_ctx_t *qctx, isc_result_t result);

static isc_result_t
query_rpzcname(query_ctx_t *qctx, dns_name_t *cname);

static isc_result_t
query_gotanswer(query_ctx_t *qctx, isc_result_t result);

static void
query_addnoqnameproof(query_ctx_t *qctx);

static isc_result_t
query_respond_any(query_ctx_t *qctx);

static isc_result_t
query_respond(query_ctx_t *qctx);

static isc_result_t
query_dns64(query_ctx_t *qctx);

static void
query_filter64(query_ctx_t *qctx);

static isc_result_t
query_notfound(query_ctx_t *qctx);

static isc_result_t
query_zone_delegation(query_ctx_t *qctx);

static isc_result_t
query_delegation(query_ctx_t *qctx);

static void
query_addds(query_ctx_t *qctx);

static isc_result_t
query_nodata(query_ctx_t *qctx, isc_result_t result);

static isc_result_t
query_sign_nodata(query_ctx_t *qctx);

static void
query_addnxrrsetnsec(query_ctx_t *qctx);

static isc_result_t
query_nxdomain(query_ctx_t *qctx, isc_boolean_t empty_wild);

static isc_result_t
query_redirect(query_ctx_t *qctx);

static isc_result_t
query_ncache(query_ctx_t *qctx, isc_result_t result);

static isc_result_t
query_coveringnsec(query_ctx_t *qctx);

static isc_result_t
query_cname(query_ctx_t *qctx);

static isc_result_t
query_dname(query_ctx_t *qctx);

static isc_result_t
query_addcname(query_ctx_t *qctx, dns_trust_t trust, dns_ttl_t ttl);

static isc_result_t
query_prepresponse(query_ctx_t *qctx);

static isc_result_t
query_addsoa(query_ctx_t *qctx, unsigned int override_ttl,
	     dns_section_t section);

static isc_result_t
query_addns(query_ctx_t *qctx);

static void
query_addbestns(query_ctx_t *qctx);

static void
query_addwildcardproof(query_ctx_t *qctx, isc_boolean_t ispositive,
		       isc_boolean_t nodata);

static void
query_addauth(query_ctx_t *qctx);

static isc_result_t
query_done(query_ctx_t *qctx);

/*%
 * Increment query statistics counters.
 */
static inline void
inc_stats(ns_client_t *client, isc_statscounter_t counter) {
	dns_zone_t *zone = client->query.authzone;
	dns_rdatatype_t qtype;
	dns_rdataset_t *rdataset;
	isc_stats_t *zonestats;
	dns_stats_t *querystats = NULL;

	ns_stats_increment(client->sctx->nsstats, counter);

	if (zone == NULL)
		return;

	/* Do regular response type stats */
	zonestats = dns_zone_getrequeststats(zone);

	if (zonestats != NULL)
		isc_stats_increment(zonestats, counter);

	/* Do query type statistics
	 *
	 * We only increment per-type if we're using the authoritative
	 * answer counter, preventing double-counting.
	 */
	if (counter == ns_statscounter_authans) {
		querystats = dns_zone_getrcvquerystats(zone);
		if (querystats != NULL) {
			rdataset = ISC_LIST_HEAD(client->query.qname->list);
			if (rdataset != NULL) {
				qtype = rdataset->type;
				dns_rdatatypestats_increment(querystats, qtype);
			}
		}
	}
}

static void
query_send(ns_client_t *client) {
	isc_statscounter_t counter;

	if ((client->message->flags & DNS_MESSAGEFLAG_AA) == 0)
		inc_stats(client, ns_statscounter_nonauthans);
	else
		inc_stats(client, ns_statscounter_authans);

	if (client->message->rcode == dns_rcode_noerror) {
		dns_section_t answer = DNS_SECTION_ANSWER;
		if (ISC_LIST_EMPTY(client->message->sections[answer])) {
			if (client->query.isreferral)
				counter = ns_statscounter_referral;
			else
				counter = ns_statscounter_nxrrset;
		} else
			counter = ns_statscounter_success;
	} else if (client->message->rcode == dns_rcode_nxdomain)
		counter = ns_statscounter_nxdomain;
	else if (client->message->rcode == dns_rcode_badcookie)
		counter = ns_statscounter_badcookie;
	else /* We end up here in case of YXDOMAIN, and maybe others */
		counter = ns_statscounter_failure;

	inc_stats(client, counter);
	ns_client_send(client);
}

static void
query_error(ns_client_t *client, isc_result_t result, int line) {
	int loglevel = ISC_LOG_DEBUG(3);

	switch (result) {
	case DNS_R_SERVFAIL:
		loglevel = ISC_LOG_DEBUG(1);
		inc_stats(client, ns_statscounter_servfail);
		break;
	case DNS_R_FORMERR:
		inc_stats(client, ns_statscounter_formerr);
		break;
	default:
		inc_stats(client, ns_statscounter_failure);
		break;
	}

	if ((client->sctx->options & NS_SERVER_LOGQUERIES) != 0)
		loglevel = ISC_LOG_INFO;

	log_queryerror(client, result, line, loglevel);

	ns_client_error(client, result);
}

static void
query_next(ns_client_t *client, isc_result_t result) {
	if (result == DNS_R_DUPLICATE)
		inc_stats(client, ns_statscounter_duplicate);
	else if (result == DNS_R_DROP)
		inc_stats(client, ns_statscounter_dropped);
	else
		inc_stats(client, ns_statscounter_failure);
	ns_client_next(client, result);
}

static inline void
query_freefreeversions(ns_client_t *client, isc_boolean_t everything) {
	ns_dbversion_t *dbversion, *dbversion_next;
	unsigned int i;

	for (dbversion = ISC_LIST_HEAD(client->query.freeversions), i = 0;
	     dbversion != NULL;
	     dbversion = dbversion_next, i++)
	{
		dbversion_next = ISC_LIST_NEXT(dbversion, link);
		/*
		 * If we're not freeing everything, we keep the first three
		 * dbversions structures around.
		 */
		if (i > 3 || everything) {
			ISC_LIST_UNLINK(client->query.freeversions, dbversion,
					link);
			isc_mem_put(client->mctx, dbversion,
				    sizeof(*dbversion));
		}
	}
}

void
ns_query_cancel(ns_client_t *client) {
	REQUIRE(NS_CLIENT_VALID(client));

	LOCK(&client->query.fetchlock);
	if (client->query.fetch != NULL) {
		dns_resolver_cancelfetch(client->query.fetch);

		client->query.fetch = NULL;
	}
	UNLOCK(&client->query.fetchlock);
}

static inline void
query_putrdataset(ns_client_t *client, dns_rdataset_t **rdatasetp) {
	dns_rdataset_t *rdataset = *rdatasetp;

	CTRACE(ISC_LOG_DEBUG(3), "query_putrdataset");
	if (rdataset != NULL) {
		if (dns_rdataset_isassociated(rdataset))
			dns_rdataset_disassociate(rdataset);
		dns_message_puttemprdataset(client->message, rdatasetp);
	}
	CTRACE(ISC_LOG_DEBUG(3), "query_putrdataset: done");
}

static inline void
query_reset(ns_client_t *client, isc_boolean_t everything) {
	isc_buffer_t *dbuf, *dbuf_next;
	ns_dbversion_t *dbversion, *dbversion_next;

	CTRACE(ISC_LOG_DEBUG(3), "query_reset");

	/*%
	 * Reset the query state of a client to its default state.
	 */

	/*
	 * Cancel the fetch if it's running.
	 */
	ns_query_cancel(client);

	/*
	 * Cleanup any active versions.
	 */
	for (dbversion = ISC_LIST_HEAD(client->query.activeversions);
	     dbversion != NULL;
	     dbversion = dbversion_next) {
		dbversion_next = ISC_LIST_NEXT(dbversion, link);
		dns_db_closeversion(dbversion->db, &dbversion->version,
				    ISC_FALSE);
		dns_db_detach(&dbversion->db);
		ISC_LIST_INITANDAPPEND(client->query.freeversions,
				      dbversion, link);
	}
	ISC_LIST_INIT(client->query.activeversions);

	if (client->query.authdb != NULL)
		dns_db_detach(&client->query.authdb);
	if (client->query.authzone != NULL)
		dns_zone_detach(&client->query.authzone);

	if (client->query.dns64_aaaa != NULL)
		query_putrdataset(client, &client->query.dns64_aaaa);
	if (client->query.dns64_sigaaaa != NULL)
		query_putrdataset(client, &client->query.dns64_sigaaaa);
	if (client->query.dns64_aaaaok != NULL) {
		isc_mem_put(client->mctx, client->query.dns64_aaaaok,
			    client->query.dns64_aaaaoklen *
			    sizeof(isc_boolean_t));
		client->query.dns64_aaaaok =  NULL;
		client->query.dns64_aaaaoklen =  0;
	}

	query_putrdataset(client, &client->query.redirect.rdataset);
	query_putrdataset(client, &client->query.redirect.sigrdataset);
	if (client->query.redirect.db != NULL) {
		if (client->query.redirect.node != NULL)
			dns_db_detachnode(client->query.redirect.db,
					  &client->query.redirect.node);
		dns_db_detach(&client->query.redirect.db);
	}
	if (client->query.redirect.zone != NULL)
		dns_zone_detach(&client->query.redirect.zone);

	query_freefreeversions(client, everything);

	for (dbuf = ISC_LIST_HEAD(client->query.namebufs);
	     dbuf != NULL;
	     dbuf = dbuf_next) {
		dbuf_next = ISC_LIST_NEXT(dbuf, link);
		if (dbuf_next != NULL || everything) {
			ISC_LIST_UNLINK(client->query.namebufs, dbuf, link);
			isc_buffer_free(&dbuf);
		}
	}

	if (client->query.restarts > 0) {
		/*
		 * client->query.qname was dynamically allocated.
		 */
		dns_message_puttempname(client->message,
					&client->query.qname);
	}
	client->query.qname = NULL;
	client->query.attributes = (NS_QUERYATTR_RECURSIONOK |
				    NS_QUERYATTR_CACHEOK |
				    NS_QUERYATTR_SECURE);
	client->query.restarts = 0;
	client->query.timerset = ISC_FALSE;
	if (client->query.rpz_st != NULL) {
		rpz_st_clear(client);
		if (everything) {
			INSIST(client->query.rpz_st->rpsdb == NULL);
			isc_mem_put(client->mctx, client->query.rpz_st,
				    sizeof(*client->query.rpz_st));
			client->query.rpz_st = NULL;
		}
	}
	client->query.origqname = NULL;
	client->query.dboptions = 0;
	client->query.fetchoptions = 0;
	client->query.gluedb = NULL;
	client->query.authdbset = ISC_FALSE;
	client->query.isreferral = ISC_FALSE;
	client->query.dns64_options = 0;
	client->query.dns64_ttl = ISC_UINT32_MAX;
	client->query.root_key_sentinel_keyid = 0;
	client->query.root_key_sentinel_is_ta = ISC_FALSE;
	client->query.root_key_sentinel_not_ta = ISC_FALSE;
	recparam_update(&client->query.recparam, 0, NULL, NULL);
}

static void
query_next_callback(ns_client_t *client) {
	query_reset(client, ISC_FALSE);
}

void
ns_query_free(ns_client_t *client) {
	REQUIRE(NS_CLIENT_VALID(client));

	query_reset(client, ISC_TRUE);
}

/*%
 * Allocate a name buffer.
 */
static inline isc_result_t
query_newnamebuf(ns_client_t *client) {
	isc_buffer_t *dbuf;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "query_newnamebuf");

	dbuf = NULL;
	result = isc_buffer_allocate(client->mctx, &dbuf, 1024);
	if (result != ISC_R_SUCCESS) {
		CTRACE(ISC_LOG_DEBUG(3),
		       "query_newnamebuf: isc_buffer_allocate failed: done");
		return (result);
	}
	ISC_LIST_APPEND(client->query.namebufs, dbuf, link);

	CTRACE(ISC_LOG_DEBUG(3), "query_newnamebuf: done");
	return (ISC_R_SUCCESS);
}

/*%
 * Get a name buffer from the pool, or allocate a new one if needed.
 */
static inline isc_buffer_t *
query_getnamebuf(ns_client_t *client) {
	isc_buffer_t *dbuf;
	isc_result_t result;
	isc_region_t r;

	CTRACE(ISC_LOG_DEBUG(3), "query_getnamebuf");
	/*%
	 * Return a name buffer with space for a maximal name, allocating
	 * a new one if necessary.
	 */

	if (ISC_LIST_EMPTY(client->query.namebufs)) {
		result = query_newnamebuf(client);
		if (result != ISC_R_SUCCESS) {
		    CTRACE(ISC_LOG_DEBUG(3),
			   "query_getnamebuf: query_newnamebuf failed: done");
			return (NULL);
		}
	}

	dbuf = ISC_LIST_TAIL(client->query.namebufs);
	INSIST(dbuf != NULL);
	isc_buffer_availableregion(dbuf, &r);
	if (r.length < DNS_NAME_MAXWIRE) {
		result = query_newnamebuf(client);
		if (result != ISC_R_SUCCESS) {
		    CTRACE(ISC_LOG_DEBUG(3),
			   "query_getnamebuf: query_newnamebuf failed: done");
			return (NULL);

		}
		dbuf = ISC_LIST_TAIL(client->query.namebufs);
		isc_buffer_availableregion(dbuf, &r);
		INSIST(r.length >= 255);
	}
	CTRACE(ISC_LOG_DEBUG(3), "query_getnamebuf: done");
	return (dbuf);
}

static inline void
query_keepname(ns_client_t *client, dns_name_t *name, isc_buffer_t *dbuf) {
	isc_region_t r;

	CTRACE(ISC_LOG_DEBUG(3), "query_keepname");
	/*%
	 * 'name' is using space in 'dbuf', but 'dbuf' has not yet been
	 * adjusted to take account of that.  We do the adjustment.
	 */

	REQUIRE((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED) != 0);

	dns_name_toregion(name, &r);
	isc_buffer_add(dbuf, r.length);
	dns_name_setbuffer(name, NULL);
	client->query.attributes &= ~NS_QUERYATTR_NAMEBUFUSED;
}

static inline void
query_releasename(ns_client_t *client, dns_name_t **namep) {
	dns_name_t *name = *namep;

	/*%
	 * 'name' is no longer needed.  Return it to our pool of temporary
	 * names.  If it is using a name buffer, relinquish its exclusive
	 * rights on the buffer.
	 */

	CTRACE(ISC_LOG_DEBUG(3), "query_releasename");
	if (dns_name_hasbuffer(name)) {
		INSIST((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED)
		       != 0);
		client->query.attributes &= ~NS_QUERYATTR_NAMEBUFUSED;
	}
	dns_message_puttempname(client->message, namep);
	CTRACE(ISC_LOG_DEBUG(3), "query_releasename: done");
}

static inline dns_name_t *
query_newname(ns_client_t *client, isc_buffer_t *dbuf, isc_buffer_t *nbuf) {
	dns_name_t *name;
	isc_region_t r;
	isc_result_t result;

	REQUIRE((client->query.attributes & NS_QUERYATTR_NAMEBUFUSED) == 0);

	CTRACE(ISC_LOG_DEBUG(3), "query_newname");
	name = NULL;
	result = dns_message_gettempname(client->message, &name);
	if (result != ISC_R_SUCCESS) {
		CTRACE(ISC_LOG_DEBUG(3),
		       "query_newname: dns_message_gettempname failed: done");
		return (NULL);
	}
	isc_buffer_availableregion(dbuf, &r);
	isc_buffer_init(nbuf, r.base, r.length);
	dns_name_init(name, NULL);
	dns_name_setbuffer(name, nbuf);
	client->query.attributes |= NS_QUERYATTR_NAMEBUFUSED;

	CTRACE(ISC_LOG_DEBUG(3), "query_newname: done");
	return (name);
}

static inline dns_rdataset_t *
query_newrdataset(ns_client_t *client) {
	dns_rdataset_t *rdataset;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "query_newrdataset");
	rdataset = NULL;
	result = dns_message_gettemprdataset(client->message, &rdataset);
	if (result != ISC_R_SUCCESS) {
		CTRACE(ISC_LOG_DEBUG(3), "query_newrdataset: "
		       "dns_message_gettemprdataset failed: done");
		return (NULL);
	}

	CTRACE(ISC_LOG_DEBUG(3), "query_newrdataset: done");
	return (rdataset);
}

static inline isc_result_t
query_newdbversion(ns_client_t *client, unsigned int n) {
	unsigned int i;
	ns_dbversion_t *dbversion;

	for (i = 0; i < n; i++) {
		dbversion = isc_mem_get(client->mctx, sizeof(*dbversion));
		if (dbversion != NULL) {
			dbversion->db = NULL;
			dbversion->version = NULL;
			ISC_LIST_INITANDAPPEND(client->query.freeversions,
					      dbversion, link);
		} else {
			/*
			 * We only return ISC_R_NOMEMORY if we couldn't
			 * allocate anything.
			 */
			if (i == 0)
				return (ISC_R_NOMEMORY);
			else
				return (ISC_R_SUCCESS);
		}
	}

	return (ISC_R_SUCCESS);
}

static inline ns_dbversion_t *
query_getdbversion(ns_client_t *client) {
	isc_result_t result;
	ns_dbversion_t *dbversion;

	if (ISC_LIST_EMPTY(client->query.freeversions)) {
		result = query_newdbversion(client, 1);
		if (result != ISC_R_SUCCESS)
			return (NULL);
	}
	dbversion = ISC_LIST_HEAD(client->query.freeversions);
	INSIST(dbversion != NULL);
	ISC_LIST_UNLINK(client->query.freeversions, dbversion, link);

	return (dbversion);
}

isc_result_t
ns_query_init(ns_client_t *client) {
	isc_result_t result;

	REQUIRE(NS_CLIENT_VALID(client));

	ISC_LIST_INIT(client->query.namebufs);
	ISC_LIST_INIT(client->query.activeversions);
	ISC_LIST_INIT(client->query.freeversions);
	client->query.restarts = 0;
	client->query.timerset = ISC_FALSE;
	client->query.rpz_st = NULL;
	client->query.qname = NULL;
	/*
	 * This mutex is destroyed when the client is destroyed in
	 * exit_check().
	 */
	result = isc_mutex_init(&client->query.fetchlock);
	if (result != ISC_R_SUCCESS)
		return (result);
	client->query.fetch = NULL;
	client->query.prefetch = NULL;
	client->query.authdb = NULL;
	client->query.authzone = NULL;
	client->query.authdbset = ISC_FALSE;
	client->query.isreferral = ISC_FALSE;
	client->query.dns64_aaaa = NULL;
	client->query.dns64_sigaaaa = NULL;
	client->query.dns64_aaaaok = NULL;
	client->query.dns64_aaaaoklen = 0;
	client->query.redirect.db = NULL;
	client->query.redirect.node = NULL;
	client->query.redirect.zone = NULL;
	client->query.redirect.qtype = dns_rdatatype_none;
	client->query.redirect.result = ISC_R_SUCCESS;
	client->query.redirect.rdataset = NULL;
	client->query.redirect.sigrdataset = NULL;
	client->query.redirect.authoritative = ISC_FALSE;
	client->query.redirect.is_zone = ISC_FALSE;
	client->query.redirect.fname =
		dns_fixedname_initname(&client->query.redirect.fixed);
	query_reset(client, ISC_FALSE);
	result = query_newdbversion(client, 3);
	if (result != ISC_R_SUCCESS) {
		DESTROYLOCK(&client->query.fetchlock);
		return (result);
	}
	result = query_newnamebuf(client);
	if (result != ISC_R_SUCCESS) {
		query_freefreeversions(client, ISC_TRUE);
		DESTROYLOCK(&client->query.fetchlock);
	}

	return (result);
}

static ns_dbversion_t *
query_findversion(ns_client_t *client, dns_db_t *db) {
	ns_dbversion_t *dbversion;

	/*%
	 * We may already have done a query related to this
	 * database.  If so, we must be sure to make subsequent
	 * queries from the same version.
	 */
	for (dbversion = ISC_LIST_HEAD(client->query.activeversions);
	     dbversion != NULL;
	     dbversion = ISC_LIST_NEXT(dbversion, link)) {
		if (dbversion->db == db)
			break;
	}

	if (dbversion == NULL) {
		/*
		 * This is a new zone for this query.  Add it to
		 * the active list.
		 */
		dbversion = query_getdbversion(client);
		if (dbversion == NULL)
			return (NULL);
		dns_db_attach(db, &dbversion->db);
		dns_db_currentversion(db, &dbversion->version);
		dbversion->acl_checked = ISC_FALSE;
		dbversion->queryok = ISC_FALSE;
		ISC_LIST_APPEND(client->query.activeversions,
				dbversion, link);
	}

	return (dbversion);
}

static inline isc_result_t
query_validatezonedb(ns_client_t *client, const dns_name_t *name,
		     dns_rdatatype_t qtype, unsigned int options,
		     dns_zone_t *zone, dns_db_t *db,
		     dns_dbversion_t **versionp)
{
	isc_result_t result;
	dns_acl_t *queryacl, *queryonacl;
	ns_dbversion_t *dbversion;

	REQUIRE(zone != NULL);
	REQUIRE(db != NULL);

	/*
	 * This limits our searching to the zone where the first name
	 * (the query target) was looked for.  This prevents following
	 * CNAMES or DNAMES into other zones and prevents returning
	 * additional data from other zones. This does not apply if we're
	 * answering a query where recursion is requested and allowed.
	 */
	if (client->query.rpz_st == NULL &&
	    !(WANTRECURSION(client) && RECURSIONOK(client)) &&
	    client->query.authdbset && db != client->query.authdb)
	{
		return (DNS_R_REFUSED);
	}

	/*
	 * Non recursive query to a static-stub zone is prohibited; its
	 * zone content is not public data, but a part of local configuration
	 * and should not be disclosed.
	 */
	if (dns_zone_gettype(zone) == dns_zone_staticstub &&
	    !RECURSIONOK(client)) {
		return (DNS_R_REFUSED);
	}

	/*
	 * If the zone has an ACL, we'll check it, otherwise
	 * we use the view's "allow-query" ACL.  Each ACL is only checked
	 * once per query.
	 *
	 * Also, get the database version to use.
	 */

	/*
	 * Get the current version of this database.
	 */
	dbversion = query_findversion(client, db);
	if (dbversion == NULL) {
		CTRACE(ISC_LOG_ERROR, "unable to get db version");
		return (DNS_R_SERVFAIL);
	}

	if ((options & DNS_GETDB_IGNOREACL) != 0)
		goto approved;
	if (dbversion->acl_checked) {
		if (!dbversion->queryok)
			return (DNS_R_REFUSED);
		goto approved;
	}

	queryacl = dns_zone_getqueryacl(zone);
	if (queryacl == NULL) {
		queryacl = client->view->queryacl;
		if ((client->query.attributes &
		     NS_QUERYATTR_QUERYOKVALID) != 0) {
			/*
			 * We've evaluated the view's queryacl already.  If
			 * NS_QUERYATTR_QUERYOK is set, then the client is
			 * allowed to make queries, otherwise the query should
			 * be refused.
			 */
			dbversion->acl_checked = ISC_TRUE;
			if ((client->query.attributes &
			     NS_QUERYATTR_QUERYOK) == 0) {
				dbversion->queryok = ISC_FALSE;
				return (DNS_R_REFUSED);
			}
			dbversion->queryok = ISC_TRUE;
			goto approved;
		}
	}

	result = ns_client_checkaclsilent(client, NULL, queryacl, ISC_TRUE);
	if ((options & DNS_GETDB_NOLOG) == 0) {
		char msg[NS_CLIENT_ACLMSGSIZE("query")];
		if (result == ISC_R_SUCCESS) {
			if (isc_log_wouldlog(ns_lctx, ISC_LOG_DEBUG(3))) {
				ns_client_aclmsg("query", name, qtype,
						 client->view->rdclass,
						 msg, sizeof(msg));
				ns_client_log(client,
					      DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_DEBUG(3),
					      "%s approved", msg);
			}
		} else {
			ns_client_aclmsg("query", name, qtype,
					 client->view->rdclass,
					 msg, sizeof(msg));
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
				      "%s denied", msg);
		}
	}

	if (queryacl == client->view->queryacl) {
		if (result == ISC_R_SUCCESS) {
			/*
			 * We were allowed by the default
			 * "allow-query" ACL.  Remember this so we
			 * don't have to check again.
			 */
			client->query.attributes |= NS_QUERYATTR_QUERYOK;
		}
		/*
		 * We've now evaluated the view's query ACL, and
		 * the NS_QUERYATTR_QUERYOK attribute is now valid.
		 */
		client->query.attributes |= NS_QUERYATTR_QUERYOKVALID;
	}

	/* If and only if we've gotten this far, check allow-query-on too */
	if (result == ISC_R_SUCCESS) {
		queryonacl = dns_zone_getqueryonacl(zone);
		if (queryonacl == NULL)
			queryonacl = client->view->queryonacl;

		result = ns_client_checkaclsilent(client, &client->destaddr,
						  queryonacl, ISC_TRUE);
		if ((options & DNS_GETDB_NOLOG) == 0 &&
		    result != ISC_R_SUCCESS)
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
				      "query-on denied");
	}

	dbversion->acl_checked = ISC_TRUE;
	if (result != ISC_R_SUCCESS) {
		dbversion->queryok = ISC_FALSE;
		return (DNS_R_REFUSED);
	}
	dbversion->queryok = ISC_TRUE;

 approved:
	/* Transfer ownership, if necessary. */
	if (versionp != NULL)
		*versionp = dbversion->version;
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
query_getzonedb(ns_client_t *client, const dns_name_t *name,
		dns_rdatatype_t qtype, unsigned int options,
		dns_zone_t **zonep, dns_db_t **dbp, dns_dbversion_t **versionp)
{
	isc_result_t result;
	unsigned int ztoptions;
	dns_zone_t *zone = NULL;
	dns_db_t *db = NULL;
	isc_boolean_t partial = ISC_FALSE;

	REQUIRE(zonep != NULL && *zonep == NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);

	/*%
	 * Find a zone database to answer the query.
	 */
	ztoptions = ((options & DNS_GETDB_NOEXACT) != 0) ?
		DNS_ZTFIND_NOEXACT : 0;

	result = dns_zt_find(client->view->zonetable, name, ztoptions, NULL,
			     &zone);

	if (result == DNS_R_PARTIALMATCH)
		partial = ISC_TRUE;
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH)
		result = dns_zone_getdb(zone, &db);

	if (result != ISC_R_SUCCESS)
		goto fail;

	result = query_validatezonedb(client, name, qtype, options, zone, db,
				      versionp);

	if (result != ISC_R_SUCCESS)
		goto fail;

	/* Transfer ownership. */
	*zonep = zone;
	*dbp = db;

	if (partial && (options & DNS_GETDB_PARTIAL) != 0)
		return (DNS_R_PARTIALMATCH);
	return (ISC_R_SUCCESS);

 fail:
	if (zone != NULL)
		dns_zone_detach(&zone);
	if (db != NULL)
		dns_db_detach(&db);

	return (result);
}

static void
rpz_log_rewrite(ns_client_t *client, isc_boolean_t disabled,
		dns_rpz_policy_t policy, dns_rpz_type_t type,
		dns_zone_t *p_zone, dns_name_t *p_name,
		dns_name_t *cname, dns_rpz_num_t rpz_num)
{
	isc_stats_t *zonestats;
	char qname_buf[DNS_NAME_FORMATSIZE];
	char p_name_buf[DNS_NAME_FORMATSIZE];
	char cname_buf[DNS_NAME_FORMATSIZE] = { 0 };
	const char *s1 = cname_buf, *s2 = cname_buf;
	dns_rpz_st_t *st;

	/*
	 * Count enabled rewrites in the global counter.
	 * Count both enabled and disabled rewrites for each zone.
	 */
	if (!disabled && policy != DNS_RPZ_POLICY_PASSTHRU) {
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_rpz_rewrites);
	}
	if (p_zone != NULL) {
		zonestats = dns_zone_getrequeststats(p_zone);
		if (zonestats != NULL)
			isc_stats_increment(zonestats,
					    ns_statscounter_rpz_rewrites);
	}

	if (!isc_log_wouldlog(ns_lctx, DNS_RPZ_INFO_LEVEL))
		return;

	st = client->query.rpz_st;
	if ((st->popt.no_log & DNS_RPZ_ZBIT(rpz_num)) != 0)
		return;

	dns_name_format(client->query.qname, qname_buf, sizeof(qname_buf));
	dns_name_format(p_name, p_name_buf, sizeof(p_name_buf));
	if (cname != NULL) {
		s1 = " (CNAME to: ";
		dns_name_format(cname, cname_buf, sizeof(cname_buf));
		s2 = ")";
	}

	ns_client_log(client, DNS_LOGCATEGORY_RPZ, NS_LOGMODULE_QUERY,
		      DNS_RPZ_INFO_LEVEL, "%srpz %s %s rewrite %s via %s%s%s%s",
		      disabled ? "disabled " : "",
		      dns_rpz_type2str(type), dns_rpz_policy2str(policy),
		      qname_buf, p_name_buf, s1, cname_buf, s2);
}

static void
rpz_log_fail_helper(ns_client_t *client, int level, dns_name_t *p_name,
		    dns_rpz_type_t rpz_type1, dns_rpz_type_t rpz_type2,
		    const char *str, isc_result_t result)
{
	char qnamebuf[DNS_NAME_FORMATSIZE];
	char p_namebuf[DNS_NAME_FORMATSIZE];
	const char *failed, *via, *slash, *str_blank;
	const char *rpztypestr1;
	const char *rpztypestr2;

	if (!isc_log_wouldlog(ns_lctx, level))
		return;

	/*
	 * bin/tests/system/rpz/tests.sh looks for "rpz.*failed" for problems.
	 */
	if (level <= DNS_RPZ_DEBUG_LEVEL1)
		failed = "failed: ";
	else
		failed = ": ";

	rpztypestr1 = dns_rpz_type2str(rpz_type1);
	if (rpz_type2 != DNS_RPZ_TYPE_BAD) {
		slash = "/";
		rpztypestr2 = dns_rpz_type2str(rpz_type2);
	} else {
		slash = "";
		rpztypestr2 = "";
	}

	str_blank = (*str != ' ' && *str != '\0') ? " " : "";

	dns_name_format(client->query.qname, qnamebuf, sizeof(qnamebuf));

	if (p_name != NULL) {
		via = " via ";
		dns_name_format(p_name, p_namebuf, sizeof(p_namebuf));
	} else {
		via = "";
		p_namebuf[0] = '\0';
	}

	ns_client_log(client, NS_LOGCATEGORY_QUERY_ERRORS,
		      NS_LOGMODULE_QUERY, level,
		      "rpz %s%s%s rewrite %s%s%s%s%s%s : %s",
		      rpztypestr1, slash, rpztypestr2,
		      qnamebuf, via, p_namebuf, str_blank,
		      str, failed, isc_result_totext(result));
}

static void
rpz_log_fail(ns_client_t *client, int level, dns_name_t *p_name,
	     dns_rpz_type_t rpz_type, const char *str, isc_result_t result)
{
	rpz_log_fail_helper(client, level, p_name,
			    rpz_type, DNS_RPZ_TYPE_BAD, str, result);
}

/*
 * Get a policy rewrite zone database.
 */
static isc_result_t
rpz_getdb(ns_client_t *client, dns_name_t *p_name, dns_rpz_type_t rpz_type,
	  dns_zone_t **zonep, dns_db_t **dbp, dns_dbversion_t **versionp)
{
	char qnamebuf[DNS_NAME_FORMATSIZE];
	char p_namebuf[DNS_NAME_FORMATSIZE];
	dns_dbversion_t *rpz_version = NULL;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_getdb");

	result = query_getzonedb(client, p_name, dns_rdatatype_any,
				 DNS_GETDB_IGNOREACL, zonep, dbp, &rpz_version);
	if (result == ISC_R_SUCCESS) {
		dns_rpz_st_t *st = client->query.rpz_st;

		/*
		 * It isn't meaningful to log this message when
		 * logging is disabled for some policy zones.
		 */
		if (st->popt.no_log == 0 &&
		    isc_log_wouldlog(ns_lctx, DNS_RPZ_DEBUG_LEVEL2))
		{
			dns_name_format(client->query.qname, qnamebuf,
					sizeof(qnamebuf));
			dns_name_format(p_name, p_namebuf, sizeof(p_namebuf));
			ns_client_log(client, DNS_LOGCATEGORY_RPZ,
				      NS_LOGMODULE_QUERY, DNS_RPZ_DEBUG_LEVEL2,
				      "try rpz %s rewrite %s via %s",
				      dns_rpz_type2str(rpz_type),
				      qnamebuf, p_namebuf);
		}
		*versionp = rpz_version;
		return (ISC_R_SUCCESS);
	}
	rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, p_name, rpz_type,
		     " query_getzonedb()", result);
	return (result);
}

static inline isc_result_t
query_getcachedb(ns_client_t *client, const dns_name_t *name,
		 dns_rdatatype_t qtype, dns_db_t **dbp, unsigned int options)
{
	isc_result_t result;
	isc_boolean_t check_acl;
	dns_db_t *db = NULL;

	REQUIRE(dbp != NULL && *dbp == NULL);

	/*%
	 * Find a cache database to answer the query.
	 * This may fail with DNS_R_REFUSED if the client
	 * is not allowed to use the cache.
	 */

	if (!USECACHE(client))
		return (DNS_R_REFUSED);
	dns_db_attach(client->view->cachedb, &db);

	if ((client->query.attributes & NS_QUERYATTR_CACHEACLOKVALID) != 0) {
		/*
		 * We've evaluated the view's cacheacl already.  If
		 * NS_QUERYATTR_CACHEACLOK is set, then the client is
		 * allowed to make queries, otherwise the query should
		 * be refused.
		 */
		check_acl = ISC_FALSE;
		if ((client->query.attributes & NS_QUERYATTR_CACHEACLOK) == 0)
			goto refuse;
	} else {
		/*
		 * We haven't evaluated the view's queryacl yet.
		 */
		check_acl = ISC_TRUE;
	}

	if (check_acl) {
		isc_boolean_t log = ISC_TF((options & DNS_GETDB_NOLOG) == 0);
		char msg[NS_CLIENT_ACLMSGSIZE("query (cache)")];

		result = ns_client_checkaclsilent(client, NULL,
						  client->view->cacheacl,
						  ISC_TRUE);
		if (result == ISC_R_SUCCESS) {
			/*
			 * We were allowed by the "allow-query-cache" ACL.
			 * Remember this so we don't have to check again.
			 */
			client->query.attributes |=
				NS_QUERYATTR_CACHEACLOK;
			if (log && isc_log_wouldlog(ns_lctx,
						     ISC_LOG_DEBUG(3)))
			{
				ns_client_aclmsg("query (cache)", name, qtype,
						 client->view->rdclass,
						 msg, sizeof(msg));
				ns_client_log(client,
					      DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_DEBUG(3),
					      "%s approved", msg);
			}
		} else if (log) {
			ns_client_aclmsg("query (cache)", name, qtype,
					 client->view->rdclass, msg,
					 sizeof(msg));
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
				      "%s denied", msg);
		}
		/*
		 * We've now evaluated the view's query ACL, and
		 * the NS_QUERYATTR_CACHEACLOKVALID attribute is now valid.
		 */
		client->query.attributes |= NS_QUERYATTR_CACHEACLOKVALID;

		if (result != ISC_R_SUCCESS)
			goto refuse;
	}

	/* Approved. */

	/* Transfer ownership. */
	*dbp = db;

	return (ISC_R_SUCCESS);

 refuse:
	result = DNS_R_REFUSED;

	if (db != NULL)
		dns_db_detach(&db);

	return (result);
}

static inline isc_result_t
query_getdb(ns_client_t *client, dns_name_t *name, dns_rdatatype_t qtype,
	    unsigned int options, dns_zone_t **zonep, dns_db_t **dbp,
	    dns_dbversion_t **versionp, isc_boolean_t *is_zonep)
{
	isc_result_t result;

	isc_result_t tresult;
	unsigned int namelabels;
	unsigned int zonelabels;
	dns_zone_t *zone = NULL;

	REQUIRE(zonep != NULL && *zonep == NULL);

	/* Calculate how many labels are in name. */
	namelabels = dns_name_countlabels(name);
	zonelabels = 0;

	/* Try to find name in bind's standard database. */
	result = query_getzonedb(client, name, qtype, options, &zone,
				 dbp, versionp);

	/* See how many labels are in the zone's name.	  */
	if (result == ISC_R_SUCCESS && zone != NULL)
		zonelabels = dns_name_countlabels(dns_zone_getorigin(zone));

	/*
	 * If # zone labels < # name labels, try to find an even better match
	 * Only try if DLZ drivers are loaded for this view
	 */
	if (ISC_UNLIKELY(zonelabels < namelabels &&
			 !ISC_LIST_EMPTY(client->view->dlz_searched)))
	{
		dns_clientinfomethods_t cm;
		dns_clientinfo_t ci;
		dns_db_t *tdbp;

		dns_clientinfomethods_init(&cm, ns_client_sourceip);
		dns_clientinfo_init(&ci, client, NULL);

		tdbp = NULL;
		tresult = dns_view_searchdlz(client->view, name,
					     zonelabels, &cm, &ci, &tdbp);
		 /* If we successful, we found a better match. */
		if (tresult == ISC_R_SUCCESS) {
			ns_dbversion_t *dbversion;

			/*
			 * If the previous search returned a zone, detach it.
			 */
			if (zone != NULL)
				dns_zone_detach(&zone);

			/*
			 * If the previous search returned a database,
			 * detach it.
			 */
			if (*dbp != NULL)
				dns_db_detach(dbp);

			/*
			 * If the previous search returned a version, clear it.
			 */
			*versionp = NULL;

			dbversion = query_findversion(client, tdbp);
			if (dbversion == NULL) {
				tresult = ISC_R_NOMEMORY;
			} else {
				/*
				 * Be sure to return our database.
				 */
				*dbp = tdbp;
				*versionp = dbversion->version;
			}

			/*
			 * We return a null zone, No stats for DLZ zones.
			 */
			zone = NULL;
			result = tresult;
		}
	}

	/* If successful, Transfer ownership of zone. */
	if (result == ISC_R_SUCCESS) {
		*zonep = zone;
		/*
		 * If neither attempt above succeeded, return the cache instead
		 */
		*is_zonep = ISC_TRUE;
	} else if (result == ISC_R_NOTFOUND) {
		result = query_getcachedb(client, name, qtype, dbp, options);
		*is_zonep = ISC_FALSE;
	}
	return (result);
}

static inline isc_boolean_t
query_isduplicate(ns_client_t *client, dns_name_t *name,
		  dns_rdatatype_t type, dns_name_t **mnamep)
{
	dns_section_t section;
	dns_name_t *mname = NULL;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "query_isduplicate");

	for (section = DNS_SECTION_ANSWER;
	     section <= DNS_SECTION_ADDITIONAL;
	     section++) {
		result = dns_message_findname(client->message, section,
					      name, type, 0, &mname, NULL);
		if (result == ISC_R_SUCCESS) {
			/*
			 * We've already got this RRset in the response.
			 */
			CTRACE(ISC_LOG_DEBUG(3),
			       "query_isduplicate: true: done");
			return (ISC_TRUE);
		} else if (result == DNS_R_NXRRSET) {
			/*
			 * The name exists, but the rdataset does not.
			 */
			if (section == DNS_SECTION_ADDITIONAL)
				break;
		} else
			RUNTIME_CHECK(result == DNS_R_NXDOMAIN);
		mname = NULL;
	}

	if (mnamep != NULL)
		*mnamep = mname;

	CTRACE(ISC_LOG_DEBUG(3), "query_isduplicate: false: done");
	return (ISC_FALSE);
}

static isc_result_t
query_addadditional(void *arg, const dns_name_t *name, dns_rdatatype_t qtype) {
	ns_client_t *client = arg;
	isc_result_t result, eresult;
	dns_dbnode_t *node;
	dns_db_t *db;
	dns_name_t *fname, *mname;
	dns_rdataset_t *rdataset, *sigrdataset, *trdataset;
	isc_buffer_t *dbuf;
	isc_buffer_t b;
	ns_dbversion_t *dbversion;
	dns_dbversion_t *version;
	isc_boolean_t added_something, need_addname;
	dns_rdatatype_t type;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	dns_rdatasetadditional_t additionaltype;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(qtype != dns_rdatatype_any);

	if (!WANTDNSSEC(client) && dns_rdatatype_isdnssec(qtype))
		return (ISC_R_SUCCESS);

	CTRACE(ISC_LOG_DEBUG(3), "query_addadditional");

	/*
	 * Initialization.
	 */
	eresult = ISC_R_SUCCESS;
	fname = NULL;
	rdataset = NULL;
	sigrdataset = NULL;
	trdataset = NULL;
	db = NULL;
	version = NULL;
	node = NULL;
	added_something = ISC_FALSE;
	need_addname = ISC_FALSE;
	additionaltype = dns_rdatasetadditional_fromauth;

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * We treat type A additional section processing as if it
	 * were "any address type" additional section processing.
	 * To avoid multiple lookups, we do an 'any' database
	 * lookup and iterate over the node.
	 */
	if (qtype == dns_rdatatype_a)
		type = dns_rdatatype_any;
	else
		type = qtype;

	/*
	 * Get some resources.
	 */
	dbuf = query_getnamebuf(client);
	if (dbuf == NULL)
		goto cleanup;
	fname = query_newname(client, dbuf, &b);
	rdataset = query_newrdataset(client);
	if (fname == NULL || rdataset == NULL)
		goto cleanup;
	if (WANTDNSSEC(client)) {
		sigrdataset = query_newrdataset(client);
		if (sigrdataset == NULL)
			goto cleanup;
	}

	/*
	 * If we want only minimal responses and are here, then it must
	 * be for glue.
	 */
	if (client->view->minimalresponses == dns_minimal_yes)
		goto try_glue;

	/*
	 * Look within the same zone database for authoritative
	 * additional data.
	 */
	if (!client->query.authdbset || client->query.authdb == NULL)
		goto try_cache;

	dbversion = query_findversion(client, client->query.authdb);
	if (dbversion == NULL)
		goto try_cache;

	dns_db_attach(client->query.authdb, &db);
	version = dbversion->version;

	CTRACE(ISC_LOG_DEBUG(3), "query_addadditional: db_find");

	/*
	 * Since we are looking for authoritative data, we do not set
	 * the GLUEOK flag.  Glue will be looked for later, but not
	 * necessarily in the same database.
	 */
	result = dns_db_findext(db, name, version, type,
				client->query.dboptions,
				client->now, &node, fname, &cm, &ci,
				rdataset, sigrdataset);
	if (result == ISC_R_SUCCESS) {
		if (sigrdataset != NULL && !dns_db_issecure(db) &&
		    dns_rdataset_isassociated(sigrdataset))
			dns_rdataset_disassociate(sigrdataset);
		goto found;
	}

	if (dns_rdataset_isassociated(rdataset))
		dns_rdataset_disassociate(rdataset);
	if (sigrdataset != NULL && dns_rdataset_isassociated(sigrdataset))
		dns_rdataset_disassociate(sigrdataset);
	if (node != NULL)
		dns_db_detachnode(db, &node);
	version = NULL;
	dns_db_detach(&db);

	/*
	 * No authoritative data was found.  The cache is our next best bet.
	 */

 try_cache:
	if (!client->view->recursion)
		goto try_glue;

	additionaltype = dns_rdatasetadditional_fromcache;
	result = query_getcachedb(client, name, qtype, &db, DNS_GETDB_NOLOG);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Most likely the client isn't allowed to query the cache.
		 */
		goto try_glue;
	}
	/*
	 * Attempt to validate glue.
	 */
	if (sigrdataset == NULL) {
		sigrdataset = query_newrdataset(client);
		if (sigrdataset == NULL)
			goto cleanup;
	}

	version = NULL;
	result = dns_db_findext(db, name, version, type,
				client->query.dboptions |
				DNS_DBFIND_GLUEOK | DNS_DBFIND_ADDITIONALOK,
				client->now, &node, fname, &cm, &ci,
				rdataset, sigrdataset);

	dns_cache_updatestats(client->view->cache, result);
	if (!WANTDNSSEC(client))
		query_putrdataset(client, &sigrdataset);
	if (result == ISC_R_SUCCESS)
		goto found;

	if (dns_rdataset_isassociated(rdataset))
		dns_rdataset_disassociate(rdataset);
	if (sigrdataset != NULL && dns_rdataset_isassociated(sigrdataset))
		dns_rdataset_disassociate(sigrdataset);
	if (node != NULL)
		dns_db_detachnode(db, &node);
	dns_db_detach(&db);

 try_glue:
	/*
	 * No cached data was found.  Glue is our last chance.
	 * RFC1035 sayeth:
	 *
	 *	NS records cause both the usual additional section
	 *	processing to locate a type A record, and, when used
	 *	in a referral, a special search of the zone in which
	 *	they reside for glue information.
	 *
	 * This is the "special search".  Note that we must search
	 * the zone where the NS record resides, not the zone it
	 * points to, and that we only do the search in the delegation
	 * case (identified by client->query.gluedb being set).
	 */

	if (client->query.gluedb == NULL)
		goto cleanup;

	/*
	 * Don't poison caches using the bailiwick protection model.
	 */
	if (!dns_name_issubdomain(name, dns_db_origin(client->query.gluedb)))
		goto cleanup;

	dbversion = query_findversion(client, client->query.gluedb);
	if (dbversion == NULL)
		goto cleanup;

	dns_db_attach(client->query.gluedb, &db);
	version = dbversion->version;
	additionaltype = dns_rdatasetadditional_fromglue;
	result = dns_db_findext(db, name, version, type,
				client->query.dboptions | DNS_DBFIND_GLUEOK,
				client->now, &node, fname, &cm, &ci,
				rdataset, sigrdataset);
	if (!(result == ISC_R_SUCCESS ||
	      result == DNS_R_ZONECUT ||
	      result == DNS_R_GLUE))
		goto cleanup;

 found:
	/*
	 * We have found a potential additional data rdataset, or
	 * at least a node to iterate over.
	 */
	query_keepname(client, fname, dbuf);

	/*
	 * If we have an rdataset, add it to the additional data
	 * section.
	 */
	mname = NULL;
	if (dns_rdataset_isassociated(rdataset) &&
	    !query_isduplicate(client, fname, type, &mname)) {
		if (mname != NULL) {
			INSIST(mname != fname);
			query_releasename(client, &fname);
			fname = mname;
		} else
			need_addname = ISC_TRUE;
		ISC_LIST_APPEND(fname->list, rdataset, link);
		trdataset = rdataset;
		rdataset = NULL;
		added_something = ISC_TRUE;
		/*
		 * Note: we only add SIGs if we've added the type they cover,
		 * so we do not need to check if the SIG rdataset is already
		 * in the response.
		 */
		if (sigrdataset != NULL &&
		    dns_rdataset_isassociated(sigrdataset))
		{
			ISC_LIST_APPEND(fname->list, sigrdataset, link);
			sigrdataset = NULL;
		}
	}

	if (qtype == dns_rdatatype_a) {
		isc_boolean_t have_a = ISC_FALSE;

		/*
		 * We now go looking for A and AAAA records, along with
		 * their signatures.
		 *
		 * XXXRTH  This code could be more efficient.
		 */
		if (rdataset != NULL) {
			if (dns_rdataset_isassociated(rdataset))
				dns_rdataset_disassociate(rdataset);
		} else {
			rdataset = query_newrdataset(client);
			if (rdataset == NULL)
				goto addname;
		}
		if (sigrdataset != NULL) {
			if (dns_rdataset_isassociated(sigrdataset))
				dns_rdataset_disassociate(sigrdataset);
		} else if (WANTDNSSEC(client)) {
			sigrdataset = query_newrdataset(client);
			if (sigrdataset == NULL)
				goto addname;
		}
		if (query_isduplicate(client, fname, dns_rdatatype_a, NULL))
			goto aaaa_lookup;
		result = dns_db_findrdataset(db, node, version,
					     dns_rdatatype_a, 0,
					     client->now,
					     rdataset, sigrdataset);
		if (result == DNS_R_NCACHENXDOMAIN) {
			goto addname;
		} else if (result == DNS_R_NCACHENXRRSET) {
			dns_rdataset_disassociate(rdataset);
			if (sigrdataset != NULL &&
			    dns_rdataset_isassociated(sigrdataset))
				dns_rdataset_disassociate(sigrdataset);
		} else if (result == ISC_R_SUCCESS) {
			isc_boolean_t invalid = ISC_FALSE;
			mname = NULL;
			have_a = ISC_TRUE;
			if (additionaltype ==
			    dns_rdatasetadditional_fromcache &&
			    (DNS_TRUST_PENDING(rdataset->trust) ||
			     DNS_TRUST_GLUE(rdataset->trust)))
			{
				/* validate() may change rdataset->trust */
				invalid = ISC_TF(!!validate(client, db, fname,
							rdataset, sigrdataset));
			}
			if (invalid && DNS_TRUST_PENDING(rdataset->trust)) {
				dns_rdataset_disassociate(rdataset);
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
					dns_rdataset_disassociate(sigrdataset);
			} else if (!query_isduplicate(client, fname,
					       dns_rdatatype_a, &mname)) {
				if (mname != fname) {
					if (mname != NULL) {
						query_releasename(client,
								  &fname);
						fname = mname;
					} else
						need_addname = ISC_TRUE;
				}
				ISC_LIST_APPEND(fname->list, rdataset, link);
				added_something = ISC_TRUE;
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
				{
					ISC_LIST_APPEND(fname->list,
							sigrdataset, link);
					sigrdataset =
						query_newrdataset(client);
				}
				rdataset = query_newrdataset(client);
				if (rdataset == NULL)
					goto addname;
				if (WANTDNSSEC(client) && sigrdataset == NULL)
					goto addname;
			} else {
				dns_rdataset_disassociate(rdataset);
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
					dns_rdataset_disassociate(sigrdataset);
			}
		}
 aaaa_lookup:
		if (query_isduplicate(client, fname, dns_rdatatype_aaaa, NULL))
			goto addname;
		result = dns_db_findrdataset(db, node, version,
					     dns_rdatatype_aaaa, 0,
					     client->now,
					     rdataset, sigrdataset);
		if (result == DNS_R_NCACHENXDOMAIN) {
			goto addname;
		} else if (result == DNS_R_NCACHENXRRSET) {
			dns_rdataset_disassociate(rdataset);
			if (sigrdataset != NULL &&
			    dns_rdataset_isassociated(sigrdataset))
				dns_rdataset_disassociate(sigrdataset);
		} else if (result == ISC_R_SUCCESS) {
			isc_boolean_t invalid = ISC_FALSE;
			mname = NULL;
			/*
			 * There's an A; check whether we're filtering AAAA
			 */
			if (have_a &&
			    (client->filter_aaaa == dns_aaaa_break_dnssec ||
			    (client->filter_aaaa == dns_aaaa_filter &&
			     (!WANTDNSSEC(client) || sigrdataset == NULL ||
			      !dns_rdataset_isassociated(sigrdataset)))))
				goto addname;
			if (additionaltype ==
			    dns_rdatasetadditional_fromcache &&
			    (DNS_TRUST_PENDING(rdataset->trust) ||
			     DNS_TRUST_GLUE(rdataset->trust)))
			{
				/* validate() may change rdataset->trust */
				invalid = ISC_TF(!!validate(client, db, fname,
							rdataset, sigrdataset));
			}

			if (invalid && DNS_TRUST_PENDING(rdataset->trust)) {
				dns_rdataset_disassociate(rdataset);
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
					dns_rdataset_disassociate(sigrdataset);
			} else if (!query_isduplicate(client, fname,
					       dns_rdatatype_aaaa, &mname)) {
				if (mname != fname) {
					if (mname != NULL) {
						query_releasename(client,
								  &fname);
						fname = mname;
					} else
						need_addname = ISC_TRUE;
				}
				ISC_LIST_APPEND(fname->list, rdataset, link);
				added_something = ISC_TRUE;
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
				{
					ISC_LIST_APPEND(fname->list,
							sigrdataset, link);
					sigrdataset = NULL;
				}
				rdataset = NULL;
			}
		}
	}

 addname:
	CTRACE(ISC_LOG_DEBUG(3), "query_addadditional: addname");
	/*
	 * If we haven't added anything, then we're done.
	 */
	if (!added_something)
		goto cleanup;

	/*
	 * We may have added our rdatasets to an existing name, if so, then
	 * need_addname will be ISC_FALSE.  Whether we used an existing name
	 * or a new one, we must set fname to NULL to prevent cleanup.
	 */
	if (need_addname)
		dns_message_addname(client->message, fname,
				    DNS_SECTION_ADDITIONAL);
	fname = NULL;

	/*
	 * In a few cases, we want to add additional data for additional
	 * data.  It's simpler to just deal with special cases here than
	 * to try to create a general purpose mechanism and allow the
	 * rdata implementations to do it themselves.
	 *
	 * This involves recursion, but the depth is limited.  The
	 * most complex case is adding a SRV rdataset, which involves
	 * recursing to add address records, which in turn can cause
	 * recursion to add KEYs.
	 */
	if (type == dns_rdatatype_srv && trdataset != NULL) {
		/*
		 * If we're adding SRV records to the additional data
		 * section, it's helpful if we add the SRV additional data
		 * as well.
		 */
		eresult = dns_rdataset_additionaldata(trdataset,
						      query_addadditional,
						      client);
	}

 cleanup:
	CTRACE(ISC_LOG_DEBUG(3), "query_addadditional: cleanup");
	query_putrdataset(client, &rdataset);
	if (sigrdataset != NULL)
		query_putrdataset(client, &sigrdataset);
	if (fname != NULL)
		query_releasename(client, &fname);
	if (node != NULL)
		dns_db_detachnode(db, &node);
	if (db != NULL)
		dns_db_detach(&db);

	CTRACE(ISC_LOG_DEBUG(3), "query_addadditional: done");
	return (eresult);
}

static void
query_addrdataset(ns_client_t *client, dns_section_t section,
		  dns_name_t *fname, dns_rdataset_t *rdataset)
{
	UNUSED(section);

	/*
	 * Add 'rdataset' and any pertinent additional data to
	 * 'fname', a name in the response message for 'client'.
	 */

	CTRACE(ISC_LOG_DEBUG(3), "query_addrdataset");

	ISC_LIST_APPEND(fname->list, rdataset, link);

	if (client->view->order != NULL)
		rdataset->attributes |= dns_order_find(client->view->order,
						       fname, rdataset->type,
						       rdataset->rdclass);
	rdataset->attributes |= DNS_RDATASETATTR_LOADORDER;

	if (NOADDITIONAL(client))
		return;

	/*
	 * Try to process glue directly.
	 */
	if (client->view->use_glue_cache &&
	    (rdataset->type == dns_rdatatype_ns) &&
	    (client->query.gluedb != NULL) &&
	    dns_db_iszone(client->query.gluedb))
	{
		isc_result_t result;
		ns_dbversion_t *dbversion;
		unsigned int options = 0;

		dbversion = query_findversion(client, client->query.gluedb);
		if (dbversion == NULL)
			goto regular;

		if (client->filter_aaaa == dns_aaaa_filter ||
		    client->filter_aaaa == dns_aaaa_break_dnssec)
		{
			options |= DNS_RDATASETADDGLUE_FILTERAAAA;
		}

		result = dns_rdataset_addglue(rdataset, dbversion->version,
					      options, client->message);
		if (result == ISC_R_SUCCESS)
			return;
	}

regular:
	/*
	 * Add additional data.
	 *
	 * We don't care if dns_rdataset_additionaldata() fails.
	 */
	(void)dns_rdataset_additionaldata(rdataset, query_addadditional,
					  client);
	CTRACE(ISC_LOG_DEBUG(3), "query_addrdataset: done");
}

static void
query_addrrset(ns_client_t *client, dns_name_t **namep,
	       dns_rdataset_t **rdatasetp, dns_rdataset_t **sigrdatasetp,
	       isc_buffer_t *dbuf, dns_section_t section)
{
	dns_name_t *name = *namep, *mname = NULL;
	dns_rdataset_t *rdataset = *rdatasetp, *mrdataset = NULL;
	dns_rdataset_t *sigrdataset = NULL;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "query_addrrset");

	if (sigrdatasetp != NULL)
		sigrdataset = *sigrdatasetp;

	/*%
	 * To the current response for 'client', add the answer RRset
	 * '*rdatasetp' and an optional signature set '*sigrdatasetp', with
	 * owner name '*namep', to section 'section', unless they are
	 * already there.  Also add any pertinent additional data.
	 *
	 * If 'dbuf' is not NULL, then '*namep' is the name whose data is
	 * stored in 'dbuf'.  In this case, query_addrrset() guarantees that
	 * when it returns the name will either have been kept or released.
	 */
	result = dns_message_findname(client->message, section,
				      name, rdataset->type, rdataset->covers,
				      &mname, &mrdataset);
	if (result == ISC_R_SUCCESS) {
		/*
		 * We've already got an RRset of the given name and type.
		 */
		CTRACE(ISC_LOG_DEBUG(3),
		       "query_addrrset: dns_message_findname succeeded: done");
		if (dbuf != NULL)
			query_releasename(client, namep);
		if ((rdataset->attributes & DNS_RDATASETATTR_REQUIRED) != 0)
			mrdataset->attributes |= DNS_RDATASETATTR_REQUIRED;
		return;
	} else if (result == DNS_R_NXDOMAIN) {
		/*
		 * The name doesn't exist.
		 */
		if (dbuf != NULL)
			query_keepname(client, name, dbuf);
		dns_message_addname(client->message, name, section);
		*namep = NULL;
		mname = name;
	} else {
		RUNTIME_CHECK(result == DNS_R_NXRRSET);
		if (dbuf != NULL)
			query_releasename(client, namep);
	}

	if (rdataset->trust != dns_trust_secure &&
	    (section == DNS_SECTION_ANSWER ||
	     section == DNS_SECTION_AUTHORITY))
		client->query.attributes &= ~NS_QUERYATTR_SECURE;

	/*
	 * Note: we only add SIGs if we've added the type they cover, so
	 * we do not need to check if the SIG rdataset is already in the
	 * response.
	 */
	query_addrdataset(client, section, mname, rdataset);
	*rdatasetp = NULL;
	if (sigrdataset != NULL && dns_rdataset_isassociated(sigrdataset)) {
		/*
		 * We have a signature.  Add it to the response.
		 */
		ISC_LIST_APPEND(mname->list, sigrdataset, link);
		*sigrdatasetp = NULL;
	}

	CTRACE(ISC_LOG_DEBUG(3), "query_addrrset: done");
}

/*
 * Mark the RRsets as secure.  Update the cache (db) to reflect the
 * change in trust level.
 */
static void
mark_secure(ns_client_t *client, dns_db_t *db, dns_name_t *name,
	    dns_rdata_rrsig_t *rrsig, dns_rdataset_t *rdataset,
	    dns_rdataset_t *sigrdataset)
{
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	isc_stdtime_t now;

	rdataset->trust = dns_trust_secure;
	sigrdataset->trust = dns_trust_secure;
	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * Save the updated secure state.  Ignore failures.
	 */
	result = dns_db_findnodeext(db, name, ISC_TRUE, &cm, &ci, &node);
	if (result != ISC_R_SUCCESS)
		return;

	isc_stdtime_get(&now);
	dns_rdataset_trimttl(rdataset, sigrdataset, rrsig, now,
			     client->view->acceptexpired);

	(void)dns_db_addrdataset(db, node, NULL, client->now, rdataset,
				 0, NULL);
	(void)dns_db_addrdataset(db, node, NULL, client->now, sigrdataset,
				 0, NULL);
	dns_db_detachnode(db, &node);
}

/*
 * Find the secure key that corresponds to rrsig.
 * Note: 'keyrdataset' maintains state between successive calls,
 * there may be multiple keys with the same keyid.
 * Return ISC_FALSE if we have exhausted all the possible keys.
 */
static isc_boolean_t
get_key(ns_client_t *client, dns_db_t *db, dns_rdata_rrsig_t *rrsig,
	dns_rdataset_t *keyrdataset, dst_key_t **keyp)
{
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	isc_boolean_t secure = ISC_FALSE;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	if (!dns_rdataset_isassociated(keyrdataset)) {
		result = dns_db_findnodeext(db, &rrsig->signer, ISC_FALSE,
					    &cm, &ci, &node);
		if (result != ISC_R_SUCCESS)
			return (ISC_FALSE);

		result = dns_db_findrdataset(db, node, NULL,
					     dns_rdatatype_dnskey, 0,
					     client->now, keyrdataset, NULL);
		dns_db_detachnode(db, &node);
		if (result != ISC_R_SUCCESS)
			return (ISC_FALSE);

		if (keyrdataset->trust != dns_trust_secure)
			return (ISC_FALSE);

		result = dns_rdataset_first(keyrdataset);
	} else
		result = dns_rdataset_next(keyrdataset);

	for ( ; result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(keyrdataset)) {
		dns_rdata_t rdata = DNS_RDATA_INIT;
		isc_buffer_t b;

		dns_rdataset_current(keyrdataset, &rdata);
		isc_buffer_init(&b, rdata.data, rdata.length);
		isc_buffer_add(&b, rdata.length);
		result = dst_key_fromdns(&rrsig->signer, rdata.rdclass, &b,
					 client->mctx, keyp);
		if (result != ISC_R_SUCCESS)
			continue;
		if (rrsig->algorithm == (dns_secalg_t)dst_key_alg(*keyp) &&
		    rrsig->keyid == (dns_keytag_t)dst_key_id(*keyp) &&
		    dst_key_iszonekey(*keyp)) {
			secure = ISC_TRUE;
			break;
		}
		dst_key_free(keyp);
	}
	return (secure);
}

static isc_boolean_t
verify(dst_key_t *key, dns_name_t *name, dns_rdataset_t *rdataset,
       dns_rdata_t *rdata, ns_client_t *client)
{
	isc_result_t result;
	dns_fixedname_t fixed;
	isc_boolean_t ignore = ISC_FALSE;

	dns_fixedname_init(&fixed);

again:
	result = dns_dnssec_verify3(name, rdataset, key, ignore,
				    client->view->maxbits, client->mctx,
				    rdata, NULL);
	if (result == DNS_R_SIGEXPIRED && client->view->acceptexpired) {
		ignore = ISC_TRUE;
		goto again;
	}
	if (result == ISC_R_SUCCESS || result == DNS_R_FROMWILDCARD)
		return (ISC_TRUE);
	return (ISC_FALSE);
}

/*
 * Validate the rdataset if possible with available records.
 */
static isc_boolean_t
validate(ns_client_t *client, dns_db_t *db, dns_name_t *name,
	 dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_rrsig_t rrsig;
	dst_key_t *key = NULL;
	dns_rdataset_t keyrdataset;

	if (sigrdataset == NULL || !dns_rdataset_isassociated(sigrdataset))
		return (ISC_FALSE);

	for (result = dns_rdataset_first(sigrdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(sigrdataset)) {

		dns_rdata_reset(&rdata);
		dns_rdataset_current(sigrdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &rrsig, NULL);
		if (result != ISC_R_SUCCESS)
			return (ISC_FALSE);
		if (!dns_resolver_algorithm_supported(client->view->resolver,
						      name, rrsig.algorithm))
			continue;
		if (!dns_name_issubdomain(name, &rrsig.signer))
			continue;
		dns_rdataset_init(&keyrdataset);
		do {
			if (!get_key(client, db, &rrsig, &keyrdataset, &key))
				break;
			if (verify(key, name, rdataset, &rdata, client)) {
				dst_key_free(&key);
				dns_rdataset_disassociate(&keyrdataset);
				mark_secure(client, db, name, &rrsig,
					    rdataset, sigrdataset);
				return (ISC_TRUE);
			}
			dst_key_free(&key);
		} while (1);
		if (dns_rdataset_isassociated(&keyrdataset))
			dns_rdataset_disassociate(&keyrdataset);
	}
	return (ISC_FALSE);
}

static void
fixrdataset(ns_client_t *client, dns_rdataset_t **rdataset) {
	if (*rdataset == NULL)
		*rdataset = query_newrdataset(client);
	else  if (dns_rdataset_isassociated(*rdataset))
		dns_rdataset_disassociate(*rdataset);
}

static void
fixfname(ns_client_t *client, dns_name_t **fname, isc_buffer_t **dbuf,
	 isc_buffer_t *nbuf)
{
	if (*fname == NULL) {
		*dbuf = query_getnamebuf(client);
		if (*dbuf == NULL)
			return;
		*fname = query_newname(client, *dbuf, nbuf);
	}
}

static void
free_devent(ns_client_t *client, isc_event_t **eventp,
	    dns_fetchevent_t **deventp)
{
	dns_fetchevent_t *devent = *deventp;

	REQUIRE((void*)(*eventp) == (void *)(*deventp));

	if (devent->fetch != NULL)
		dns_resolver_destroyfetch(&devent->fetch);
	if (devent->node != NULL)
		dns_db_detachnode(devent->db, &devent->node);
	if (devent->db != NULL)
		dns_db_detach(&devent->db);
	if (devent->rdataset != NULL)
		query_putrdataset(client, &devent->rdataset);
	if (devent->sigrdataset != NULL)
		query_putrdataset(client, &devent->sigrdataset);
	/*
	 * If the two pointers are the same then leave the setting of
	 * (*deventp) to NULL to isc_event_free.
	 */
	if ((void *)eventp != (void *)deventp)
		(*deventp) = NULL;
	isc_event_free(eventp);
}

static void
prefetch_done(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent = (dns_fetchevent_t *)event;
	ns_client_t *client;

	UNUSED(task);

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	client = devent->ev_arg;
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);

	LOCK(&client->query.fetchlock);
	if (client->query.prefetch != NULL) {
		INSIST(devent->fetch == client->query.prefetch);
		client->query.prefetch = NULL;
	}
	UNLOCK(&client->query.fetchlock);
	free_devent(client, &event, &devent);
	ns_client_detach(&client);
}

static void
query_prefetch(ns_client_t *client, dns_name_t *qname,
	       dns_rdataset_t *rdataset)
{
	isc_result_t result;
	isc_sockaddr_t *peeraddr;
	dns_rdataset_t *tmprdataset;
	ns_client_t *dummy = NULL;
	unsigned int options;

	if (client->query.prefetch != NULL ||
	    client->view->prefetch_trigger == 0U ||
	    rdataset->ttl > client->view->prefetch_trigger ||
	    (rdataset->attributes & DNS_RDATASETATTR_PREFETCH) == 0)
		return;

	if (client->recursionquota == NULL) {
		result = isc_quota_attach(&client->sctx->recursionquota,
					  &client->recursionquota);
		if (result == ISC_R_SUCCESS && !client->mortal && !TCP(client))
			result = ns_client_replace(client);
		if (result != ISC_R_SUCCESS)
			return;
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_recursclients);
	}

	tmprdataset = query_newrdataset(client);
	if (tmprdataset == NULL)
		return;
	if (!TCP(client))
		peeraddr = &client->peeraddr;
	else
		peeraddr = NULL;
	ns_client_attach(client, &dummy);
	options = client->query.fetchoptions | DNS_FETCHOPT_PREFETCH;
	result = dns_resolver_createfetch3(client->view->resolver,
					   qname, rdataset->type, NULL, NULL,
					   NULL, peeraddr, client->message->id,
					   options, 0, NULL, client->task,
					   prefetch_done, client,
					   tmprdataset, NULL,
					   &client->query.prefetch);
	if (result != ISC_R_SUCCESS) {
		query_putrdataset(client, &tmprdataset);
		ns_client_detach(&dummy);
	}
	dns_rdataset_clearprefetch(rdataset);
	ns_stats_increment(client->sctx->nsstats,
			   ns_statscounter_prefetch);
}

static inline void
rpz_clean(dns_zone_t **zonep, dns_db_t **dbp, dns_dbnode_t **nodep,
	  dns_rdataset_t **rdatasetp)
{
	if (nodep != NULL && *nodep != NULL) {
		REQUIRE(dbp != NULL && *dbp != NULL);
		dns_db_detachnode(*dbp, nodep);
	}
	if (dbp != NULL && *dbp != NULL)
		dns_db_detach(dbp);
	if (zonep != NULL && *zonep != NULL)
		dns_zone_detach(zonep);
	if (rdatasetp != NULL && *rdatasetp != NULL &&
	    dns_rdataset_isassociated(*rdatasetp))
		dns_rdataset_disassociate(*rdatasetp);
}

static inline void
rpz_match_clear(dns_rpz_st_t *st) {
	rpz_clean(&st->m.zone, &st->m.db, &st->m.node, &st->m.rdataset);
	st->m.version = NULL;
}

static inline isc_result_t
rpz_ready(ns_client_t *client, dns_rdataset_t **rdatasetp) {
	REQUIRE(rdatasetp != NULL);

	CTRACE(ISC_LOG_DEBUG(3), "rpz_ready");

	if (*rdatasetp == NULL) {
		*rdatasetp = query_newrdataset(client);
		if (*rdatasetp == NULL) {
			CTRACE(ISC_LOG_ERROR,
			       "rpz_ready: query_newrdataset failed");
			return (DNS_R_SERVFAIL);
		}
	} else if (dns_rdataset_isassociated(*rdatasetp)) {
		dns_rdataset_disassociate(*rdatasetp);
	}
	return (ISC_R_SUCCESS);
}

static void
rpz_st_clear(ns_client_t *client) {
	dns_rpz_st_t *st = client->query.rpz_st;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_st_clear");

	if (st->m.rdataset != NULL) {
		query_putrdataset(client, &st->m.rdataset);
	}
	rpz_match_clear(st);

	rpz_clean(NULL, &st->r.db, NULL, NULL);
	if (st->r.ns_rdataset != NULL) {
		query_putrdataset(client, &st->r.ns_rdataset);
	}
	if (st->r.r_rdataset != NULL) {
		query_putrdataset(client, &st->r.r_rdataset);
	}

	rpz_clean(&st->q.zone, &st->q.db, &st->q.node, NULL);
	if (st->q.rdataset != NULL) {
		query_putrdataset(client, &st->q.rdataset);
	}
	if (st->q.sigrdataset != NULL) {
		query_putrdataset(client, &st->q.sigrdataset);
	}
	st->state = 0;
	st->m.type = DNS_RPZ_TYPE_BAD;
	st->m.policy = DNS_RPZ_POLICY_MISS;
	if (st->rpsdb != NULL) {
		dns_db_detach(&st->rpsdb);
	}

}

static dns_rpz_zbits_t
rpz_get_zbits(ns_client_t *client,
	      dns_rdatatype_t ip_type, dns_rpz_type_t rpz_type)
{
	dns_rpz_st_t *st;
	dns_rpz_zbits_t zbits;

	REQUIRE(client != NULL);
	REQUIRE(client->query.rpz_st != NULL);

	st = client->query.rpz_st;

#ifdef USE_DNSRPS
	if (st->popt.dnsrps_enabled) {
		if (st->rpsdb == NULL ||
		    librpz->have_trig(dns_dnsrps_type2trig(rpz_type),
				      ip_type == dns_rdatatype_aaaa,
				      ((rpsdb_t *)st->rpsdb)->rsp))
		{
			return (DNS_RPZ_ALL_ZBITS);
		}
		return (0);
	}
#endif

	switch (rpz_type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		zbits = st->have.client_ip;
		break;
	case DNS_RPZ_TYPE_QNAME:
		zbits = st->have.qname;
		break;
	case DNS_RPZ_TYPE_IP:
		if (ip_type == dns_rdatatype_a) {
			zbits = st->have.ipv4;
		} else if (ip_type == dns_rdatatype_aaaa) {
			zbits = st->have.ipv6;
		} else {
			zbits = st->have.ip;
		}
		break;
	case DNS_RPZ_TYPE_NSDNAME:
		zbits = st->have.nsdname;
		break;
	case DNS_RPZ_TYPE_NSIP:
		if (ip_type == dns_rdatatype_a) {
			zbits = st->have.nsipv4;
		} else if (ip_type == dns_rdatatype_aaaa) {
			zbits = st->have.nsipv6;
		} else {
			zbits = st->have.nsip;
		}
		break;
	default:
		INSIST(0);
		break;
	}

	/*
	 * Choose
	 *	the earliest configured policy zone (rpz->num)
	 *	QNAME over IP over NSDNAME over NSIP (rpz_type)
	 *	the smallest name,
	 *	the longest IP address prefix,
	 *	the lexically smallest address.
	 */
	if (st->m.policy != DNS_RPZ_POLICY_MISS) {
		if (st->m.type >= rpz_type) {
			zbits &= DNS_RPZ_ZMASK(st->m.rpz->num);
		} else{
			zbits &= DNS_RPZ_ZMASK(st->m.rpz->num) >> 1;
		}
	}

	/*
	 * If the client wants recursion, allow only compatible policies.
	 */
	if (!RECURSIONOK(client))
		zbits &= st->popt.no_rd_ok;

	return (zbits);
}

static void
query_rpzfetch(ns_client_t *client, dns_name_t *qname, dns_rdatatype_t type) {
	isc_result_t result;
	isc_sockaddr_t *peeraddr;
	dns_rdataset_t *tmprdataset;
	ns_client_t *dummy = NULL;
	unsigned int options;

	if (client->query.prefetch != NULL)
		return;

	if (client->recursionquota == NULL) {
		result = isc_quota_attach(&client->sctx->recursionquota,
					  &client->recursionquota);
		if (result == ISC_R_SUCCESS && !client->mortal && !TCP(client))
			result = ns_client_replace(client);
		if (result != ISC_R_SUCCESS)
			return;
		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_recursclients);
	}

	tmprdataset = query_newrdataset(client);
	if (tmprdataset == NULL)
		return;
	if (!TCP(client))
		peeraddr = &client->peeraddr;
	else
		peeraddr = NULL;
	ns_client_attach(client, &dummy);
	options = client->query.fetchoptions;
	result = dns_resolver_createfetch3(client->view->resolver, qname, type,
					   NULL, NULL, NULL, peeraddr,
					   client->message->id, options, 0,
					   NULL, client->task, prefetch_done,
					   client, tmprdataset, NULL,
					   &client->query.prefetch);
	if (result != ISC_R_SUCCESS) {
		query_putrdataset(client, &tmprdataset);
		ns_client_detach(&dummy);
	}
}

/*
 * Get an NS, A, or AAAA rrset related to the response for the client
 * to check the contents of that rrset for hits by eligible policy zones.
 */
static isc_result_t
rpz_rrset_find(ns_client_t *client, dns_name_t *name, dns_rdatatype_t type,
	       dns_rpz_type_t rpz_type, dns_db_t **dbp,
	       dns_dbversion_t *version, dns_rdataset_t **rdatasetp,
	       isc_boolean_t resuming)
{
	dns_rpz_st_t *st;
	isc_boolean_t is_zone;
	dns_dbnode_t *node;
	dns_fixedname_t fixed;
	dns_name_t *found;
	isc_result_t result;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rrset_find");

	st = client->query.rpz_st;
	if ((st->state & DNS_RPZ_RECURSING) != 0) {
		INSIST(st->r.r_type == type);
		INSIST(dns_name_equal(name, st->r_name));
		INSIST(*rdatasetp == NULL ||
		       !dns_rdataset_isassociated(*rdatasetp));
		st->state &= ~DNS_RPZ_RECURSING;
		RESTORE(*dbp, st->r.db);
		if (*rdatasetp != NULL)
			query_putrdataset(client, rdatasetp);
		RESTORE(*rdatasetp, st->r.r_rdataset);
		result = st->r.r_result;
		if (result == DNS_R_DELEGATION) {
			CTRACE(ISC_LOG_ERROR, "RPZ recursing");
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, name,
				     rpz_type, " rpz_rrset_find(1)", result);
			st->m.policy = DNS_RPZ_POLICY_ERROR;
			result = DNS_R_SERVFAIL;
		}
		return (result);
	}

	result = rpz_ready(client, rdatasetp);
	if (result != ISC_R_SUCCESS) {
		st->m.policy = DNS_RPZ_POLICY_ERROR;
		return (result);
	}
	if (*dbp != NULL) {
		is_zone = ISC_FALSE;
	} else {
		dns_zone_t *zone;

		version = NULL;
		zone = NULL;
		result = query_getdb(client, name, type, 0, &zone, dbp,
				     &version, &is_zone);
		if (result != ISC_R_SUCCESS) {
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, name,
				     rpz_type, " rpz_rrset_find(2)", result);
			st->m.policy = DNS_RPZ_POLICY_ERROR;
			if (zone != NULL)
				dns_zone_detach(&zone);
			return (result);
		}
		if (zone != NULL)
			dns_zone_detach(&zone);
	}

	node = NULL;
	found = dns_fixedname_initname(&fixed);
	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);
	result = dns_db_findext(*dbp, name, version, type, DNS_DBFIND_GLUEOK,
				client->now, &node, found,
				&cm, &ci, *rdatasetp, NULL);
	if (result == DNS_R_DELEGATION && is_zone && USECACHE(client)) {
		/*
		 * Try the cache if we're authoritative for an
		 * ancestor but not the domain itself.
		 */
		rpz_clean(NULL, dbp, &node, rdatasetp);
		version = NULL;
		dns_db_attach(client->view->cachedb, dbp);
		result = dns_db_findext(*dbp, name, version, type,
					0, client->now, &node, found,
					&cm, &ci, *rdatasetp, NULL);
	}
	rpz_clean(NULL, dbp, &node, NULL);
	if (result == DNS_R_DELEGATION) {
		rpz_clean(NULL, NULL, NULL, rdatasetp);
		/*
		 * Recurse for NS rrset or A or AAAA rrset for an NS.
		 * Do not recurse for addresses for the query name.
		 */
		if (rpz_type == DNS_RPZ_TYPE_IP) {
			result = DNS_R_NXRRSET;
		} else if (!client->view->rpzs->p.nsip_wait_recurse) {
			query_rpzfetch(client, name, type);
			result = DNS_R_NXRRSET;
		} else {
			dns_name_copy(name, st->r_name, NULL);
			result = query_recurse(client, type, st->r_name,
					       NULL, NULL, resuming);
			if (result == ISC_R_SUCCESS) {
				st->state |= DNS_RPZ_RECURSING;
				result = DNS_R_DELEGATION;
			}
		}
	}
	return (result);
}

/*
 * Compute a policy owner name, p_name, in a policy zone given the needed
 * policy type and the trigger name.
 */
static isc_result_t
rpz_get_p_name(ns_client_t *client, dns_name_t *p_name,
	       dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	       dns_name_t *trig_name)
{
	dns_offsets_t prefix_offsets;
	dns_name_t prefix, *suffix;
	unsigned int first, labels;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_get_p_name");

	/*
	 * The policy owner name consists of a suffix depending on the type
	 * and policy zone and a prefix that is the longest possible string
	 * from the trigger name that keesp the resulting policy owner name
	 * from being too long.
	 */
	switch (rpz_type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		suffix = &rpz->client_ip;
		break;
	case DNS_RPZ_TYPE_QNAME:
		suffix = &rpz->origin;
		break;
	case DNS_RPZ_TYPE_IP:
		suffix = &rpz->ip;
		break;
	case DNS_RPZ_TYPE_NSDNAME:
		suffix = &rpz->nsdname;
		break;
	case DNS_RPZ_TYPE_NSIP:
		suffix = &rpz->nsip;
		break;
	default:
		INSIST(0);
	}

	/*
	 * Start with relative version of the full trigger name,
	 * and trim enough allow the addition of the suffix.
	 */
	dns_name_init(&prefix, prefix_offsets);
	labels = dns_name_countlabels(trig_name);
	first = 0;
	for (;;) {
		dns_name_getlabelsequence(trig_name, first, labels-first-1,
					  &prefix);
		result = dns_name_concatenate(&prefix, suffix, p_name, NULL);
		if (result == ISC_R_SUCCESS)
			break;
		INSIST(result == DNS_R_NAMETOOLONG);
		/*
		 * Trim the trigger name until the combination is not too long.
		 */
		if (labels-first < 2) {
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, suffix,
				     rpz_type, " concatentate()", result);
			return (ISC_R_FAILURE);
		}
		/*
		 * Complain once about trimming the trigger name.
		 */
		if (first == 0) {
			rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL1, suffix,
				     rpz_type, " concatentate()", result);
		}
		++first;
	}
	return (ISC_R_SUCCESS);
}

/*
 * Look in policy zone rpz for a policy of rpz_type by p_name.
 * The self-name (usually the client qname or an NS name) is compared with
 * the target of a CNAME policy for the old style passthru encoding.
 * If found, the policy is recorded in *zonep, *dbp, *versionp, *nodep,
 * *rdatasetp, and *policyp.
 * The target DNS type, qtype, chooses the best rdataset for *rdatasetp.
 * The caller must decide if the found policy is most suitable, including
 * better than a previously found policy.
 * If it is best, the caller records it in client->query.rpz_st->m.
 */
static isc_result_t
rpz_find_p(ns_client_t *client, dns_name_t *self_name, dns_rdatatype_t qtype,
	   dns_name_t *p_name, dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	   dns_zone_t **zonep, dns_db_t **dbp, dns_dbversion_t **versionp,
	   dns_dbnode_t **nodep, dns_rdataset_t **rdatasetp,
	   dns_rpz_policy_t *policyp)
{
	dns_fixedname_t foundf;
	dns_name_t *found;
	isc_result_t result;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	REQUIRE(nodep != NULL);

	CTRACE(ISC_LOG_DEBUG(3), "rpz_find_p");

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * Try to find either a CNAME or the type of record demanded by the
	 * request from the policy zone.
	 */
	rpz_clean(zonep, dbp, nodep, rdatasetp);
	result = rpz_ready(client, rdatasetp);
	if (result != ISC_R_SUCCESS) {
		CTRACE(ISC_LOG_ERROR, "rpz_ready() failed");
		return (DNS_R_SERVFAIL);
	}
	*versionp = NULL;
	result = rpz_getdb(client, p_name, rpz_type, zonep, dbp, versionp);
	if (result != ISC_R_SUCCESS)
		return (DNS_R_NXDOMAIN);
	found = dns_fixedname_initname(&foundf);

	result = dns_db_findext(*dbp, p_name, *versionp, dns_rdatatype_any, 0,
				client->now, nodep, found, &cm, &ci,
				*rdatasetp, NULL);
	/*
	 * Choose the best rdataset if we found something.
	 */
	if (result == ISC_R_SUCCESS) {
		dns_rdatasetiter_t *rdsiter;

		rdsiter = NULL;
		result = dns_db_allrdatasets(*dbp, *nodep, *versionp, 0,
					     &rdsiter);
		if (result != ISC_R_SUCCESS) {
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, p_name,
				     rpz_type, " allrdatasets()", result);
			CTRACE(ISC_LOG_ERROR,
			       "rpz_find_p: allrdatasets failed");
			return (DNS_R_SERVFAIL);
		}
		for (result = dns_rdatasetiter_first(rdsiter);
		     result == ISC_R_SUCCESS;
		     result = dns_rdatasetiter_next(rdsiter)) {
			dns_rdatasetiter_current(rdsiter, *rdatasetp);
			if ((*rdatasetp)->type == dns_rdatatype_cname ||
			    (*rdatasetp)->type == qtype)
				break;
			dns_rdataset_disassociate(*rdatasetp);
		}
		dns_rdatasetiter_destroy(&rdsiter);
		if (result != ISC_R_SUCCESS) {
			if (result != ISC_R_NOMORE) {
				rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL,
					     p_name, rpz_type,
					     " rdatasetiter", result);
				CTRACE(ISC_LOG_ERROR,
				       "rpz_find_p: rdatasetiter failed");
				return (DNS_R_SERVFAIL);
			}
			/*
			 * Ask again to get the right DNS_R_DNAME/NXRRSET/...
			 * result if there is neither a CNAME nor target type.
			 */
			if (dns_rdataset_isassociated(*rdatasetp))
				dns_rdataset_disassociate(*rdatasetp);
			dns_db_detachnode(*dbp, nodep);

			if (qtype == dns_rdatatype_rrsig ||
			    qtype == dns_rdatatype_sig)
				result = DNS_R_NXRRSET;
			else
				result = dns_db_findext(*dbp, p_name, *versionp,
							qtype, 0, client->now,
							nodep, found, &cm, &ci,
							*rdatasetp, NULL);
		}
	}
	switch (result) {
	case ISC_R_SUCCESS:
		if ((*rdatasetp)->type != dns_rdatatype_cname) {
			*policyp = DNS_RPZ_POLICY_RECORD;
		} else {
			*policyp = dns_rpz_decode_cname(rpz, *rdatasetp,
							self_name);
			if ((*policyp == DNS_RPZ_POLICY_RECORD ||
			     *policyp == DNS_RPZ_POLICY_WILDCNAME) &&
			    qtype != dns_rdatatype_cname &&
			    qtype != dns_rdatatype_any)
				return (DNS_R_CNAME);
		}
		return (ISC_R_SUCCESS);
	case DNS_R_NXRRSET:
		*policyp = DNS_RPZ_POLICY_NODATA;
		return (result);
	case DNS_R_DNAME:
		/*
		 * DNAME policy RRs have very few if any uses that are not
		 * better served with simple wildcards.  Making them work would
		 * require complications to get the number of labels matched
		 * in the name or the found name to the main DNS_R_DNAME case
		 * in query_dname().  The domain also does not appear in the
		 * summary database at the right level, so this happens only
		 * with a single policy zone when we have no summary database.
		 * Treat it as a miss.
		 */
	case DNS_R_NXDOMAIN:
	case DNS_R_EMPTYNAME:
		return (DNS_R_NXDOMAIN);
	default:
		rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, p_name, rpz_type,
			     "", result);
		CTRACE(ISC_LOG_ERROR,
		       "rpz_find_p: unexpected result");
		return (DNS_R_SERVFAIL);
	}
}

static void
rpz_save_p(dns_rpz_st_t *st, dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	   dns_rpz_policy_t policy, dns_name_t *p_name, dns_rpz_prefix_t prefix,
	   isc_result_t result, dns_zone_t **zonep, dns_db_t **dbp,
	   dns_dbnode_t **nodep, dns_rdataset_t **rdatasetp,
	   dns_dbversion_t *version)
{
	dns_rdataset_t *trdataset = NULL;

	rpz_match_clear(st);
	st->m.rpz = rpz;
	st->m.type = rpz_type;
	st->m.policy = policy;
	dns_name_copy(p_name, st->p_name, NULL);
	st->m.prefix = prefix;
	st->m.result = result;
	SAVE(st->m.zone, *zonep);
	SAVE(st->m.db, *dbp);
	SAVE(st->m.node, *nodep);
	if (*rdatasetp != NULL && dns_rdataset_isassociated(*rdatasetp)) {
		/*
		 * Save the replacement rdataset from the policy
		 * and make the previous replacement rdataset scratch.
		 */
		SAVE(trdataset, st->m.rdataset);
		SAVE(st->m.rdataset, *rdatasetp);
		SAVE(*rdatasetp, trdataset);
		st->m.ttl = ISC_MIN(st->m.rdataset->ttl, rpz->max_policy_ttl);
	} else {
		st->m.ttl = ISC_MIN(DNS_RPZ_TTL_DEFAULT, rpz->max_policy_ttl);
	}
	SAVE(st->m.version, version);
}

#ifdef USE_DNSRPS
/*
 * Check the results of a RPZ service interface lookup.
 * Stop after an error (<0) or not a hit on a disabled zone (0).
 * Continue after a hit on a disabled zone (>0).
 */
static int
dnsrps_ck(librpz_emsg_t *emsg, ns_client_t *client, rpsdb_t *rpsdb,
	  isc_boolean_t recursed)
{
	isc_region_t region;
	librpz_domain_buf_t pname_buf;

	if (!librpz->rsp_result(emsg, &rpsdb->result, recursed, rpsdb->rsp)) {
		return (-1);
	}

	/*
	 * Forget the state from before the IP address or domain check
	 * if the lookup hit nothing.
	 */
	if (rpsdb->result.policy == LIBRPZ_POLICY_UNDEFINED ||
	    rpsdb->result.hit_id != rpsdb->hit_id ||
	    rpsdb->result.policy != LIBRPZ_POLICY_DISABLED)
	{
		if (!librpz->rsp_pop_discard(emsg, rpsdb->rsp)) {
			return (-1);
		}
		return (0);
	}

	/*
	 * Log a hit on a disabled zone.
	 * Forget the zone to not try it again, and restore the pre-hit state.
	 */
	if (!librpz->rsp_domain(emsg, &pname_buf, rpsdb->rsp)) {
		return (-1);
	}
	region.base = pname_buf.d;
	region.length = pname_buf.size;
	dns_name_fromregion(client->query.rpz_st->p_name, &region);
	rpz_log_rewrite(client, ISC_TRUE,
			dns_dnsrps_2policy(rpsdb->result.zpolicy),
			dns_dnsrps_trig2type(rpsdb->result.trig), NULL,
			client->query.rpz_st->p_name, NULL,
			rpsdb->result.cznum);

	if (!librpz->rsp_forget_zone(emsg, rpsdb->result.cznum, rpsdb->rsp) ||
	    !librpz->rsp_pop(emsg, &rpsdb->result, rpsdb->rsp))
	{
		return (-1);
	}
	return (1);
}

/*
 * Ready the shim database and rdataset for a DNSRPS hit.
 */
static isc_boolean_t
dnsrps_set_p(librpz_emsg_t *emsg, ns_client_t *client, dns_rpz_st_t *st,
	     dns_rdatatype_t qtype, dns_rdataset_t **p_rdatasetp,
	     isc_boolean_t recursed)
{
	rpsdb_t *rpsdb;
	librpz_domain_buf_t pname_buf;
	isc_region_t region;
	dns_zone_t *p_zone;
	dns_db_t *p_db;
	dns_dbnode_t *p_node;
	dns_rpz_policy_t policy;
	dns_fixedname_t foundf;
	dns_name_t *found;
	dns_rdatatype_t foundtype, searchtype;
	isc_result_t result;

	rpsdb = (rpsdb_t *)st->rpsdb;

	if (!librpz->rsp_result(emsg, &rpsdb->result, recursed, rpsdb->rsp)) {
		return (ISC_FALSE);
	}

	if (rpsdb->result.policy == LIBRPZ_POLICY_UNDEFINED) {
		return (ISC_TRUE);
	}

	/*
	 * Give the fake or shim DNSRPS database its new origin.
	 */
	if (!librpz->rsp_soa(emsg, NULL, NULL, &rpsdb->origin_buf,
			     &rpsdb->result, rpsdb->rsp))
	{
		return (ISC_FALSE);
	}
	region.base = rpsdb->origin_buf.d;
	region.length = rpsdb->origin_buf.size;
	dns_name_fromregion(&rpsdb->common.origin, &region);

	if (!librpz->rsp_domain(emsg, &pname_buf, rpsdb->rsp)) {
		return (ISC_FALSE);
	}
	region.base = pname_buf.d;
	region.length = pname_buf.size;
	dns_name_fromregion(st->p_name, &region);

	p_zone = NULL;
	p_db = NULL;
	p_node = NULL;
	rpz_ready(client, p_rdatasetp);
	dns_db_attach(st->rpsdb, &p_db);
	policy = dns_dnsrps_2policy(rpsdb->result.policy);
	if (policy != DNS_RPZ_POLICY_RECORD) {
		result = ISC_R_SUCCESS;
	} else if (qtype == dns_rdatatype_rrsig) {
		/*
		 * dns_find_db() refuses to look for and fail to
		 * find dns_rdatatype_rrsig.
		 */
		result = DNS_R_NXRRSET;
		policy = DNS_RPZ_POLICY_NODATA;
	} else {
		/*
		 * Get the next (and so first) RR from the policy node.
		 * If it is a CNAME, then look for it regardless of the
		 * query type.
		 */
		if (!librpz->rsp_rr(emsg, &foundtype, NULL, NULL, NULL,
				    &rpsdb->result, rpsdb->qname->ndata,
				    rpsdb->qname->length, rpsdb->rsp))
		{
			return (ISC_FALSE);
		}
		if (foundtype == dns_rdatatype_cname) {
			searchtype = dns_rdatatype_cname;
		} else {
			searchtype = qtype;
		}
		/*
		 * Get the DNSPRS imitation rdataset.
		 */
		found = dns_fixedname_initname(&foundf);
		result = dns_db_find(p_db, st->p_name, NULL, searchtype,
				     0, 0, &p_node, found, *p_rdatasetp, NULL);

		if (result == ISC_R_SUCCESS) {
			if (searchtype == dns_rdatatype_cname &&
			    qtype != dns_rdatatype_cname)
			{
				result = DNS_R_CNAME;
			}
		} else if (result == DNS_R_NXRRSET) {
			policy = DNS_RPZ_POLICY_NODATA;
		} else {
			snprintf(emsg->c, sizeof(emsg->c), "dns_db_find(): %s",
				 isc_result_totext(result));
			return (ISC_FALSE);
		}
	}

	rpz_save_p(st, client->view->rpzs->zones[rpsdb->result.cznum],
		   dns_dnsrps_trig2type(rpsdb->result.trig), policy,
		   st->p_name, 0, result, &p_zone, &p_db, &p_node,
		   p_rdatasetp, NULL);

	rpz_clean(NULL, NULL, NULL, p_rdatasetp);

	return (ISC_TRUE);
}

static isc_result_t
dnsrps_rewrite_ip(ns_client_t *client, const isc_netaddr_t *netaddr,
		  dns_rpz_type_t rpz_type, dns_rdataset_t **p_rdatasetp)
{
	dns_rpz_st_t *st;
	rpsdb_t *rpsdb;
	librpz_trig_t trig = LIBRPZ_TRIG_CLIENT_IP;
	isc_boolean_t recursed = ISC_FALSE;
	int res;
	librpz_emsg_t emsg;
	isc_result_t result;

	st = client->query.rpz_st;
	rpsdb = (rpsdb_t *)st->rpsdb;

	result = rpz_ready(client, p_rdatasetp);
	if (result != ISC_R_SUCCESS) {
		st->m.policy = DNS_RPZ_POLICY_ERROR;
		return (result);
	}

	switch (rpz_type) {
	case DNS_RPZ_TYPE_CLIENT_IP:
		trig = LIBRPZ_TRIG_CLIENT_IP;
		recursed = ISC_FALSE;
		break;
	case DNS_RPZ_TYPE_IP:
		trig = LIBRPZ_TRIG_IP;
		recursed = ISC_TRUE;
		break;
	case DNS_RPZ_TYPE_NSIP:
		trig = LIBRPZ_TRIG_NSIP;
		recursed = ISC_TRUE;
		break;
	default:
		INSIST(0);
	}

	do {
		if (!librpz->rsp_push(&emsg, rpsdb->rsp) ||
		    !librpz->ck_ip(&emsg,
				   netaddr->family == AF_INET
				   ? (const void *)&netaddr->type.in
				   : (const void *)&netaddr->type.in6,
				   netaddr->family, trig, ++rpsdb->hit_id,
				   recursed, rpsdb->rsp) ||
		    (res = dnsrps_ck(&emsg, client, rpsdb, recursed)) < 0)
		{
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, NULL,
				     rpz_type, emsg.c, DNS_R_SERVFAIL);
			st->m.policy = DNS_RPZ_POLICY_ERROR;
			return (DNS_R_SERVFAIL);
		}
	} while (res != 0);
	return (ISC_R_SUCCESS);
}

static isc_result_t
dnsrps_rewrite_name(ns_client_t *client, dns_name_t *trig_name,
		    isc_boolean_t recursed, dns_rpz_type_t rpz_type,
		    dns_rdataset_t **p_rdatasetp)
{
	dns_rpz_st_t *st;
	rpsdb_t *rpsdb;
	librpz_trig_t trig = LIBRPZ_TRIG_CLIENT_IP;
	isc_region_t r;
	int res;
	librpz_emsg_t emsg;
	isc_result_t result;

	st = client->query.rpz_st;
	rpsdb = (rpsdb_t *)st->rpsdb;

	result = rpz_ready(client, p_rdatasetp);
	if (result != ISC_R_SUCCESS) {
		st->m.policy = DNS_RPZ_POLICY_ERROR;
		return (result);
	}

	switch (rpz_type) {
	case DNS_RPZ_TYPE_QNAME:
		trig = LIBRPZ_TRIG_QNAME;
		break;
	case DNS_RPZ_TYPE_NSDNAME:
		trig = LIBRPZ_TRIG_NSDNAME;
		break;
	default:
		INSIST(0);
	}

	dns_name_toregion(trig_name, &r);
	do {
		if (!librpz->rsp_push(&emsg, rpsdb->rsp) ||
		    !librpz->ck_domain(&emsg, r.base, r.length,
				       trig, ++rpsdb->hit_id,
				       recursed, rpsdb->rsp) ||
		    (res = dnsrps_ck(&emsg, client, rpsdb, recursed)) < 0)
		{
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, NULL,
				     rpz_type, emsg.c, DNS_R_SERVFAIL);
			st->m.policy = DNS_RPZ_POLICY_ERROR;
			return (DNS_R_SERVFAIL);
		}
	} while (res != 0);
	return (ISC_R_SUCCESS);
}
#endif /* USE_DNSRPS */

/*
 * Check this address in every eligible policy zone.
 */
static isc_result_t
rpz_rewrite_ip(ns_client_t *client, const isc_netaddr_t *netaddr,
	       dns_rdatatype_t qtype, dns_rpz_type_t rpz_type,
	       dns_rpz_zbits_t zbits, dns_rdataset_t **p_rdatasetp)
{
	dns_rpz_zones_t *rpzs;
	dns_rpz_st_t *st;
	dns_rpz_zone_t *rpz;
	dns_rpz_prefix_t prefix;
	dns_rpz_num_t rpz_num;
	dns_fixedname_t ip_namef, p_namef;
	dns_name_t *ip_name, *p_name;
	dns_zone_t *p_zone;
	dns_db_t *p_db;
	dns_dbversion_t *p_version;
	dns_dbnode_t *p_node;
	dns_rpz_policy_t policy;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_ip");

	rpzs = client->view->rpzs;
	st = client->query.rpz_st;
#ifdef USE_DNSRPS
	if (st->popt.dnsrps_enabled) {
		return (dnsrps_rewrite_ip(client, netaddr, rpz_type,
					 p_rdatasetp));
	}
#endif

	ip_name = dns_fixedname_initname(&ip_namef);

	p_zone = NULL;
	p_db = NULL;
	p_node = NULL;

	while (zbits != 0) {
		rpz_num = dns_rpz_find_ip(rpzs, rpz_type, zbits, netaddr,
					  ip_name, &prefix);
		if (rpz_num == DNS_RPZ_INVALID_NUM)
			break;
		zbits &= (DNS_RPZ_ZMASK(rpz_num) >> 1);

		/*
		 * Do not try applying policy zones that cannot replace a
		 * previously found policy zone.
		 * Stop looking if the next best choice cannot
		 * replace what we already have.
		 */
		rpz = rpzs->zones[rpz_num];
		if (st->m.policy != DNS_RPZ_POLICY_MISS) {
			if (st->m.rpz->num < rpz->num)
				break;
			if (st->m.rpz->num == rpz->num &&
			    (st->m.type < rpz_type ||
			     st->m.prefix > prefix))
				break;
		}

		/*
		 * Get the policy for a prefix at least as long
		 * as the prefix of the entry we had before.
		 */
		p_name = dns_fixedname_initname(&p_namef);
		result = rpz_get_p_name(client, p_name, rpz, rpz_type, ip_name);
		if (result != ISC_R_SUCCESS)
			continue;
		result = rpz_find_p(client, ip_name, qtype,
				    p_name, rpz, rpz_type,
				    &p_zone, &p_db, &p_version, &p_node,
				    p_rdatasetp, &policy);
		switch (result) {
		case DNS_R_NXDOMAIN:
			/*
			 * Continue after a policy record that is missing
			 * contrary to the summary data.  The summary
			 * data can out of date during races with and among
			 * policy zone updates.
			 */
			CTRACE(ISC_LOG_ERROR,
			       "rpz_rewrite_ip: mismatched summary data; "
			       "continuing");
			continue;
		case DNS_R_SERVFAIL:
			rpz_clean(&p_zone, &p_db, &p_node, p_rdatasetp);
			st->m.policy = DNS_RPZ_POLICY_ERROR;
			return (DNS_R_SERVFAIL);
		default:
			/*
			 * Forget this policy if it is not preferable
			 * to the previously found policy.
			 * If this policy is not good, then stop looking
			 * because none of the later policy zones would work.
			 *
			 * With more than one applicable policy, prefer
			 * the earliest configured policy,
			 * client-IP over QNAME over IP over NSDNAME over NSIP,
			 * the longest prefix
			 * the lexically smallest address.
			 * dns_rpz_find_ip() ensures st->m.rpz->num >= rpz->num.
			 * We can compare new and current p_name because
			 * both are of the same type and in the same zone.
			 * The tests above eliminate other reasons to
			 * reject this policy.  If this policy can't work,
			 * then neither can later zones.
			 */
			if (st->m.policy != DNS_RPZ_POLICY_MISS &&
			    rpz->num == st->m.rpz->num &&
			     (st->m.type == rpz_type &&
			      st->m.prefix == prefix &&
			      0 > dns_name_rdatacompare(st->p_name, p_name)))
				break;

			/*
			 * Stop checking after saving an enabled hit in this
			 * policy zone.  The radix tree in the policy zone
			 * ensures that we found the longest match.
			 */
			if (rpz->policy != DNS_RPZ_POLICY_DISABLED) {
				CTRACE(ISC_LOG_DEBUG(3),
				       "rpz_rewrite_ip: rpz_save_p");
				rpz_save_p(st, rpz, rpz_type,
					   policy, p_name, prefix, result,
					   &p_zone, &p_db, &p_node,
					   p_rdatasetp, p_version);
				break;
			}

			/*
			 * Log DNS_RPZ_POLICY_DISABLED zones
			 * and try the next eligible policy zone.
			 */
			rpz_log_rewrite(client, ISC_TRUE, policy, rpz_type,
					p_zone, p_name, NULL, rpz_num);
		}
	}

	rpz_clean(&p_zone, &p_db, &p_node, p_rdatasetp);
	return (ISC_R_SUCCESS);
}

/*
 * Check the IP addresses in the A or AAAA rrsets for name against
 * all eligible rpz_type (IP or NSIP) response policy rewrite rules.
 */
static isc_result_t
rpz_rewrite_ip_rrset(ns_client_t *client,
		     dns_name_t *name, dns_rdatatype_t qtype,
		     dns_rpz_type_t rpz_type, dns_rdatatype_t ip_type,
		     dns_db_t **ip_dbp, dns_dbversion_t *ip_version,
		     dns_rdataset_t **ip_rdatasetp,
		     dns_rdataset_t **p_rdatasetp, isc_boolean_t resuming)
{
	dns_rpz_zbits_t zbits;
	isc_netaddr_t netaddr;
	struct in_addr ina;
	struct in6_addr in6a;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_ip_rrset");

	zbits = rpz_get_zbits(client, ip_type, rpz_type);
	if (zbits == 0)
		return (ISC_R_SUCCESS);

	/*
	 * Get the A or AAAA rdataset.
	 */
	result = rpz_rrset_find(client, name, ip_type, rpz_type, ip_dbp,
				ip_version, ip_rdatasetp, resuming);
	switch (result) {
	case ISC_R_SUCCESS:
	case DNS_R_GLUE:
	case DNS_R_ZONECUT:
		break;
	case DNS_R_EMPTYNAME:
	case DNS_R_EMPTYWILD:
	case DNS_R_NXDOMAIN:
	case DNS_R_NCACHENXDOMAIN:
	case DNS_R_NXRRSET:
	case DNS_R_NCACHENXRRSET:
	case ISC_R_NOTFOUND:
		return (ISC_R_SUCCESS);
	case DNS_R_DELEGATION:
	case DNS_R_DUPLICATE:
	case DNS_R_DROP:
		return (result);
	case DNS_R_CNAME:
	case DNS_R_DNAME:
		rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL1, name, rpz_type,
			     " NS address rewrite rrset", result);
		return (ISC_R_SUCCESS);
	default:
		if (client->query.rpz_st->m.policy != DNS_RPZ_POLICY_ERROR) {
			client->query.rpz_st->m.policy = DNS_RPZ_POLICY_ERROR;
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, name,
				     rpz_type, " NS address rewrite rrset",
				     result);
		}
		CTRACE(ISC_LOG_ERROR,
		       "rpz_rewrite_ip_rrset: unexpected result");
		return (DNS_R_SERVFAIL);
	}

	/*
	 * Check all of the IP addresses in the rdataset.
	 */
	for (result = dns_rdataset_first(*ip_rdatasetp);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(*ip_rdatasetp)) {

		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdataset_current(*ip_rdatasetp, &rdata);
		switch (rdata.type) {
		case dns_rdatatype_a:
			INSIST(rdata.length == 4);
			memmove(&ina.s_addr, rdata.data, 4);
			isc_netaddr_fromin(&netaddr, &ina);
			break;
		case dns_rdatatype_aaaa:
			INSIST(rdata.length == 16);
			memmove(in6a.s6_addr, rdata.data, 16);
			isc_netaddr_fromin6(&netaddr, &in6a);
			break;
		default:
			continue;
		}

		result = rpz_rewrite_ip(client, &netaddr, qtype, rpz_type,
					zbits, p_rdatasetp);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	return (ISC_R_SUCCESS);
}

/*
 * Look for IP addresses in A and AAAA rdatasets
 * that trigger all eligible IP or NSIP policy rules.
 */
static isc_result_t
rpz_rewrite_ip_rrsets(ns_client_t *client, dns_name_t *name,
		      dns_rdatatype_t qtype, dns_rpz_type_t rpz_type,
		      dns_rdataset_t **ip_rdatasetp, isc_boolean_t resuming)
{
	dns_rpz_st_t *st;
	dns_dbversion_t *ip_version;
	dns_db_t *ip_db;
	dns_rdataset_t *p_rdataset;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_ip_rrsets");

	st = client->query.rpz_st;
	ip_version = NULL;
	ip_db = NULL;
	p_rdataset = NULL;
	if ((st->state & DNS_RPZ_DONE_IPv4) == 0 &&
	    (qtype == dns_rdatatype_a ||
	     qtype == dns_rdatatype_any ||
	     rpz_type == DNS_RPZ_TYPE_NSIP)) {
		/*
		 * Rewrite based on an IPv4 address that will appear
		 * in the ANSWER section or if we are checking IP addresses.
		 */
		result = rpz_rewrite_ip_rrset(client, name, qtype,
					      rpz_type, dns_rdatatype_a,
					      &ip_db, ip_version, ip_rdatasetp,
					      &p_rdataset, resuming);
		if (result == ISC_R_SUCCESS)
			st->state |= DNS_RPZ_DONE_IPv4;
	} else {
		result = ISC_R_SUCCESS;
	}
	if (result == ISC_R_SUCCESS &&
	    (qtype == dns_rdatatype_aaaa ||
	     qtype == dns_rdatatype_any ||
	     rpz_type == DNS_RPZ_TYPE_NSIP)) {
		/*
		 * Rewrite based on IPv6 addresses that will appear
		 * in the ANSWER section or if we are checking IP addresses.
		 */
		result = rpz_rewrite_ip_rrset(client, name,  qtype,
					      rpz_type, dns_rdatatype_aaaa,
					      &ip_db, ip_version, ip_rdatasetp,
					      &p_rdataset, resuming);
	}
	if (ip_db != NULL)
		dns_db_detach(&ip_db);
	query_putrdataset(client, &p_rdataset);
	return (result);
}

/*
 * Try to rewrite a request for a qtype rdataset based on the trigger name
 * trig_name and rpz_type (DNS_RPZ_TYPE_QNAME or DNS_RPZ_TYPE_NSDNAME).
 * Record the results including the replacement rdataset if any
 * in client->query.rpz_st.
 * *rdatasetp is a scratch rdataset.
 */
static isc_result_t
rpz_rewrite_name(ns_client_t *client, dns_name_t *trig_name,
		 dns_rdatatype_t qtype, dns_rpz_type_t rpz_type,
		 dns_rpz_zbits_t allowed_zbits, isc_boolean_t recursed,
		 dns_rdataset_t **rdatasetp)
{
	dns_rpz_zones_t *rpzs;
	dns_rpz_zone_t *rpz;
	dns_rpz_st_t *st;
	dns_fixedname_t p_namef;
	dns_name_t *p_name;
	dns_rpz_zbits_t zbits;
	dns_rpz_num_t rpz_num;
	dns_zone_t *p_zone;
	dns_db_t *p_db;
	dns_dbversion_t *p_version;
	dns_dbnode_t *p_node;
	dns_rpz_policy_t policy;
	isc_result_t result;

#ifndef USE_DNSRPS
	UNUSED(recursed);
#endif

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_name");

	rpzs = client->view->rpzs;
	st = client->query.rpz_st;

#ifdef USE_DNSRPS
	if (st->popt.dnsrps_enabled) {
		return (dnsrps_rewrite_name(client, trig_name, recursed,
					     rpz_type, rdatasetp));
	}
#endif

	zbits = rpz_get_zbits(client, qtype, rpz_type);
	zbits &= allowed_zbits;
	if (zbits == 0)
		return (ISC_R_SUCCESS);

	/*
	 * Use the summary database to find the bit mask of policy zones
	 * with policies for this trigger name. We do this even if there
	 * is only one eligible policy zone so that wildcard triggers
	 * are matched correctly, and not into their parent.
	 */
	zbits = dns_rpz_find_name(rpzs, rpz_type, zbits, trig_name);
	if (zbits == 0)
		return (ISC_R_SUCCESS);

	p_name = dns_fixedname_initname(&p_namef);

	p_zone = NULL;
	p_db = NULL;
	p_node = NULL;

	/*
	 * Check the trigger name in every policy zone that the summary data
	 * says has a hit for the trigger name.
	 * Most of the time there are no eligible zones and the summary data
	 * keeps us from getting this far.
	 * We check the most eligible zone first and so usually check only
	 * one policy zone.
	 */
	for (rpz_num = 0; zbits != 0; ++rpz_num, zbits >>= 1) {
		if ((zbits & 1) == 0)
			continue;

		/*
		 * Do not check policy zones that cannot replace a previously
		 * found policy.
		 */
		rpz = rpzs->zones[rpz_num];
		if (st->m.policy != DNS_RPZ_POLICY_MISS) {
			if (st->m.rpz->num < rpz->num)
				break;
			if (st->m.rpz->num == rpz->num &&
			    st->m.type < rpz_type)
				break;
		}

		/*
		 * Get the next policy zone's record for this trigger name.
		 */
		result = rpz_get_p_name(client, p_name, rpz, rpz_type,
					trig_name);
		if (result != ISC_R_SUCCESS)
			continue;
		result = rpz_find_p(client, trig_name, qtype, p_name,
				    rpz, rpz_type,
				    &p_zone, &p_db, &p_version, &p_node,
				    rdatasetp, &policy);
		switch (result) {
		case DNS_R_NXDOMAIN:
			/*
			 * Continue after a missing policy record
			 * contrary to the summary data.  The summary
			 * data can out of date during races with and among
			 * policy zone updates.
			 */
			CTRACE(ISC_LOG_ERROR,
			       "rpz_rewrite_name: mismatched summary data; "
			       "continuing");
			continue;
		case DNS_R_SERVFAIL:
			rpz_clean(&p_zone, &p_db, &p_node, rdatasetp);
			st->m.policy = DNS_RPZ_POLICY_ERROR;
			return (DNS_R_SERVFAIL);
		default:
			/*
			 * With more than one applicable policy, prefer
			 * the earliest configured policy,
			 * client-IP over QNAME over IP over NSDNAME over NSIP,
			 * and the smallest name.
			 * We known st->m.rpz->num >= rpz->num  and either
			 * st->m.rpz->num > rpz->num or st->m.type >= rpz_type
			 */
			if (st->m.policy != DNS_RPZ_POLICY_MISS &&
			    rpz->num == st->m.rpz->num &&
			    (st->m.type < rpz_type ||
			     (st->m.type == rpz_type &&
			      0 >= dns_name_compare(p_name, st->p_name))))
				continue;
#if 0
			/*
			 * This code would block a customer reported information
			 * leak of rpz rules by rewriting requests in the
			 * rpz-ip, rpz-nsip, rpz-nsdname,and rpz-passthru TLDs.
			 * Without this code, a bad guy could request
			 * 24.0.3.2.10.rpz-ip. to find the policy rule for
			 * 10.2.3.0/14.  It is an insignificant leak and this
			 * code is not worth its cost, because the bad guy
			 * could publish "evil.com A 10.2.3.4" and request
			 * evil.com to get the same information.
			 * Keep code with "#if 0" in case customer demand
			 * is irresistible.
			 *
			 * We have the less frequent case of a triggered
			 * policy.  Check that we have not trigger on one
			 * of the pretend RPZ TLDs.
			 * This test would make it impossible to rewrite
			 * names in TLDs that start with "rpz-" should
			 * ICANN ever allow such TLDs.
			 */
			unsigned int labels;
			labels = dns_name_countlabels(trig_name);
			if (labels >= 2) {
				dns_label_t label;

				dns_name_getlabel(trig_name, labels-2, &label);
				if (label.length >= sizeof(DNS_RPZ_PREFIX)-1 &&
				    strncasecmp((const char *)label.base+1,
						DNS_RPZ_PREFIX,
						sizeof(DNS_RPZ_PREFIX)-1) == 0)
					continue;
			}
#endif
			if (rpz->policy != DNS_RPZ_POLICY_DISABLED) {
				CTRACE(ISC_LOG_DEBUG(3),
				       "rpz_rewrite_name: rpz_save_p");
				rpz_save_p(st, rpz, rpz_type,
					   policy, p_name, 0, result,
					   &p_zone, &p_db, &p_node,
					   rdatasetp, p_version);
				/*
				 * After a hit, higher numbered policy zones
				 * are irrelevant
				 */
				rpz_clean(&p_zone, &p_db, &p_node, rdatasetp);
				return (ISC_R_SUCCESS);
			}
			/*
			 * Log DNS_RPZ_POLICY_DISABLED zones
			 * and try the next eligible policy zone.
			 */
			rpz_log_rewrite(client, ISC_TRUE, policy, rpz_type,
					p_zone, p_name, NULL, rpz_num);
			break;
		}
	}

	rpz_clean(&p_zone, &p_db, &p_node, rdatasetp);
	return (ISC_R_SUCCESS);
}

static void
rpz_rewrite_ns_skip(ns_client_t *client, dns_name_t *nsname,
		    isc_result_t result, int level, const char *str)
{
	dns_rpz_st_t *st;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_ns_skip");

	st = client->query.rpz_st;

	if (str != NULL)
		rpz_log_fail_helper(client, level, nsname,
				    DNS_RPZ_TYPE_NSIP, DNS_RPZ_TYPE_NSDNAME,
				    str, result);
	if (st->r.ns_rdataset != NULL &&
	    dns_rdataset_isassociated(st->r.ns_rdataset))
		dns_rdataset_disassociate(st->r.ns_rdataset);

	st->r.label--;
}

/*
 * RPZ query result types
 */
typedef enum {
	qresult_type_done = 0,
	qresult_type_restart = 1,
	qresult_type_recurse = 2
} qresult_type_t;

/*
 * Look for response policy zone QNAME, NSIP, and NSDNAME rewriting.
 */
static isc_result_t
rpz_rewrite(ns_client_t *client, dns_rdatatype_t qtype,
	    isc_result_t qresult, isc_boolean_t resuming,
	    dns_rdataset_t *ordataset, dns_rdataset_t *osigset)
{
	dns_rpz_zones_t *rpzs;
	dns_rpz_st_t *st;
	dns_rdataset_t *rdataset;
	dns_fixedname_t nsnamef;
	dns_name_t *nsname;
	qresult_type_t qresult_type;
	dns_rpz_zbits_t zbits;
	isc_result_t result = ISC_R_SUCCESS;
	dns_rpz_have_t have;
	dns_rpz_popt_t popt;
	int rpz_ver;
#ifdef USE_DNSRPS
	librpz_emsg_t emsg;
#endif

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite");

	rpzs = client->view->rpzs;
	st = client->query.rpz_st;

	if (rpzs == NULL ||
	    (st != NULL && (st->state & DNS_RPZ_REWRITTEN) != 0))
		return (DNS_R_DISALLOWED);

	RWLOCK(&rpzs->search_lock, isc_rwlocktype_read);
	if ((rpzs->p.num_zones == 0 && !rpzs->p.dnsrps_enabled) ||
	    (!RECURSIONOK(client) && rpzs->p.no_rd_ok == 0) ||
	    !rpz_ck_dnssec(client, qresult, ordataset, osigset))
	{
		RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_read);
		return (DNS_R_DISALLOWED);
	}
	have = rpzs->have;
	popt = rpzs->p;
	rpz_ver = rpzs->rpz_ver;
	RWUNLOCK(&rpzs->search_lock, isc_rwlocktype_read);

#ifndef USE_DNSRPS
	INSIST(!popt.dnsrps_enabled);
#endif

	if (st == NULL) {
		st = isc_mem_get(client->mctx, sizeof(*st));
		if (st == NULL)
			return (ISC_R_NOMEMORY);
		st->state = 0;
		st->rpsdb = NULL;
	}
	if (st->state == 0) {
		st->state |= DNS_RPZ_ACTIVE;
		memset(&st->m, 0, sizeof(st->m));
		st->m.type = DNS_RPZ_TYPE_BAD;
		st->m.policy = DNS_RPZ_POLICY_MISS;
		st->m.ttl = ~0;
		memset(&st->r, 0, sizeof(st->r));
		memset(&st->q, 0, sizeof(st->q));
		st->p_name = dns_fixedname_initname(&st->_p_namef);
		st->r_name = dns_fixedname_initname(&st->_r_namef);
		st->fname = dns_fixedname_initname(&st->_fnamef);
		st->have = have;
		st->popt = popt;
		st->rpz_ver = rpz_ver;
		client->query.rpz_st = st;
#ifdef USE_DNSRPS
		if (popt.dnsrps_enabled) {
			if (st->rpsdb != NULL) {
				dns_db_detach(&st->rpsdb);
			}
			result = dns_dnsrps_rewrite_init(&emsg, st, rpzs,
							  client->query.qname,
							  client->mctx,
							  RECURSIONOK(client));
			if (result != ISC_R_SUCCESS) {
				rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL,
					     NULL, DNS_RPZ_TYPE_QNAME,
					     emsg.c, result);
				st->m.policy = DNS_RPZ_POLICY_ERROR;
				return (ISC_R_SUCCESS);
			}
		}
#endif
	}

	/*
	 * There is nothing to rewrite if the main query failed.
	 */
	switch (qresult) {
	case ISC_R_SUCCESS:
	case DNS_R_GLUE:
	case DNS_R_ZONECUT:
		qresult_type = qresult_type_done;
		break;
	case DNS_R_EMPTYNAME:
	case DNS_R_NXRRSET:
	case DNS_R_NXDOMAIN:
	case DNS_R_EMPTYWILD:
	case DNS_R_NCACHENXDOMAIN:
	case DNS_R_NCACHENXRRSET:
	case DNS_R_CNAME:
	case DNS_R_DNAME:
		qresult_type = qresult_type_restart;
		break;
	case DNS_R_DELEGATION:
	case ISC_R_NOTFOUND:
		/*
		 * If recursion is on, do only tentative rewriting.
		 * If recursion is off, this the normal and only time we
		 * can rewrite.
		 */
		if (RECURSIONOK(client))
			qresult_type = qresult_type_recurse;
		else
			qresult_type = qresult_type_restart;
		break;
	case ISC_R_FAILURE:
	case ISC_R_TIMEDOUT:
	case DNS_R_BROKENCHAIN:
		rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL3, NULL,
			     DNS_RPZ_TYPE_QNAME,
			     " stop on qresult in rpz_rewrite()", qresult);
		return (ISC_R_SUCCESS);
	default:
		rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL1, NULL,
			     DNS_RPZ_TYPE_QNAME,
			     " stop on unrecognized qresult in rpz_rewrite()",
			     qresult);
		return (ISC_R_SUCCESS);
	}

	rdataset = NULL;

	if ((st->state & (DNS_RPZ_DONE_CLIENT_IP | DNS_RPZ_DONE_QNAME)) !=
	    (DNS_RPZ_DONE_CLIENT_IP | DNS_RPZ_DONE_QNAME))
	{
		isc_netaddr_t netaddr;
		dns_rpz_zbits_t allowed;

		if (!st->popt.dnsrps_enabled &&
		    qresult_type == qresult_type_recurse)
		{
			/*
			 * This request needs recursion that has not been done.
			 * Get bits for the policy zones that do not need
			 * to wait for the results of recursion.
			 */
			allowed = st->have.qname_skip_recurse;
			if (allowed == 0) {
				return (ISC_R_SUCCESS);
			}
		} else {
			allowed = DNS_RPZ_ALL_ZBITS;
		}

		/*
		 * Check once for triggers for the client IP address.
		 */
		if ((st->state & DNS_RPZ_DONE_CLIENT_IP) == 0) {
			zbits = rpz_get_zbits(client, dns_rdatatype_none,
					      DNS_RPZ_TYPE_CLIENT_IP);
			zbits &= allowed;
			if (zbits != 0) {
				isc_netaddr_fromsockaddr(&netaddr,
							 &client->peeraddr);
				result = rpz_rewrite_ip(client, &netaddr, qtype,
							DNS_RPZ_TYPE_CLIENT_IP,
							zbits, &rdataset);
				if (result != ISC_R_SUCCESS)
					goto cleanup;
			}
		}

		/*
		 * Check triggers for the query name if this is the first time
		 * for the current qname.
		 * There is a first time for each name in a CNAME chain
		 */
		if ((st->state & DNS_RPZ_DONE_QNAME) == 0) {
			isc_boolean_t norec =
				ISC_TF(qresult_type != qresult_type_recurse);
			result = rpz_rewrite_name(client, client->query.qname,
						  qtype, DNS_RPZ_TYPE_QNAME,
						  allowed, norec,
						  &rdataset);
			if (result != ISC_R_SUCCESS)
				goto cleanup;

			/*
			 * Check IPv4 addresses in A RRs next.
			 * Reset to the start of the NS names.
			 */
			st->r.label = dns_name_countlabels(client->query.qname);
			st->state &= ~(DNS_RPZ_DONE_QNAME_IP |
				       DNS_RPZ_DONE_IPv4);

		}

		/*
		 * Quit if this was an attempt to find a qname or
		 * client-IP trigger before recursion.
		 * We will be back if no pre-recursion triggers hit.
		 * For example, consider 2 policy zones, both with qname and
		 * IP address triggers.  If the qname misses the 1st zone,
		 * then we cannot know whether a hit for the qname in the
		 * 2nd zone matters until after recursing to get the A RRs and
		 * testing them in the first zone.
		 * Do not bother saving the work from this attempt,
		 * because recusion is so slow.
		 */
		if (qresult_type == qresult_type_recurse)
			goto cleanup;

		/*
		 * DNS_RPZ_DONE_QNAME but not DNS_RPZ_DONE_CLIENT_IP
		 * is reset at the end of dealing with each CNAME.
		 */
		st->state |= (DNS_RPZ_DONE_CLIENT_IP | DNS_RPZ_DONE_QNAME);
	}

	/*
	 * Check known IP addresses for the query name if the database lookup
	 * resulted in some addresses (qresult_type == qresult_type_done)
	 * and if we have not already checked them.
	 * Any recursion required for the query has already happened.
	 * Do not check addresses that will not be in the ANSWER section.
	 */
	if ((st->state & DNS_RPZ_DONE_QNAME_IP) == 0 &&
	    qresult_type == qresult_type_done &&
	    rpz_get_zbits(client, qtype, DNS_RPZ_TYPE_IP) != 0) {
		result = rpz_rewrite_ip_rrsets(client,
					       client->query.qname, qtype,
					       DNS_RPZ_TYPE_IP,
					       &rdataset, resuming);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		/*
		 * We are finished checking the IP addresses for the qname.
		 * Start with IPv4 if we will check NS IP addesses.
		 */
		st->state |= DNS_RPZ_DONE_QNAME_IP;
		st->state &= ~DNS_RPZ_DONE_IPv4;
	}

	/*
	 * Stop looking for rules if there are none of the other kinds
	 * that could override what we already have.
	 */
	if (rpz_get_zbits(client, dns_rdatatype_any,
			  DNS_RPZ_TYPE_NSDNAME) == 0 &&
	    rpz_get_zbits(client, dns_rdatatype_any,
			  DNS_RPZ_TYPE_NSIP) == 0) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	dns_fixedname_init(&nsnamef);
	dns_name_clone(client->query.qname, dns_fixedname_name(&nsnamef));
	while (st->r.label > st->popt.min_ns_labels) {
		/*
		 * Get NS rrset for each domain in the current qname.
		 */
		if (st->r.label == dns_name_countlabels(client->query.qname)) {
			nsname = client->query.qname;
		} else {
			nsname = dns_fixedname_name(&nsnamef);
			dns_name_split(client->query.qname, st->r.label,
				       NULL, nsname);
		}
		if (st->r.ns_rdataset == NULL ||
		    !dns_rdataset_isassociated(st->r.ns_rdataset))
		{
			dns_db_t *db = NULL;
			result = rpz_rrset_find(client, nsname,
						dns_rdatatype_ns,
						DNS_RPZ_TYPE_NSDNAME,
						&db, NULL, &st->r.ns_rdataset,
						resuming);
			if (db != NULL)
				dns_db_detach(&db);
			if (st->m.policy == DNS_RPZ_POLICY_ERROR)
				goto cleanup;
			switch (result) {
			case ISC_R_SUCCESS:
				result = dns_rdataset_first(st->r.ns_rdataset);
				if (result != ISC_R_SUCCESS)
					goto cleanup;
				st->state &= ~(DNS_RPZ_DONE_NSDNAME |
					       DNS_RPZ_DONE_IPv4);
				break;
			case DNS_R_DELEGATION:
			case DNS_R_DUPLICATE:
			case DNS_R_DROP:
				goto cleanup;
			case DNS_R_EMPTYNAME:
			case DNS_R_NXRRSET:
			case DNS_R_EMPTYWILD:
			case DNS_R_NXDOMAIN:
			case DNS_R_NCACHENXDOMAIN:
			case DNS_R_NCACHENXRRSET:
			case ISC_R_NOTFOUND:
			case DNS_R_CNAME:
			case DNS_R_DNAME:
				rpz_rewrite_ns_skip(client, nsname, result,
						    0, NULL);
				continue;
			case ISC_R_TIMEDOUT:
			case DNS_R_BROKENCHAIN:
			case ISC_R_FAILURE:
				rpz_rewrite_ns_skip(client, nsname, result,
						DNS_RPZ_DEBUG_LEVEL3,
						" NS rpz_rrset_find()");
				continue;
			default:
				rpz_rewrite_ns_skip(client, nsname, result,
						DNS_RPZ_INFO_LEVEL,
						" unrecognized NS"
						" rpz_rrset_find()");
				continue;
			}
		}
		/*
		 * Check all NS names.
		 */
		do {
			dns_rdata_ns_t ns;
			dns_rdata_t nsrdata = DNS_RDATA_INIT;

			dns_rdataset_current(st->r.ns_rdataset, &nsrdata);
			result = dns_rdata_tostruct(&nsrdata, &ns, NULL);
			dns_rdata_reset(&nsrdata);
			if (result != ISC_R_SUCCESS) {
				rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL,
					     nsname, DNS_RPZ_TYPE_NSIP,
					     " rdata_tostruct()", result);
				st->m.policy = DNS_RPZ_POLICY_ERROR;
				goto cleanup;
			}
			/*
			 * Do nothing about "NS ."
			 */
			if (dns_name_equal(&ns.name, dns_rootname)) {
				dns_rdata_freestruct(&ns);
				result = dns_rdataset_next(st->r.ns_rdataset);
				continue;
			}
			/*
			 * Check this NS name if we did not handle it
			 * during a previous recursion.
			 */
			if ((st->state & DNS_RPZ_DONE_NSDNAME) == 0) {
				result = rpz_rewrite_name(client, &ns.name,
							qtype,
							DNS_RPZ_TYPE_NSDNAME,
							DNS_RPZ_ALL_ZBITS,
							ISC_TRUE,
							&rdataset);
				if (result != ISC_R_SUCCESS) {
					dns_rdata_freestruct(&ns);
					goto cleanup;
				}
				st->state |= DNS_RPZ_DONE_NSDNAME;
			}
			/*
			 * Check all IP addresses for this NS name.
			 */
			result = rpz_rewrite_ip_rrsets(client, &ns.name, qtype,
						       DNS_RPZ_TYPE_NSIP,
						       &rdataset, resuming);
			dns_rdata_freestruct(&ns);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
			st->state &= ~(DNS_RPZ_DONE_NSDNAME |
				       DNS_RPZ_DONE_IPv4);
			result = dns_rdataset_next(st->r.ns_rdataset);
		} while (result == ISC_R_SUCCESS);
		dns_rdataset_disassociate(st->r.ns_rdataset);
		st->r.label--;

		if (rpz_get_zbits(client, dns_rdatatype_any,
				  DNS_RPZ_TYPE_NSDNAME) == 0 &&
		    rpz_get_zbits(client, dns_rdatatype_any,
				  DNS_RPZ_TYPE_NSIP) == 0)
			break;
	}

	/*
	 * Use the best hit, if any.
	 */
	result = ISC_R_SUCCESS;

cleanup:
#ifdef USE_DNSRPS
	if (st->popt.dnsrps_enabled &&
	    st->m.policy != DNS_RPZ_POLICY_ERROR &&
	    !dnsrps_set_p(&emsg, client, st, qtype, &rdataset,
			  ISC_TF(qresult_type != qresult_type_recurse)))
	{
		rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, NULL,
			     DNS_RPZ_TYPE_BAD, emsg.c, DNS_R_SERVFAIL);
		st->m.policy = DNS_RPZ_POLICY_ERROR;
	}
#endif
	if (st->m.policy != DNS_RPZ_POLICY_MISS &&
	    st->m.policy != DNS_RPZ_POLICY_ERROR &&
	    st->m.rpz->policy != DNS_RPZ_POLICY_GIVEN)
		st->m.policy = st->m.rpz->policy;
	if (st->m.policy == DNS_RPZ_POLICY_MISS ||
	    st->m.policy == DNS_RPZ_POLICY_PASSTHRU ||
	    st->m.policy == DNS_RPZ_POLICY_ERROR) {
		if (st->m.policy == DNS_RPZ_POLICY_PASSTHRU &&
		    result != DNS_R_DELEGATION)
			rpz_log_rewrite(client, ISC_FALSE, st->m.policy,
					st->m.type, st->m.zone, st->p_name,
					NULL, st->m.rpz->num);
		rpz_match_clear(st);
	}
	if (st->m.policy == DNS_RPZ_POLICY_ERROR) {
		CTRACE(ISC_LOG_ERROR, "SERVFAIL due to RPZ policy");
		st->m.type = DNS_RPZ_TYPE_BAD;
		result = DNS_R_SERVFAIL;
	}
	query_putrdataset(client, &rdataset);
	if ((st->state & DNS_RPZ_RECURSING) == 0)
		rpz_clean(NULL, &st->r.db, NULL, &st->r.ns_rdataset);

	return (result);
}

/*
 * See if response policy zone rewriting is allowed by a lack of interest
 * by the client in DNSSEC or a lack of signatures.
 */
static isc_boolean_t
rpz_ck_dnssec(ns_client_t *client, isc_result_t qresult,
	      dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset)
{
	dns_fixedname_t fixed;
	dns_name_t *found;
	dns_rdataset_t trdataset;
	dns_rdatatype_t type;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_ck_dnssec");

	if (client->view->rpzs->p.break_dnssec || !WANTDNSSEC(client))
		return (ISC_TRUE);

	/*
	 * We do not know if there are signatures if we have not recursed
	 * for them.
	 */
	if (qresult == DNS_R_DELEGATION || qresult == ISC_R_NOTFOUND)
		return (ISC_FALSE);

	if (sigrdataset == NULL)
		return (ISC_TRUE);
	if (dns_rdataset_isassociated(sigrdataset))
		return (ISC_FALSE);

	/*
	 * We are happy to rewrite nothing.
	 */
	if (rdataset == NULL || !dns_rdataset_isassociated(rdataset))
		return (ISC_TRUE);
	/*
	 * Do not rewrite if there is any sign of signatures.
	 */
	if (rdataset->type == dns_rdatatype_nsec ||
	    rdataset->type == dns_rdatatype_nsec3 ||
	    rdataset->type == dns_rdatatype_rrsig)
		return (ISC_FALSE);

	/*
	 * Look for a signature in a negative cache rdataset.
	 */
	if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) == 0)
		return (ISC_TRUE);
	found = dns_fixedname_initname(&fixed);
	dns_rdataset_init(&trdataset);
	for (result = dns_rdataset_first(rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset)) {
		dns_ncache_current(rdataset, found, &trdataset);
		type = trdataset.type;
		dns_rdataset_disassociate(&trdataset);
		if (type == dns_rdatatype_nsec ||
		    type == dns_rdatatype_nsec3 ||
		    type == dns_rdatatype_rrsig)
			return (ISC_FALSE);
	}
	return (ISC_TRUE);
}

/*
 * Extract a network address from the RDATA of an A or AAAA
 * record.
 *
 * Returns:
 *	ISC_R_SUCCESS
 *	ISC_R_NOTIMPLEMENTED	The rdata is not a known address type.
 */
static isc_result_t
rdata_tonetaddr(const dns_rdata_t *rdata, isc_netaddr_t *netaddr) {
	struct in_addr ina;
	struct in6_addr in6a;

	switch (rdata->type) {
	case dns_rdatatype_a:
		INSIST(rdata->length == 4);
		memmove(&ina.s_addr, rdata->data, 4);
		isc_netaddr_fromin(netaddr, &ina);
		return (ISC_R_SUCCESS);
	case dns_rdatatype_aaaa:
		INSIST(rdata->length == 16);
		memmove(in6a.s6_addr, rdata->data, 16);
		isc_netaddr_fromin6(netaddr, &in6a);
		return (ISC_R_SUCCESS);
	default:
		return (ISC_R_NOTIMPLEMENTED);
	}
}

static unsigned char inaddr10_offsets[] = { 0, 3, 11, 16 };
static unsigned char inaddr172_offsets[] = { 0, 3, 7, 15, 20 };
static unsigned char inaddr192_offsets[] = { 0, 4, 8, 16, 21 };

static unsigned char inaddr10[] = "\00210\007IN-ADDR\004ARPA";

static unsigned char inaddr16172[] = "\00216\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr17172[] = "\00217\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr18172[] = "\00218\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr19172[] = "\00219\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr20172[] = "\00220\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr21172[] = "\00221\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr22172[] = "\00222\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr23172[] = "\00223\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr24172[] = "\00224\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr25172[] = "\00225\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr26172[] = "\00226\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr27172[] = "\00227\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr28172[] = "\00228\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr29172[] = "\00229\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr30172[] = "\00230\003172\007IN-ADDR\004ARPA";
static unsigned char inaddr31172[] = "\00231\003172\007IN-ADDR\004ARPA";

static unsigned char inaddr168192[] = "\003168\003192\007IN-ADDR\004ARPA";

static dns_name_t rfc1918names[] = {
	DNS_NAME_INITABSOLUTE(inaddr10, inaddr10_offsets),
	DNS_NAME_INITABSOLUTE(inaddr16172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr17172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr18172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr19172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr20172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr21172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr22172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr23172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr24172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr25172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr26172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr27172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr28172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr29172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr30172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr31172, inaddr172_offsets),
	DNS_NAME_INITABSOLUTE(inaddr168192, inaddr192_offsets)
};


static unsigned char prisoner_data[] = "\010prisoner\004iana\003org";
static unsigned char hostmaster_data[] = "\012hostmaster\014root-servers\003org";

static unsigned char prisoner_offsets[] = { 0, 9, 14, 18 };
static unsigned char hostmaster_offsets[] = { 0, 11, 24, 28 };

static dns_name_t const prisoner =
	DNS_NAME_INITABSOLUTE(prisoner_data, prisoner_offsets);
static dns_name_t const hostmaster =
	DNS_NAME_INITABSOLUTE(hostmaster_data, hostmaster_offsets);

static void
warn_rfc1918(ns_client_t *client, dns_name_t *fname, dns_rdataset_t *rdataset) {
	unsigned int i;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_soa_t soa;
	dns_rdataset_t found;
	isc_result_t result;

	for (i = 0; i < (sizeof(rfc1918names)/sizeof(*rfc1918names)); i++) {
		if (dns_name_issubdomain(fname, &rfc1918names[i])) {
			dns_rdataset_init(&found);
			result = dns_ncache_getrdataset(rdataset,
							&rfc1918names[i],
							dns_rdatatype_soa,
							&found);
			if (result != ISC_R_SUCCESS)
				return;

			result = dns_rdataset_first(&found);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			dns_rdataset_current(&found, &rdata);
			result = dns_rdata_tostruct(&rdata, &soa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			if (dns_name_equal(&soa.origin, &prisoner) &&
			    dns_name_equal(&soa.contact, &hostmaster)) {
				char buf[DNS_NAME_FORMATSIZE];
				dns_name_format(fname, buf, sizeof(buf));
				ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_WARNING,
					      "RFC 1918 response from "
					      "Internet for %s", buf);
			}
			dns_rdataset_disassociate(&found);
			return;
		}
	}
}

static void
query_findclosestnsec3(dns_name_t *qname, dns_db_t *db,
		       dns_dbversion_t *version, ns_client_t *client,
		       dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		       dns_name_t *fname, isc_boolean_t exact,
		       dns_name_t *found)
{
	unsigned char salt[256];
	size_t salt_length;
	isc_uint16_t iterations;
	isc_result_t result;
	unsigned int dboptions;
	dns_fixedname_t fixed;
	dns_hash_t hash;
	dns_name_t name;
	unsigned int skip = 0, labels;
	dns_rdata_nsec3_t nsec3;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_boolean_t optout;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	salt_length = sizeof(salt);
	result = dns_db_getnsec3parameters(db, version, &hash, NULL,
					   &iterations, salt, &salt_length);
	if (result != ISC_R_SUCCESS)
		return;

	dns_name_init(&name, NULL);
	dns_name_clone(qname, &name);
	labels = dns_name_countlabels(&name);
	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * Map unknown algorithm to known value.
	 */
	if (hash == DNS_NSEC3_UNKNOWNALG)
		hash = 1;

 again:
	dns_fixedname_init(&fixed);
	result = dns_nsec3_hashname(&fixed, NULL, NULL, &name,
				    dns_db_origin(db), hash,
				    iterations, salt, salt_length);
	if (result != ISC_R_SUCCESS)
		return;

	dboptions = client->query.dboptions | DNS_DBFIND_FORCENSEC3;
	result = dns_db_findext(db, dns_fixedname_name(&fixed), version,
				dns_rdatatype_nsec3, dboptions, client->now,
				NULL, fname, &cm, &ci, rdataset, sigrdataset);

	if (result == DNS_R_NXDOMAIN) {
		if (!dns_rdataset_isassociated(rdataset)) {
			return;
		}
		result = dns_rdataset_first(rdataset);
		INSIST(result == ISC_R_SUCCESS);
		dns_rdataset_current(rdataset, &rdata);
		dns_rdata_tostruct(&rdata, &nsec3, NULL);
		dns_rdata_reset(&rdata);
		optout = ISC_TF((nsec3.flags & DNS_NSEC3FLAG_OPTOUT) != 0);
		if (found != NULL && optout &&
		    dns_name_issubdomain(&name, dns_db_origin(db)))
		{
			dns_rdataset_disassociate(rdataset);
			if (dns_rdataset_isassociated(sigrdataset))
				dns_rdataset_disassociate(sigrdataset);
			skip++;
			dns_name_getlabelsequence(qname, skip, labels - skip,
						  &name);
			ns_client_log(client, DNS_LOGCATEGORY_DNSSEC,
				      NS_LOGMODULE_QUERY, ISC_LOG_DEBUG(3),
				      "looking for closest provable encloser");
			goto again;
		}
		if (exact)
			ns_client_log(client, DNS_LOGCATEGORY_DNSSEC,
				      NS_LOGMODULE_QUERY, ISC_LOG_WARNING,
				      "expected a exact match NSEC3, got "
				      "a covering record");

	} else if (result != ISC_R_SUCCESS) {
		return;
	} else if (!exact)
		ns_client_log(client, DNS_LOGCATEGORY_DNSSEC,
			      NS_LOGMODULE_QUERY, ISC_LOG_WARNING,
			      "expected covering NSEC3, got an exact match");
	if (found == qname) {
		if (skip != 0U)
			dns_name_getlabelsequence(qname, skip, labels - skip,
						  found);
	} else if (found != NULL)
		dns_name_copy(&name, found, NULL);
	return;
}

static isc_boolean_t
is_v4_client(ns_client_t *client) {
	if (isc_sockaddr_pf(&client->peeraddr) == AF_INET)
		return (ISC_TRUE);
	if (isc_sockaddr_pf(&client->peeraddr) == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&client->peeraddr.type.sin6.sin6_addr))
		return (ISC_TRUE);
	return (ISC_FALSE);
}

static isc_boolean_t
is_v6_client(ns_client_t *client) {
	if (isc_sockaddr_pf(&client->peeraddr) == AF_INET6 &&
	    !IN6_IS_ADDR_V4MAPPED(&client->peeraddr.type.sin6.sin6_addr))
		return (ISC_TRUE);
	return (ISC_FALSE);
}

static isc_uint32_t
dns64_ttl(dns_db_t *db, dns_dbversion_t *version) {
	dns_dbnode_t *node = NULL;
	dns_rdata_soa_t soa;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdataset_t rdataset;
	isc_result_t result;
	isc_uint32_t ttl = ISC_UINT32_MAX;

	dns_rdataset_init(&rdataset);

	result = dns_db_getoriginnode(db, &node);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = dns_db_findrdataset(db, node, version, dns_rdatatype_soa,
				     0, 0, &rdataset, NULL);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_rdataset_first(&rdataset);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	dns_rdataset_current(&rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &soa, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	ttl = ISC_MIN(rdataset.ttl, soa.minimum);

cleanup:
	if (dns_rdataset_isassociated(&rdataset))
		dns_rdataset_disassociate(&rdataset);
	if (node != NULL)
		dns_db_detachnode(db, &node);
	return (ttl);
}

static isc_boolean_t
dns64_aaaaok(ns_client_t *client, dns_rdataset_t *rdataset,
	     dns_rdataset_t *sigrdataset)
{
	isc_netaddr_t netaddr;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(client->interface->mgr);
	dns_dns64_t *dns64 = ISC_LIST_HEAD(client->view->dns64);
	unsigned int flags = 0;
	unsigned int i, count;
	isc_boolean_t *aaaaok;

	INSIST(client->query.dns64_aaaaok == NULL);
	INSIST(client->query.dns64_aaaaoklen == 0);
	INSIST(client->query.dns64_aaaa == NULL);
	INSIST(client->query.dns64_sigaaaa == NULL);

	if (dns64 == NULL)
		return (ISC_TRUE);

	if (RECURSIONOK(client))
		flags |= DNS_DNS64_RECURSIVE;

	if (WANTDNSSEC(client) && sigrdataset != NULL &&
	    dns_rdataset_isassociated(sigrdataset))
		flags |= DNS_DNS64_DNSSEC;

	count = dns_rdataset_count(rdataset);
	aaaaok = isc_mem_get(client->mctx, sizeof(isc_boolean_t) * count);

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	if (dns_dns64_aaaaok(dns64, &netaddr, client->signer,
			     env, flags, rdataset, aaaaok, count))
	{
		for (i = 0; i < count; i++) {
			if (aaaaok != NULL && !aaaaok[i]) {
				SAVE(client->query.dns64_aaaaok, aaaaok);
				client->query.dns64_aaaaoklen = count;
				break;
			}
		}
		if (aaaaok != NULL)
			isc_mem_put(client->mctx, aaaaok,
				    sizeof(isc_boolean_t) * count);
		return (ISC_TRUE);
	}
	if (aaaaok != NULL)
		isc_mem_put(client->mctx, aaaaok,
			    sizeof(isc_boolean_t) * count);
	return (ISC_FALSE);
}

/*
 * Look for the name and type in the redirection zone.  If found update
 * the arguments as appropriate.  Return ISC_TRUE if a update was
 * performed.
 *
 * Only perform the update if the client is in the allow query acl and
 * returning the update would not cause a DNSSEC validation failure.
 */
static isc_result_t
redirect(ns_client_t *client, dns_name_t *name, dns_rdataset_t *rdataset,
	 dns_dbnode_t **nodep, dns_db_t **dbp, dns_dbversion_t **versionp,
	 dns_rdatatype_t qtype)
{
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t fixed;
	dns_name_t *found;
	dns_rdataset_t trdataset;
	isc_result_t result;
	dns_rdatatype_t type;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	ns_dbversion_t *dbversion;

	CTRACE(ISC_LOG_DEBUG(3), "redirect");

	if (client->view->redirect == NULL)
		return (ISC_R_NOTFOUND);

	found = dns_fixedname_initname(&fixed);
	dns_rdataset_init(&trdataset);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	if (WANTDNSSEC(client) && dns_db_iszone(*dbp) && dns_db_issecure(*dbp))
		return (ISC_R_NOTFOUND);

	if (WANTDNSSEC(client) && dns_rdataset_isassociated(rdataset)) {
		if (rdataset->trust == dns_trust_secure)
			return (ISC_R_NOTFOUND);
		if (rdataset->trust == dns_trust_ultimate &&
		    (rdataset->type == dns_rdatatype_nsec ||
		     rdataset->type == dns_rdatatype_nsec3))
			return (ISC_R_NOTFOUND);
		if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) != 0) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset)) {
				dns_ncache_current(rdataset, found, &trdataset);
				type = trdataset.type;
				dns_rdataset_disassociate(&trdataset);
				if (type == dns_rdatatype_nsec ||
				    type == dns_rdatatype_nsec3 ||
				    type == dns_rdatatype_rrsig)
					return (ISC_R_NOTFOUND);
			}
		}
	}

	result = ns_client_checkaclsilent(client, NULL,
				 dns_zone_getqueryacl(client->view->redirect),
					  ISC_TRUE);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_NOTFOUND);

	result = dns_zone_getdb(client->view->redirect, &db);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_NOTFOUND);

	dbversion = query_findversion(client, db);
	if (dbversion == NULL) {
		dns_db_detach(&db);
		return (ISC_R_NOTFOUND);
	}

	/*
	 * Lookup the requested data in the redirect zone.
	 */
	result = dns_db_findext(db, client->query.qname, dbversion->version,
				qtype, DNS_DBFIND_NOZONECUT, client->now,
				&node, found, &cm, &ci, &trdataset, NULL);
	if (result == DNS_R_NXRRSET || result == DNS_R_NCACHENXRRSET) {
		if (dns_rdataset_isassociated(rdataset))
			dns_rdataset_disassociate(rdataset);
		if (dns_rdataset_isassociated(&trdataset))
			dns_rdataset_disassociate(&trdataset);
		goto nxrrset;
	} else if (result != ISC_R_SUCCESS) {
		if (dns_rdataset_isassociated(&trdataset))
			dns_rdataset_disassociate(&trdataset);
		if (node != NULL)
			dns_db_detachnode(db, &node);
		dns_db_detach(&db);
		return (ISC_R_NOTFOUND);
	}

	CTRACE(ISC_LOG_DEBUG(3), "redirect: found data: done");
	dns_name_copy(found, name, NULL);
	if (dns_rdataset_isassociated(rdataset))
		dns_rdataset_disassociate(rdataset);
	if (dns_rdataset_isassociated(&trdataset)) {
		dns_rdataset_clone(&trdataset, rdataset);
		dns_rdataset_disassociate(&trdataset);
	}
 nxrrset:
	if (*nodep != NULL)
		dns_db_detachnode(*dbp, nodep);
	dns_db_detach(dbp);
	dns_db_attachnode(db, node, nodep);
	dns_db_attach(db, dbp);
	dns_db_detachnode(db, &node);
	dns_db_detach(&db);
	*versionp = dbversion->version;

	client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
				     NS_QUERYATTR_NOADDITIONAL);

	return (result);
}

static isc_result_t
redirect2(ns_client_t *client, dns_name_t *name, dns_rdataset_t *rdataset,
	  dns_dbnode_t **nodep, dns_db_t **dbp, dns_dbversion_t **versionp,
	  dns_rdatatype_t qtype, isc_boolean_t *is_zonep)
{
	dns_db_t *db = NULL;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t fixed;
	dns_fixedname_t fixedredirect;
	dns_name_t *found, *redirectname;
	dns_rdataset_t trdataset;
	isc_result_t result;
	dns_rdatatype_t type;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	dns_dbversion_t *version = NULL;
	dns_zone_t *zone = NULL;
	isc_boolean_t is_zone;
	unsigned int options;

	CTRACE(ISC_LOG_DEBUG(3), "redirect2");

	if (client->view->redirectzone == NULL)
		return (ISC_R_NOTFOUND);

	if (dns_name_issubdomain(name, client->view->redirectzone))
		return (ISC_R_NOTFOUND);

	found = dns_fixedname_initname(&fixed);
	dns_rdataset_init(&trdataset);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	if (WANTDNSSEC(client) && dns_db_iszone(*dbp) && dns_db_issecure(*dbp))
		return (ISC_R_NOTFOUND);

	if (WANTDNSSEC(client) && dns_rdataset_isassociated(rdataset)) {
		if (rdataset->trust == dns_trust_secure)
			return (ISC_R_NOTFOUND);
		if (rdataset->trust == dns_trust_ultimate &&
		    (rdataset->type == dns_rdatatype_nsec ||
		     rdataset->type == dns_rdatatype_nsec3))
			return (ISC_R_NOTFOUND);
		if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) != 0) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset)) {
				dns_ncache_current(rdataset, found, &trdataset);
				type = trdataset.type;
				dns_rdataset_disassociate(&trdataset);
				if (type == dns_rdatatype_nsec ||
				    type == dns_rdatatype_nsec3 ||
				    type == dns_rdatatype_rrsig)
					return (ISC_R_NOTFOUND);
			}
		}
	}

	redirectname = dns_fixedname_initname(&fixedredirect);
	if (dns_name_countlabels(name) > 1U) {
		dns_name_t prefix;
		unsigned int labels = dns_name_countlabels(name) - 1;

		dns_name_init(&prefix, NULL);
		dns_name_getlabelsequence(name, 0, labels, &prefix);
		result = dns_name_concatenate(&prefix,
					      client->view->redirectzone,
					      redirectname, NULL);
		if (result != ISC_R_SUCCESS)
			return (ISC_R_NOTFOUND);
	} else
		dns_name_copy(redirectname, client->view->redirectzone, NULL);

	options = 0;
	result = query_getdb(client, redirectname, qtype, options, &zone,
			     &db, &version, &is_zone);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_NOTFOUND);
	if (zone != NULL)
		dns_zone_detach(&zone);

	/*
	 * Lookup the requested data in the redirect zone.
	 */
	result = dns_db_findext(db, redirectname, version,
				qtype, 0, client->now,
				&node, found, &cm, &ci, &trdataset, NULL);
	if (result == DNS_R_NXRRSET || result == DNS_R_NCACHENXRRSET) {
		if (dns_rdataset_isassociated(rdataset))
			dns_rdataset_disassociate(rdataset);
		if (dns_rdataset_isassociated(&trdataset))
			dns_rdataset_disassociate(&trdataset);
		goto nxrrset;
	} else if (result == ISC_R_NOTFOUND || result == DNS_R_DELEGATION) {
		/*
		 * Cleanup.
		 */
		if (dns_rdataset_isassociated(&trdataset))
			dns_rdataset_disassociate(&trdataset);
		if (node != NULL)
			dns_db_detachnode(db, &node);
		dns_db_detach(&db);
		/*
		 * Don't loop forever if the lookup failed last time.
		 */
		if (!REDIRECT(client)) {
			result = query_recurse(client, qtype, redirectname,
					       NULL, NULL, ISC_TRUE);
			if (result == ISC_R_SUCCESS) {
				client->query.attributes |=
						NS_QUERYATTR_RECURSING;
				client->query.attributes |=
						NS_QUERYATTR_REDIRECT;
				return (DNS_R_CONTINUE);
			}
		}
		return (ISC_R_NOTFOUND);
	} else if (result != ISC_R_SUCCESS) {
		if (dns_rdataset_isassociated(&trdataset))
			dns_rdataset_disassociate(&trdataset);
		if (node != NULL)
			dns_db_detachnode(db, &node);
		dns_db_detach(&db);
		return (ISC_R_NOTFOUND);
	}

	CTRACE(ISC_LOG_DEBUG(3), "redirect2: found data: done");
	/*
	 * Adjust the found name to not include the redirectzone suffix.
	 */
	dns_name_split(found, dns_name_countlabels(client->view->redirectzone),
		       found, NULL);
	/*
	 * Make the name absolute.
	 */
	result = dns_name_concatenate(found, dns_rootname, found, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	dns_name_copy(found, name, NULL);
	if (dns_rdataset_isassociated(rdataset))
		dns_rdataset_disassociate(rdataset);
	if (dns_rdataset_isassociated(&trdataset)) {
		dns_rdataset_clone(&trdataset, rdataset);
		dns_rdataset_disassociate(&trdataset);
	}
 nxrrset:
	if (*nodep != NULL)
		dns_db_detachnode(*dbp, nodep);
	dns_db_detach(dbp);
	dns_db_attachnode(db, node, nodep);
	dns_db_attach(db, dbp);
	dns_db_detachnode(db, &node);
	dns_db_detach(&db);
	*is_zonep = is_zone;
	*versionp = version;

	client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
				     NS_QUERYATTR_NOADDITIONAL);

	return (result);
}

/*%
 * Initialize query context 'qctx'. Run by query_setup() when
 * first handling a client query, and by query_resume() when
 * returning from recursion.
 */
static void
qctx_init(ns_client_t *client, dns_fetchevent_t *event,
	  dns_rdatatype_t qtype, query_ctx_t *qctx)
{
	REQUIRE(qctx != NULL);
	REQUIRE(client != NULL);

	/* Set this first so CCTRACE will work */
	qctx->client = client;

	CCTRACE(ISC_LOG_DEBUG(3), "qctx_create");

	qctx->event = event;
	qctx->qtype = qctx->type = qtype;
	qctx->result = ISC_R_SUCCESS;
	qctx->fname = NULL;
	qctx->zfname = NULL;
	qctx->rdataset = NULL;
	qctx->zrdataset = NULL;
	qctx->sigrdataset = NULL;
	qctx->zsigrdataset = NULL;
	qctx->zversion = NULL;
	qctx->node = NULL;
	qctx->db = NULL;
	qctx->zdb = NULL;
	qctx->version = NULL;
	qctx->zone = NULL;
	qctx->need_wildcardproof = ISC_FALSE;
	qctx->redirected = ISC_FALSE;
	qctx->dns64_exclude = qctx->dns64 = qctx->rpz = ISC_FALSE;
	qctx->options = 0;
	qctx->resuming = ISC_FALSE;
	qctx->is_zone = ISC_FALSE;
	qctx->findcoveringnsec = client->view->synthfromdnssec;
	qctx->is_staticstub_zone = ISC_FALSE;
	qctx->nxrewrite = ISC_FALSE;
	qctx->want_stale = ISC_FALSE;
	qctx->answer_has_ns = ISC_FALSE;
	qctx->authoritative = ISC_FALSE;
}

/*%
 * Clean up and disassociate the rdataset and node pointers in qctx.
 */
static void
qctx_clean(query_ctx_t *qctx) {
	if (qctx->rdataset != NULL &&
	    dns_rdataset_isassociated(qctx->rdataset))
	{
		dns_rdataset_disassociate(qctx->rdataset);
	}
	if (qctx->sigrdataset != NULL &&
	    dns_rdataset_isassociated(qctx->sigrdataset))
	{
		dns_rdataset_disassociate(qctx->sigrdataset);
	}
	if (qctx->db != NULL && qctx->node != NULL) {
		dns_db_detachnode(qctx->db, &qctx->node);
	}
}

/*%
 * Free any allocated memory associated with qctx.
 */
static void
qctx_freedata(query_ctx_t *qctx) {
	if (qctx->rdataset != NULL) {
		query_putrdataset(qctx->client, &qctx->rdataset);
	}

	if (qctx->sigrdataset != NULL) {
		query_putrdataset(qctx->client, &qctx->sigrdataset);
	}

	if (qctx->fname != NULL) {
		query_releasename(qctx->client, &qctx->fname);
	}

	if (qctx->db != NULL) {
		INSIST(qctx->node == NULL);
		dns_db_detach(&qctx->db);
	}

	if (qctx->zone != NULL) {
		dns_zone_detach(&qctx->zone);
	}

	if (qctx->zdb != NULL) {
		query_putrdataset(qctx->client, &qctx->zrdataset);
		if (qctx->zsigrdataset != NULL)
			query_putrdataset(qctx->client, &qctx->zsigrdataset);
		if (qctx->zfname != NULL)
			query_releasename(qctx->client, &qctx->zfname);
		dns_db_detach(&qctx->zdb);
	}

	if (qctx->event != NULL) {
		free_devent(qctx->client,
			    ISC_EVENT_PTR(&qctx->event), &qctx->event);
	}
}

/*%
 * Log detailed information about the query immediately after
 * the client request or a return from recursion.
 */
static void
query_trace(query_ctx_t *qctx) {
#ifdef WANT_QUERYTRACE
	char mbuf[BUFSIZ];
	char qbuf[DNS_NAME_FORMATSIZE];

	if (qctx->client->query.origqname != NULL)
		dns_name_format(qctx->client->query.origqname, qbuf,
				sizeof(qbuf));
	else
		snprintf(qbuf, sizeof(qbuf), "<unset>");

	snprintf(mbuf, sizeof(mbuf) - 1,
		 "client attr:0x%x, query attr:0x%X, restarts:%u, "
		 "origqname:%s, timer:%d, authdb:%d, referral:%d",
		 qctx->client->attributes,
		 qctx->client->query.attributes,
		 qctx->client->query.restarts, qbuf,
		 (int) qctx->client->query.timerset,
		 (int) qctx->client->query.authdbset,
		 (int) qctx->client->query.isreferral);
	CCTRACE(ISC_LOG_DEBUG(3), mbuf);
#else
	UNUSED(qctx);
#endif
}

/*
 * Set up query processing for the current query of 'client'.
 * Calls qctx_init() to initialize a query context, checks
 * the SERVFAIL cache, then hands off processing to ns__query_start().
 *
 * This is called only from ns_query_start(), to begin a query
 * for the first time.  Restarting an existing query (for
 * instance, to handle CNAME lookups), is done by calling
 * ns__query_start() again with the same query context. Resuming from
 * recursion is handled by query_resume().
 */
static isc_result_t
query_setup(ns_client_t *client, dns_rdatatype_t qtype) {
	isc_result_t result;
	query_ctx_t qctx;

	qctx_init(client, NULL, qtype, &qctx);
	query_trace(&qctx);

	/*
	 * If it's a SIG query, we'll iterate the node.
	 */
	if (qctx.qtype == dns_rdatatype_rrsig ||
	    qctx.qtype == dns_rdatatype_sig)
	{
		qctx.type = dns_rdatatype_any;
	}

	PROCESS_HOOK(NS_QUERY_SETUP_QCTX_INITIALIZED, &qctx);

	/*
	 * Check SERVFAIL cache
	 */
	result = ns__query_sfcache(&qctx);
	if (result != ISC_R_COMPLETE) {
		return (result);
	}

	return (ns__query_start(&qctx));
}

static isc_boolean_t
get_root_key_sentinel_id(query_ctx_t *qctx, const char *ndata) {
	unsigned int v = 0;
	int i;

	for (i = 0; i < 5; i++) {
		if (ndata[i] < '0' || ndata[i] > '9') {
			return (ISC_FALSE);
		}
		v *= 10;
		v += ndata[i] - '0';
	}
	if (v > 65535U) {
		return (ISC_FALSE);
	}
	qctx->client->query.root_key_sentinel_keyid = v;
	return (ISC_TRUE);
}

/*%
 * Find out if the query is for a root key sentinel and if so, record the type
 * of root key sentinel query and the key id that is being checked for.
 *
 * The code is assuming a zero padded decimal field of width 5.
 */
static void
root_key_sentinel_detect(query_ctx_t *qctx) {
	const char *ndata = (const char *)qctx->client->query.qname->ndata;

	if (qctx->client->query.qname->length > 30 && ndata[0] == 29 &&
	    strncasecmp(ndata + 1, "root-key-sentinel-is-ta-", 24) == 0)
	{
		if (!get_root_key_sentinel_id(qctx, ndata + 25)) {
			return;
		}
		qctx->client->query.root_key_sentinel_is_ta = ISC_TRUE;
		/*
		 * Simplify processing by disabling agressive
		 * negative caching.
		 */
		qctx->findcoveringnsec = ISC_FALSE;
		ns_client_log(qctx->client, NS_LOGCATEGORY_TAT,
			      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
			      "root-key-sentinel-is-ta query label found");
	} else if (qctx->client->query.qname->length > 31 && ndata[0] == 30 &&
		   strncasecmp(ndata + 1, "root-key-sentinel-not-ta-", 25) == 0)
	{
		if (!get_root_key_sentinel_id(qctx, ndata + 26)) {
			return;
		}
		qctx->client->query.root_key_sentinel_not_ta = ISC_TRUE;
		/*
		 * Simplify processing by disabling agressive
		 * negative caching.
		 */
		qctx->findcoveringnsec = ISC_FALSE;
		ns_client_log(qctx->client, NS_LOGCATEGORY_TAT,
			      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
			      "root-key-sentinel-not-ta query label found");
	}
}

/*%
 * Starting point for a client query or a chaining query.
 *
 * Called first by query_setup(), and then again as often as needed to
 * follow a CNAME chain.  Determines which authoritative database to
 * search, then hands off processing to query_lookup().
 */
isc_result_t
ns__query_start(query_ctx_t *qctx) {
	isc_result_t result;
	CCTRACE(ISC_LOG_DEBUG(3), "ns__query_start");
	qctx->want_restart = ISC_FALSE;
	qctx->authoritative = ISC_FALSE;
	qctx->version = NULL;
	qctx->zversion = NULL;
	qctx->need_wildcardproof = ISC_FALSE;
	qctx->rpz = ISC_FALSE;

	if (qctx->client->view->checknames &&
	    !dns_rdata_checkowner(qctx->client->query.qname,
				  qctx->client->message->rdclass,
				  qctx->qtype, ISC_FALSE))
	{
		char namebuf[DNS_NAME_FORMATSIZE];
		char typename[DNS_RDATATYPE_FORMATSIZE];
		char classname[DNS_RDATACLASS_FORMATSIZE];

		dns_name_format(qctx->client->query.qname,
				namebuf, sizeof(namebuf));
		dns_rdatatype_format(qctx->qtype, typename, sizeof(typename));
		dns_rdataclass_format(qctx->client->message->rdclass,
				      classname, sizeof(classname));
		ns_client_log(qctx->client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_QUERY, ISC_LOG_ERROR,
			      "check-names failure %s/%s/%s", namebuf,
			      typename, classname);
		QUERY_ERROR(qctx, DNS_R_REFUSED);
		return (query_done(qctx));
	}

	/*
	 * Setup for root key sentinel processing.
	 */
	if (qctx->client->view->root_key_sentinel &&
	    qctx->client->query.restarts == 0 &&
	    (qctx->qtype == dns_rdatatype_a ||
	     qctx->qtype == dns_rdatatype_aaaa) &&
	    (qctx->client->message->flags & DNS_MESSAGEFLAG_CD) == 0)
	{
		 root_key_sentinel_detect(qctx);
	}

	/*
	 * First we must find the right database.
	 */
	qctx->options &= DNS_GETDB_NOLOG; /* Preserve DNS_GETDB_NOLOG. */
	if (dns_rdatatype_atparent(qctx->qtype) &&
	    !dns_name_equal(qctx->client->query.qname, dns_rootname))
	{
		/*
		 * If authoritative data for this QTYPE is supposed to live in
		 * the parent zone, do not look for an exact match for QNAME,
		 * but rather for its containing zone (unless the QNAME is
		 * root).
		 */
		qctx->options |= DNS_GETDB_NOEXACT;
	}

	result = query_getdb(qctx->client, qctx->client->query.qname,
			     qctx->qtype, qctx->options, &qctx->zone,
			     &qctx->db, &qctx->version, &qctx->is_zone);
	if (ISC_UNLIKELY((result != ISC_R_SUCCESS || !qctx->is_zone) &&
			 qctx->qtype == dns_rdatatype_ds &&
			 !RECURSIONOK(qctx->client) &&
			 (qctx->options & DNS_GETDB_NOEXACT) != 0))
	{
		/*
		 * This is a non-recursive QTYPE=DS query with QNAME whose
		 * parent we are not authoritative for.  Check whether we are
		 * authoritative for QNAME, because if so, we need to send a
		 * "no data" response as required by RFC 4035, section 3.1.4.1.
		 */
		dns_db_t *tdb = NULL;
		dns_zone_t *tzone = NULL;
		dns_dbversion_t *tversion = NULL;
		isc_result_t tresult;

		tresult = query_getzonedb(qctx->client,
					  qctx->client->query.qname,
					  qctx->qtype,
					  DNS_GETDB_PARTIAL,
					  &tzone, &tdb, &tversion);
		if (tresult == ISC_R_SUCCESS) {
			/*
			 * We are authoritative for QNAME.  Attach the relevant
			 * zone to query context, set result to ISC_R_SUCCESS.
			 */
			qctx->options &= ~DNS_GETDB_NOEXACT;
			query_putrdataset(qctx->client, &qctx->rdataset);
			if (qctx->db != NULL) {
				dns_db_detach(&qctx->db);
			}
			if (qctx->zone != NULL) {
				dns_zone_detach(&qctx->zone);
			}
			qctx->version = NULL;
			RESTORE(qctx->version, tversion);
			RESTORE(qctx->db, tdb);
			RESTORE(qctx->zone, tzone);
			qctx->is_zone = ISC_TRUE;
			result = ISC_R_SUCCESS;
		} else {
			/*
			 * We are not authoritative for QNAME.  Clean up and
			 * leave result as it was.
			 */
			if (tdb != NULL) {
				dns_db_detach(&tdb);
			}
			if (tzone != NULL) {
				dns_zone_detach(&tzone);
			}
		}
	}
	/*
	 * If we did not find a database from which we can answer the query,
	 * respond with either REFUSED or SERVFAIL, depending on what the
	 * result of query_getdb() was.
	 */
	if (result != ISC_R_SUCCESS) {
		if (result == DNS_R_REFUSED) {
			if (WANTRECURSION(qctx->client)) {
				inc_stats(qctx->client,
					  ns_statscounter_recurserej);
			} else {
				inc_stats(qctx->client,
					  ns_statscounter_authrej);
			}
			if (!PARTIALANSWER(qctx->client)) {
				QUERY_ERROR(qctx, DNS_R_REFUSED);
			}
		} else {
			CCTRACE(ISC_LOG_ERROR,
			       "ns__query_start: query_getdb failed");
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		}
		return (query_done(qctx));
	}

	/*
	 * We found a database from which we can answer the query.  Update
	 * relevant query context flags if the answer is to be prepared using
	 * authoritative data.
	 */
	qctx->is_staticstub_zone = ISC_FALSE;
	if (qctx->is_zone) {
		qctx->authoritative = ISC_TRUE;
		if (qctx->zone != NULL &&
		    dns_zone_gettype(qctx->zone) == dns_zone_staticstub)
		{
			qctx->is_staticstub_zone = ISC_TRUE;
		}
	}

	/*
	 * Attach to the database which will be used to prepare the answer.
	 * Update query statistics.
	 */
	if (qctx->event == NULL && qctx->client->query.restarts == 0) {
		if (qctx->is_zone) {
			if (qctx->zone != NULL) {
				/*
				 * if is_zone = true, zone = NULL then this is
				 * a DLZ zone.  Don't attempt to attach zone.
				 */
				dns_zone_attach(qctx->zone,
						&qctx->client->query.authzone);
			}
			dns_db_attach(qctx->db, &qctx->client->query.authdb);
		}
		qctx->client->query.authdbset = ISC_TRUE;

		/* Track TCP vs UDP stats per zone */
		if (TCP(qctx->client)) {
			inc_stats(qctx->client, ns_statscounter_tcp);
		} else {
			inc_stats(qctx->client, ns_statscounter_udp);
		}
	}

	return (query_lookup(qctx));
}

/*%
 * Perform a local database lookup, in either an authoritative or
 * cache database. If unable to answer, call query_done(); otherwise
 * hand off processing to query_gotanswer().
 */
static isc_result_t
query_lookup(query_ctx_t *qctx) {
	isc_buffer_t b;
	isc_result_t result;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	dns_name_t *rpzqname = NULL;
	unsigned int dboptions;

	CCTRACE(ISC_LOG_DEBUG(3), "query_lookup");

	PROCESS_HOOK(NS_QUERY_LOOKUP_BEGIN, qctx);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, qctx->client, NULL);

	/*
	 * We'll need some resources...
	 */
	qctx->dbuf = query_getnamebuf(qctx->client);
	if (ISC_UNLIKELY(qctx->dbuf == NULL)) {
		CCTRACE(ISC_LOG_ERROR,
		       "query_lookup: query_getnamebuf failed (2)");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	qctx->fname = query_newname(qctx->client, qctx->dbuf, &b);
	qctx->rdataset = query_newrdataset(qctx->client);

	if (ISC_UNLIKELY(qctx->fname == NULL || qctx->rdataset == NULL)) {
		CCTRACE(ISC_LOG_ERROR,
		       "query_lookup: query_newname failed (2)");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	if ((WANTDNSSEC(qctx->client) || qctx->findcoveringnsec) &&
	    (!qctx->is_zone || dns_db_issecure(qctx->db)))
	{
		qctx->sigrdataset = query_newrdataset(qctx->client);
		if (qctx->sigrdataset == NULL) {
			CCTRACE(ISC_LOG_ERROR,
			       "query_lookup: query_newrdataset failed (2)");
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
			return (query_done(qctx));
		}
	}

	/*
	 * Now look for an answer in the database.
	 */
	if (qctx->dns64 && qctx->rpz) {
		rpzqname = qctx->client->query.rpz_st->p_name;
	} else {
		rpzqname = qctx->client->query.qname;
	}

	dboptions = qctx->client->query.dboptions;
	if (!qctx->is_zone && qctx->findcoveringnsec &&
	    (qctx->type != dns_rdatatype_null || !dns_name_istat(rpzqname)))
		dboptions |= DNS_DBFIND_COVERINGNSEC;

	result = dns_db_findext(qctx->db, rpzqname, qctx->version, qctx->type,
				dboptions, qctx->client->now, &qctx->node,
				qctx->fname, &cm, &ci,
				qctx->rdataset, qctx->sigrdataset);

	/*
	 * Fixup fname and sigrdataset.
	 */
	if (qctx->dns64 && qctx->rpz) {
		isc_result_t rresult;

		rresult = dns_name_copy(qctx->client->query.qname,
					qctx->fname, NULL);
		RUNTIME_CHECK(rresult == ISC_R_SUCCESS);
		if (qctx->sigrdataset != NULL &&
		    dns_rdataset_isassociated(qctx->sigrdataset))
		{
			dns_rdataset_disassociate(qctx->sigrdataset);
		}
	}

	if (!qctx->is_zone) {
		dns_cache_updatestats(qctx->client->view->cache, result);
	}

	if (qctx->want_stale) {
		char namebuf[DNS_NAME_FORMATSIZE];
		isc_boolean_t success;

		qctx->client->query.dboptions &= ~DNS_DBFIND_STALEOK;
		qctx->want_stale = ISC_FALSE;
		if (dns_rdataset_isassociated(qctx->rdataset) &&
		    dns_rdataset_count(qctx->rdataset) > 0 &&
		    STALE(qctx->rdataset)) {
			qctx->rdataset->ttl =
					 qctx->client->view->staleanswerttl;
			success = ISC_TRUE;
		} else {
			success = ISC_FALSE;
		}

		dns_name_format(qctx->client->query.qname,
				namebuf, sizeof(namebuf));
		isc_log_write(ns_lctx, NS_LOGCATEGORY_SERVE_STALE,
			      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
			      "%s resolver failure, stale answer %s",
			      namebuf, success ? "used" : "unavailable");

		if (!success) {
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
			return (query_done(qctx));
		}
	}

	return (query_gotanswer(qctx, result));
}

/*
 * Event handler to resume processing a query after recursion.
 * If the query has timed out or been canceled or the system
 * is shutting down, clean up and exit; otherwise, call
 * query_resume() to continue the ongoing work.
 */
static void
fetch_callback(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent = (dns_fetchevent_t *)event;
	dns_fetch_t *fetch = NULL;
	ns_client_t *client;
	isc_boolean_t fetch_canceled, client_shuttingdown;
	isc_result_t result;
	isc_logcategory_t *logcategory = NS_LOGCATEGORY_QUERY_ERRORS;
	int errorloglevel;

	UNUSED(task);

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	client = devent->ev_arg;
	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);
	REQUIRE(RECURSING(client));

	LOCK(&client->query.fetchlock);
	if (client->query.fetch != NULL) {
		/*
		 * This is the fetch we've been waiting for.
		 */
		INSIST(devent->fetch == client->query.fetch);
		client->query.fetch = NULL;
		fetch_canceled = ISC_FALSE;
		/*
		 * Update client->now.
		 */
		isc_stdtime_get(&client->now);
	} else {
		/*
		 * This is a fetch completion event for a canceled fetch.
		 * Clean up and don't resume the find.
		 */
		fetch_canceled = ISC_TRUE;
	}
	UNLOCK(&client->query.fetchlock);
	INSIST(client->query.fetch == NULL);

	client->query.attributes &= ~NS_QUERYATTR_RECURSING;
	SAVE(fetch, devent->fetch);

	/*
	 * If this client is shutting down, or this transaction
	 * has timed out, do not resume the find.
	 */
	client_shuttingdown = ns_client_shuttingdown(client);
	if (fetch_canceled || client_shuttingdown) {
		free_devent(client, &event, &devent);
		if (fetch_canceled) {
			CTRACE(ISC_LOG_ERROR, "fetch cancelled");
			query_error(client, DNS_R_SERVFAIL, __LINE__);
		} else {
			query_next(client, ISC_R_CANCELED);
		}
		/*
		 * This may destroy the client.
		 */
		ns_client_detach(&client);
	} else {
		query_ctx_t qctx;

		qctx_init(client, devent, 0, &qctx);
		query_trace(&qctx);

		result = query_resume(&qctx);
		if (result != ISC_R_SUCCESS) {
			if (result == DNS_R_SERVFAIL) {
				errorloglevel = ISC_LOG_DEBUG(2);
			} else {
				errorloglevel = ISC_LOG_DEBUG(4);
			}
			if (isc_log_wouldlog(ns_lctx, errorloglevel)) {
				dns_resolver_logfetch(fetch, ns_lctx,
						      logcategory,
						      NS_LOGMODULE_QUERY,
						      errorloglevel, ISC_FALSE);
			}
		}
	}

	dns_resolver_destroyfetch(&fetch);
}

/*%
 * Check whether the recursion parameters in 'param' match the current query's
 * recursion parameters provided in 'qtype', 'qname', and 'qdomain'.
 */
static isc_boolean_t
recparam_match(const ns_query_recparam_t *param, dns_rdatatype_t qtype,
	       const dns_name_t *qname, const dns_name_t *qdomain)
{
	REQUIRE(param != NULL);

	return (ISC_TF(param->qtype == qtype &&
		       param->qname != NULL && qname != NULL &&
		       param->qdomain != NULL && qdomain != NULL &&
		       dns_name_equal(param->qname, qname) &&
		       dns_name_equal(param->qdomain, qdomain)));
}

/*%
 * Update 'param' with current query's recursion parameters provided in
 * 'qtype', 'qname', and 'qdomain'.
 */
static void
recparam_update(ns_query_recparam_t *param, dns_rdatatype_t qtype,
		const dns_name_t *qname, const dns_name_t *qdomain)
{
	isc_result_t result;

	REQUIRE(param != NULL);

	param->qtype = qtype;

	if (qname == NULL) {
		param->qname = NULL;
	} else {
		dns_fixedname_init(&param->fqname);
		param->qname = dns_fixedname_name(&param->fqname);
		result = dns_name_copy(qname, param->qname, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	if (qdomain == NULL) {
		param->qdomain = NULL;
	} else {
		dns_fixedname_init(&param->fqdomain);
		param->qdomain = dns_fixedname_name(&param->fqdomain);
		result = dns_name_copy(qdomain, param->qdomain, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}
}

/*%
 * Prepare client for recursion, then create a resolver fetch, with
 * the event callback set to fetch_callback(). Afterward we terminate
 * this phase of the query, and resume with a new query context when
 * recursion completes.
 */
static isc_result_t
query_recurse(ns_client_t *client, dns_rdatatype_t qtype, dns_name_t *qname,
	      dns_name_t *qdomain, dns_rdataset_t *nameservers,
	      isc_boolean_t resuming)
{
	isc_result_t result;
	dns_rdataset_t *rdataset, *sigrdataset;
	isc_sockaddr_t *peeraddr = NULL;

	CTRACE(ISC_LOG_DEBUG(3), "query_recurse");

	/*
	 * Check recursion parameters from the previous query to see if they
	 * match.  If not, update recursion parameters and proceed.
	 */
	if (recparam_match(&client->query.recparam, qtype, qname, qdomain)) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
			      "recursion loop detected");
		return (ISC_R_FAILURE);
	}

	recparam_update(&client->query.recparam, qtype, qname, qdomain);

	if (!resuming)
		inc_stats(client, ns_statscounter_recursion);

	/*
	 * We are about to recurse, which means that this client will
	 * be unavailable for serving new requests for an indeterminate
	 * amount of time.  If this client is currently responsible
	 * for handling incoming queries, set up a new client
	 * object to handle them while we are waiting for a
	 * response.  There is no need to replace TCP clients
	 * because those have already been replaced when the
	 * connection was accepted (if allowed by the TCP quota).
	 */
	if (client->recursionquota == NULL) {
		result = isc_quota_attach(&client->sctx->recursionquota,
					  &client->recursionquota);

		ns_stats_increment(client->sctx->nsstats,
				   ns_statscounter_recursclients);

		if  (result == ISC_R_SOFTQUOTA) {
			static isc_stdtime_t last = 0;
			isc_stdtime_t now;
			isc_stdtime_get(&now);
			if (now != last) {
				last = now;
				ns_client_log(client, NS_LOGCATEGORY_CLIENT,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_WARNING,
					      "recursive-clients soft limit "
					      "exceeded (%d/%d/%d), "
					      "aborting oldest query",
					      client->recursionquota->used,
					      client->recursionquota->soft,
					      client->recursionquota->max);
			}
			ns_client_killoldestquery(client);
			result = ISC_R_SUCCESS;
		} else if (result == ISC_R_QUOTA) {
			static isc_stdtime_t last = 0;
			isc_stdtime_t now;
			isc_stdtime_get(&now);
			if (now != last) {
				last = now;
				ns_client_log(client, NS_LOGCATEGORY_CLIENT,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_WARNING,
					      "no more recursive clients "
					      "(%d/%d/%d): %s",
					      client->sctx->recursionquota.used,
					      client->sctx->recursionquota.soft,
					      client->sctx->recursionquota.max,
					      isc_result_totext(result));
			}
			ns_client_killoldestquery(client);
		}
		if (result == ISC_R_SUCCESS && !client->mortal &&
		    !TCP(client)) {
			result = ns_client_replace(client);
			if (result != ISC_R_SUCCESS) {
				ns_client_log(client, NS_LOGCATEGORY_CLIENT,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_WARNING,
					      "ns_client_replace() failed: %s",
					      isc_result_totext(result));
				isc_quota_detach(&client->recursionquota);
				ns_stats_decrement(client->sctx->nsstats,
					   ns_statscounter_recursclients);
			}
		}
		if (result != ISC_R_SUCCESS)
			return (result);
		ns_client_recursing(client);
	}

	/*
	 * Invoke the resolver.
	 */
	REQUIRE(nameservers == NULL || nameservers->type == dns_rdatatype_ns);
	REQUIRE(client->query.fetch == NULL);

	rdataset = query_newrdataset(client);
	if (rdataset == NULL) {
		return (ISC_R_NOMEMORY);
	}

	if (WANTDNSSEC(client)) {
		sigrdataset = query_newrdataset(client);
		if (sigrdataset == NULL) {
			query_putrdataset(client, &rdataset);
			return (ISC_R_NOMEMORY);
		}
	} else {
		sigrdataset = NULL;
	}

	if (client->query.timerset == ISC_FALSE) {
		ns_client_settimeout(client, 60);
	}

	if (!TCP(client)) {
		peeraddr = &client->peeraddr;
	}

	result = dns_resolver_createfetch3(client->view->resolver,
					   qname, qtype, qdomain, nameservers,
					   NULL, peeraddr, client->message->id,
					   client->query.fetchoptions, 0, NULL,
					   client->task, fetch_callback,
					   client, rdataset, sigrdataset,
					   &client->query.fetch);
	if (result != ISC_R_SUCCESS) {
		query_putrdataset(client, &rdataset);
		if (sigrdataset != NULL) {
			query_putrdataset(client, &sigrdataset);
		}
	}

	/*
	 * We're now waiting for a fetch event. A client which is
	 * shutting down will not be destroyed until all the events
	 * have been received.
	 */

	return (result);
}

/*%
 * Restores the query context after resuming from recursion, and
 * continues the query processing if needed.
 */
static isc_result_t
query_resume(query_ctx_t *qctx) {
	isc_result_t result;
	dns_name_t *tname;
	isc_buffer_t b;
#ifdef WANT_QUERYTRACE
	char mbuf[BUFSIZ];
	char qbuf[DNS_NAME_FORMATSIZE];
	char tbuf[DNS_RDATATYPE_FORMATSIZE];
#endif

	qctx->want_restart = ISC_FALSE;

	qctx->rpz_st = qctx->client->query.rpz_st;
	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0)
	{
		CCTRACE(ISC_LOG_DEBUG(3), "resume from RPZ recursion");
#ifdef WANT_QUERYTRACE
		{
			char pbuf[DNS_NAME_FORMATSIZE] = "<unset>";
			char fbuf[DNS_NAME_FORMATSIZE] = "<unset>";
			if (qctx->rpz_st->r_name != NULL)
				dns_name_format(qctx->rpz_st->r_name,
						qbuf, sizeof(qbuf));
			else
				snprintf(qbuf, sizeof(qbuf),
					 "<unset>");
			if (qctx->rpz_st->p_name != NULL)
				dns_name_format(qctx->rpz_st->p_name,
						pbuf, sizeof(pbuf));
			if (qctx->rpz_st->fname != NULL)
				dns_name_format(qctx->rpz_st->fname,
						fbuf, sizeof(fbuf));

			snprintf(mbuf, sizeof(mbuf) - 1,
				 "rpz rname:%s, pname:%s, qctx->fname:%s",
				 qbuf, pbuf, fbuf);
			CCTRACE(ISC_LOG_DEBUG(3), mbuf);
		}
#endif

		qctx->is_zone = qctx->rpz_st->q.is_zone;
		qctx->authoritative = qctx->rpz_st->q.authoritative;
		RESTORE(qctx->zone, qctx->rpz_st->q.zone);
		RESTORE(qctx->node, qctx->rpz_st->q.node);
		RESTORE(qctx->db, qctx->rpz_st->q.db);
		RESTORE(qctx->rdataset, qctx->rpz_st->q.rdataset);
		RESTORE(qctx->sigrdataset, qctx->rpz_st->q.sigrdataset);
		qctx->qtype = qctx->rpz_st->q.qtype;

		if (qctx->event->node != NULL)
			dns_db_detachnode(qctx->event->db, &qctx->event->node);
		SAVE(qctx->rpz_st->r.db, qctx->event->db);
		qctx->rpz_st->r.r_type = qctx->event->qtype;
		SAVE(qctx->rpz_st->r.r_rdataset, qctx->event->rdataset);
		query_putrdataset(qctx->client, &qctx->event->sigrdataset);
	} else if (REDIRECT(qctx->client)) {
		/*
		 * Restore saved state.
		 */
		CCTRACE(ISC_LOG_DEBUG(3),
		       "resume from redirect recursion");
#ifdef WANT_QUERYTRACE
		dns_name_format(qctx->client->query.redirect.fname,
				qbuf, sizeof(qbuf));
		dns_rdatatype_format(qctx->client->query.redirect.qtype,
				     tbuf, sizeof(tbuf));
		snprintf(mbuf, sizeof(mbuf) - 1,
			 "redirect qctx->fname:%s, qtype:%s, auth:%d",
			 qbuf, tbuf,
			 qctx->client->query.redirect.authoritative);
		CCTRACE(ISC_LOG_DEBUG(3), mbuf);
#endif
		qctx->qtype = qctx->client->query.redirect.qtype;
		INSIST(qctx->client->query.redirect.rdataset != NULL);
		RESTORE(qctx->rdataset, qctx->client->query.redirect.rdataset);
		RESTORE(qctx->sigrdataset,
			qctx->client->query.redirect.sigrdataset);
		RESTORE(qctx->db, qctx->client->query.redirect.db);
		RESTORE(qctx->node, qctx->client->query.redirect.node);
		RESTORE(qctx->zone, qctx->client->query.redirect.zone);
		qctx->authoritative =
			qctx->client->query.redirect.authoritative;
		qctx->is_zone = qctx->client->query.redirect.is_zone;

		/*
		 * Free resources used while recursing.
		 */
		query_putrdataset(qctx->client, &qctx->event->rdataset);
		query_putrdataset(qctx->client, &qctx->event->sigrdataset);
		if (qctx->event->node != NULL)
			dns_db_detachnode(qctx->event->db, &qctx->event->node);
		if (qctx->event->db != NULL)
			dns_db_detach(&qctx->event->db);
	} else {
		CCTRACE(ISC_LOG_DEBUG(3),
		       "resume from normal recursion");
		qctx->authoritative = ISC_FALSE;

		qctx->qtype = qctx->event->qtype;
		SAVE(qctx->db, qctx->event->db);
		SAVE(qctx->node, qctx->event->node);
		SAVE(qctx->rdataset, qctx->event->rdataset);
		SAVE(qctx->sigrdataset, qctx->event->sigrdataset);
	}
	INSIST(qctx->rdataset != NULL);

	if (qctx->qtype == dns_rdatatype_rrsig ||
	    qctx->qtype == dns_rdatatype_sig)
	{
		qctx->type = dns_rdatatype_any;
	} else {
		qctx->type = qctx->qtype;
	}

	if (DNS64(qctx->client)) {
		qctx->client->query.attributes &= ~NS_QUERYATTR_DNS64;
		qctx->dns64 = ISC_TRUE;
	}

	if (DNS64EXCLUDE(qctx->client)) {
		qctx->client->query.attributes &= ~NS_QUERYATTR_DNS64EXCLUDE;
		qctx->dns64_exclude = ISC_TRUE;
	}

	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0)
	{
		/*
		 * Has response policy changed out from under us?
		 */
		if (qctx->rpz_st->rpz_ver != qctx->client->view->rpzs->rpz_ver)
		{
			ns_client_log(qctx->client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_QUERY,
				      DNS_RPZ_INFO_LEVEL,
				      "query_resume: RPZ settings "
				      "out of date "
				      "(rpz_ver %d, expected %d)",
				      qctx->client->view->rpzs->rpz_ver,
				      qctx->rpz_st->rpz_ver);
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
			return (query_done(qctx));
		}
	}

	/*
	 * We'll need some resources...
	 */
	qctx->dbuf = query_getnamebuf(qctx->client);
	if (qctx->dbuf == NULL) {
		CCTRACE(ISC_LOG_ERROR,
		       "query_resume: query_getnamebuf failed (1)");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	qctx->fname = query_newname(qctx->client, qctx->dbuf, &b);
	if (qctx->fname == NULL) {
		CCTRACE(ISC_LOG_ERROR,
		       "query_resume: query_newname failed (1)");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0) {
		tname = qctx->rpz_st->fname;
	} else if (REDIRECT(qctx->client)) {
		tname = qctx->client->query.redirect.fname;
	} else {
		tname = dns_fixedname_name(&qctx->event->foundname);
	}

	result = dns_name_copy(tname, qctx->fname, NULL);
	if (result != ISC_R_SUCCESS) {
		CCTRACE(ISC_LOG_ERROR,
		       "query_resume: dns_name_copy failed");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0) {
		qctx->rpz_st->r.r_result = qctx->event->result;
		result = qctx->rpz_st->q.result;
		free_devent(qctx->client,
			    ISC_EVENT_PTR(&qctx->event), &qctx->event);
	} else if (REDIRECT(qctx->client)) {
		result = qctx->client->query.redirect.result;
		qctx->is_zone = qctx->client->query.redirect.is_zone;
	} else {
		result = qctx->event->result;
	}

	qctx->resuming = ISC_TRUE;

	return (query_gotanswer(qctx, result));
}

/*%
 * If the query is recursive, check the SERVFAIL cache to see whether
 * identical queries have failed recently.  If we find a match, and it was
 * from a query with CD=1, *or* if the current query has CD=0, then we just
 * return SERVFAIL again.  This prevents a validation failure from eliciting a
 * SERVFAIL response to a CD=1 query.
 */
isc_result_t
ns__query_sfcache(query_ctx_t *qctx) {
	isc_boolean_t failcache;
	isc_uint32_t flags;

	/*
	 * The SERVFAIL cache doesn't apply to authoritative queries.
	 */
	if (!RECURSIONOK(qctx->client)) {
		return (ISC_R_COMPLETE);
	}

	flags = 0;
#ifdef ENABLE_AFL
	if (qctx->client->sctx->fuzztype == isc_fuzz_resolver) {
		failcache = ISC_FALSE;
	} else {
		failcache =
			dns_badcache_find(qctx->client->view->failcache,
					  qctx->client->query.qname,
					  qctx->qtype, &flags,
					  &qctx->client->tnow);
	}
#else
	failcache = dns_badcache_find(qctx->client->view->failcache,
				      qctx->client->query.qname,
				      qctx->qtype, &flags,
				      &qctx->client->tnow);
#endif
	if (failcache &&
	    (((flags & NS_FAILCACHE_CD) != 0) ||
	     ((qctx->client->message->flags & DNS_MESSAGEFLAG_CD) == 0)))
	{
		if (isc_log_wouldlog(ns_lctx, ISC_LOG_DEBUG(1))) {
			char namebuf[DNS_NAME_FORMATSIZE];
			char typename[DNS_RDATATYPE_FORMATSIZE];

			dns_name_format(qctx->client->query.qname,
					namebuf, sizeof(namebuf));
			dns_rdatatype_format(qctx->qtype, typename,
					     sizeof(typename));
			ns_client_log(qctx->client,
				      NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_QUERY,
				      ISC_LOG_DEBUG(1),
				      "servfail cache hit %s/%s (%s)",
				      namebuf, typename,
				      ((flags & NS_FAILCACHE_CD) != 0)
				       ? "CD=1"
				       : "CD=0");
		}

		qctx->client->attributes |= NS_CLIENTATTR_NOSETFC;
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	return (ISC_R_COMPLETE);
}

/*%
 * Handle response rate limiting (RRL).
 */
static isc_result_t
query_checkrrl(query_ctx_t *qctx, isc_result_t result) {
	/*
	 * Rate limit these responses to this client.
	 * Do not delay counting and handling obvious referrals,
	 *	since those won't come here again.
	 * Delay handling delegations for which we are certain to recurse and
	 *	return here (DNS_R_DELEGATION, not a child of one of our
	 *	own zones, and recursion enabled)
	 * Don't mess with responses rewritten by RPZ
	 * Count each response at most once.
	 */
	if (qctx->client->view->rrl != NULL &&
	    !HAVECOOKIE(qctx->client) &&
	    ((qctx->fname != NULL && dns_name_isabsolute(qctx->fname)) ||
	     (result == ISC_R_NOTFOUND && !RECURSIONOK(qctx->client))) &&
	    !(result == DNS_R_DELEGATION &&
	      !qctx->is_zone && RECURSIONOK(qctx->client)) &&
	    (qctx->client->query.rpz_st == NULL ||
	     (qctx->client->query.rpz_st->state & DNS_RPZ_REWRITTEN) == 0) &&
	    (qctx->client->query.attributes & NS_QUERYATTR_RRL_CHECKED) == 0)
	{
		dns_rdataset_t nc_rdataset;
		isc_boolean_t wouldlog;
		dns_fixedname_t fixed;
		const dns_name_t *constname;
		char log_buf[DNS_RRL_LOG_BUF_LEN];
		isc_result_t nc_result, resp_result;
		dns_rrl_result_t rrl_result;

		qctx->client->query.attributes |= NS_QUERYATTR_RRL_CHECKED;

		wouldlog = isc_log_wouldlog(ns_lctx, DNS_RRL_LOG_DROP);
		constname = qctx->fname;
		if (result == DNS_R_NXDOMAIN) {
			/*
			 * Use the database origin name to rate limit NXDOMAIN
			 */
			if (qctx->db != NULL)
				constname = dns_db_origin(qctx->db);
			resp_result = result;
		} else if (result == DNS_R_NCACHENXDOMAIN &&
			   qctx->rdataset != NULL &&
			   dns_rdataset_isassociated(qctx->rdataset) &&
			   (qctx->rdataset->attributes &
			    DNS_RDATASETATTR_NEGATIVE) != 0) {
			/*
			 * Try to use owner name in the negative cache SOA.
			 */
			dns_fixedname_init(&fixed);
			dns_rdataset_init(&nc_rdataset);
			for (nc_result = dns_rdataset_first(qctx->rdataset);
			     nc_result == ISC_R_SUCCESS;
			     nc_result = dns_rdataset_next(qctx->rdataset))
			{
				dns_ncache_current(qctx->rdataset,
						   dns_fixedname_name(&fixed),
						   &nc_rdataset);
				if (nc_rdataset.type == dns_rdatatype_soa) {
					dns_rdataset_disassociate(&nc_rdataset);
					constname = dns_fixedname_name(&fixed);
					break;
				}
				dns_rdataset_disassociate(&nc_rdataset);
			}
			resp_result = DNS_R_NXDOMAIN;
		} else if (result == DNS_R_NXRRSET ||
			   result == DNS_R_EMPTYNAME) {
			resp_result = DNS_R_NXRRSET;
		} else if (result == DNS_R_DELEGATION) {
			resp_result = result;
		} else if (result == ISC_R_NOTFOUND) {
			/*
			 * Handle referral to ".", including when recursion
			 * is off or not requested and the hints have not
			 * been loaded or we have "additional-from-cache no".
			 */
			constname = dns_rootname;
			resp_result = DNS_R_DELEGATION;
		} else {
			resp_result = ISC_R_SUCCESS;
		}

		rrl_result = dns_rrl(qctx->client->view,
				     &qctx->client->peeraddr,
				     TCP(qctx->client),
				     qctx->client->message->rdclass,
				     qctx->qtype, constname,
				     resp_result, qctx->client->now,
				     wouldlog, log_buf, sizeof(log_buf));
		if (rrl_result != DNS_RRL_RESULT_OK) {
			/*
			 * Log dropped or slipped responses in the query
			 * category so that requests are not silently lost.
			 * Starts of rate-limited bursts are logged in
			 * DNS_LOGCATEGORY_RRL.
			 *
			 * Dropped responses are counted with dropped queries
			 * in QryDropped while slipped responses are counted
			 * with other truncated responses in RespTruncated.
			 */
			if (wouldlog) {
				ns_client_log(qctx->client, DNS_LOGCATEGORY_RRL,
					      NS_LOGMODULE_QUERY,
					      DNS_RRL_LOG_DROP,
					      "%s", log_buf);
			}

			if (!qctx->client->view->rrl->log_only) {
				if (rrl_result == DNS_RRL_RESULT_DROP) {
					/*
					 * These will also be counted in
					 * ns_statscounter_dropped
					 */
					inc_stats(qctx->client,
						ns_statscounter_ratedropped);
					QUERY_ERROR(qctx, DNS_R_DROP);
				} else {
					/*
					 * These will also be counted in
					 * ns_statscounter_truncatedresp
					 */
					inc_stats(qctx->client,
						ns_statscounter_rateslipped);
					if (WANTCOOKIE(qctx->client)) {
						qctx->client->message->flags &=
							~DNS_MESSAGEFLAG_AA;
						qctx->client->message->flags &=
							~DNS_MESSAGEFLAG_AD;
						qctx->client->message->rcode =
							   dns_rcode_badcookie;
					} else {
						qctx->client->message->flags |=
							DNS_MESSAGEFLAG_TC;
						if (resp_result ==
						    DNS_R_NXDOMAIN)
						{
						  qctx->client->message->rcode =
							  dns_rcode_nxdomain;
						}
					}
				}
				return (DNS_R_DROP);
			}
		}
	} else if (!TCP(qctx->client) &&
		   qctx->client->view->requireservercookie &&
		   WANTCOOKIE(qctx->client) && !HAVECOOKIE(qctx->client))
	{
		qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AA;
		qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AD;
		qctx->client->message->rcode = dns_rcode_badcookie;
		return (DNS_R_DROP);
	}

	return (ISC_R_SUCCESS);
}

/*%
 * Do any RPZ rewriting that may be needed for this query.
 */
static isc_result_t
query_checkrpz(query_ctx_t *qctx, isc_result_t result) {
	isc_result_t rresult;

	rresult = rpz_rewrite(qctx->client, qctx->qtype,
			      result, qctx->resuming,
			      qctx->rdataset, qctx->sigrdataset);
	qctx->rpz_st = qctx->client->query.rpz_st;
	switch (rresult) {
	case ISC_R_SUCCESS:
		break;
	case DNS_R_DISALLOWED:
		return (result);
	case DNS_R_DELEGATION:
		/*
		 * recursing for NS names or addresses,
		 * so save the main query state
		 */
		qctx->rpz_st->q.qtype = qctx->qtype;
		qctx->rpz_st->q.is_zone = qctx->is_zone;
		qctx->rpz_st->q.authoritative = qctx->authoritative;
		SAVE(qctx->rpz_st->q.zone, qctx->zone);
		SAVE(qctx->rpz_st->q.db, qctx->db);
		SAVE(qctx->rpz_st->q.node, qctx->node);
		SAVE(qctx->rpz_st->q.rdataset, qctx->rdataset);
		SAVE(qctx->rpz_st->q.sigrdataset, qctx->sigrdataset);
		dns_name_copy(qctx->fname, qctx->rpz_st->fname, NULL);
		qctx->rpz_st->q.result = result;
		qctx->client->query.attributes |= NS_QUERYATTR_RECURSING;
		return (ISC_R_COMPLETE);
	default:
		RECURSE_ERROR(qctx, rresult);
		return (ISC_R_COMPLETE);;
	}

	if (qctx->rpz_st->m.policy != DNS_RPZ_POLICY_MISS) {
		qctx->rpz_st->state |= DNS_RPZ_REWRITTEN;
	}

	if (qctx->rpz_st->m.policy != DNS_RPZ_POLICY_MISS &&
	    qctx->rpz_st->m.policy != DNS_RPZ_POLICY_PASSTHRU &&
	    (qctx->rpz_st->m.policy != DNS_RPZ_POLICY_TCP_ONLY ||
	     !TCP(qctx->client)) &&
	    qctx->rpz_st->m.policy != DNS_RPZ_POLICY_ERROR)
	{
		/*
		 * We got a hit and are going to answer with our
		 * fiction. Ensure that we answer with the name
		 * we looked up even if we were stopped short
		 * in recursion or for a deferral.
		 */
		rresult = dns_name_copy(qctx->client->query.qname,
					qctx->fname, NULL);
		RUNTIME_CHECK(rresult == ISC_R_SUCCESS);
		rpz_clean(&qctx->zone, &qctx->db, &qctx->node, NULL);
		if (qctx->rpz_st->m.rdataset != NULL) {
			query_putrdataset(qctx->client, &qctx->rdataset);
			RESTORE(qctx->rdataset, qctx->rpz_st->m.rdataset);
		} else {
			qctx_clean(qctx);
		}
		qctx->version = NULL;

		RESTORE(qctx->node, qctx->rpz_st->m.node);
		RESTORE(qctx->db, qctx->rpz_st->m.db);
		RESTORE(qctx->version, qctx->rpz_st->m.version);
		RESTORE(qctx->zone, qctx->rpz_st->m.zone);

		/*
		 * Add SOA record to additional section
		 */
		rresult = query_addsoa(qctx,
			       dns_rdataset_isassociated(qctx->rdataset),
			       DNS_SECTION_ADDITIONAL);
		if (rresult != ISC_R_SUCCESS) {
			QUERY_ERROR(qctx, result);
			return (ISC_R_COMPLETE);
		}

		switch (qctx->rpz_st->m.policy) {
		case DNS_RPZ_POLICY_TCP_ONLY:
			qctx->client->message->flags |= DNS_MESSAGEFLAG_TC;
			if (result == DNS_R_NXDOMAIN ||
					result == DNS_R_NCACHENXDOMAIN)
				qctx->client->message->rcode =
						dns_rcode_nxdomain;
			rpz_log_rewrite(qctx->client, ISC_FALSE,
					qctx->rpz_st->m.policy,
					qctx->rpz_st->m.type, qctx->zone,
					qctx->rpz_st->p_name, NULL,
					qctx->rpz_st->m.rpz->num);
			return (ISC_R_COMPLETE);
		case DNS_RPZ_POLICY_DROP:
			QUERY_ERROR(qctx, DNS_R_DROP);
			rpz_log_rewrite(qctx->client, ISC_FALSE,
					qctx->rpz_st->m.policy,
					qctx->rpz_st->m.type, qctx->zone,
					qctx->rpz_st->p_name, NULL,
					qctx->rpz_st->m.rpz->num);
			return (ISC_R_COMPLETE);
		case DNS_RPZ_POLICY_NXDOMAIN:
			result = DNS_R_NXDOMAIN;
			qctx->nxrewrite = ISC_TRUE;
			qctx->rpz = ISC_TRUE;
			break;
		case DNS_RPZ_POLICY_NODATA:
			result = DNS_R_NXRRSET;
			qctx->nxrewrite = ISC_TRUE;
			qctx->rpz = ISC_TRUE;
			break;
		case DNS_RPZ_POLICY_RECORD:
			result = qctx->rpz_st->m.result;
			if (qctx->qtype == dns_rdatatype_any &&
					result != DNS_R_CNAME) {
				/*
				 * We will add all of the rdatasets of
				 * the node by iterating later,
				 * and set the TTL then.
				 */
				if (dns_rdataset_isassociated(qctx->rdataset))
				      dns_rdataset_disassociate(qctx->rdataset);
			} else {
				/*
				 * We will add this rdataset.
				 */
				qctx->rdataset->ttl =
					ISC_MIN(qctx->rdataset->ttl,
						qctx->rpz_st->m.ttl);
			}
			qctx->rpz = ISC_TRUE;
			break;
		case DNS_RPZ_POLICY_WILDCNAME: {
			dns_rdata_t rdata = DNS_RDATA_INIT;
			dns_rdata_cname_t cname;
			result = dns_rdataset_first(qctx->rdataset);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			dns_rdataset_current(qctx->rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &cname,
						    NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			dns_rdata_reset(&rdata);
			result = query_rpzcname(qctx, &cname.cname);
			if (result != ISC_R_SUCCESS)
				return (ISC_R_COMPLETE);
			qctx->fname = NULL;
			qctx->want_restart = ISC_TRUE;
			return (ISC_R_COMPLETE);
		}
		case DNS_RPZ_POLICY_CNAME:
			/*
			 * Add overridding CNAME from a named.conf
			 * response-policy statement
			 */
			result = query_rpzcname(qctx,
						&qctx->rpz_st->m.rpz->cname);
			if (result != ISC_R_SUCCESS)
				return (ISC_R_COMPLETE);
			qctx->fname = NULL;
			qctx->want_restart = ISC_TRUE;
			return (ISC_R_COMPLETE);
		default:
			INSIST(0);
		}

		/*
		 * Turn off DNSSEC because the results of a
		 * response policy zone cannot verify.
		 */
		qctx->client->attributes &= ~(NS_CLIENTATTR_WANTDNSSEC |
					      NS_CLIENTATTR_WANTAD);
		qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AD;
		query_putrdataset(qctx->client, &qctx->sigrdataset);
		qctx->rpz_st->q.is_zone = qctx->is_zone;
		qctx->is_zone = ISC_TRUE;
		rpz_log_rewrite(qctx->client, ISC_FALSE,
				qctx->rpz_st->m.policy,
				qctx->rpz_st->m.type, qctx->zone,
				qctx->rpz_st->p_name, NULL,
				qctx->rpz_st->m.rpz->num);
	}

	return (result);
}

/*%
 * Add a CNAME to a query response, including translating foo.evil.com and
 *	*.evil.com CNAME *.example.com
 * to
 *	foo.evil.com CNAME foo.evil.com.example.com
 */
static isc_result_t
query_rpzcname(query_ctx_t *qctx, dns_name_t *cname) {
	ns_client_t *client = qctx->client;
	dns_fixedname_t prefix, suffix;
	unsigned int labels;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "query_rpzcname");

	labels = dns_name_countlabels(cname);
	if (labels > 2 && dns_name_iswildcard(cname)) {
		dns_fixedname_init(&prefix);
		dns_name_split(client->query.qname, 1,
			       dns_fixedname_name(&prefix), NULL);
		dns_fixedname_init(&suffix);
		dns_name_split(cname, labels-1,
			       NULL, dns_fixedname_name(&suffix));
		result = dns_name_concatenate(dns_fixedname_name(&prefix),
					      dns_fixedname_name(&suffix),
					      qctx->fname, NULL);
		if (result == DNS_R_NAMETOOLONG) {
			client->message->rcode = dns_rcode_yxdomain;
		} else if (result != ISC_R_SUCCESS) {
			return (result);
		}
	} else {
		result = dns_name_copy(cname, qctx->fname, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	query_keepname(client, qctx->fname, qctx->dbuf);
	result = query_addcname(qctx, dns_trust_authanswer,
				qctx->rpz_st->m.ttl);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	rpz_log_rewrite(client, ISC_FALSE, qctx->rpz_st->m.policy,
			qctx->rpz_st->m.type, qctx->rpz_st->m.zone,
			qctx->rpz_st->p_name, qctx->fname,
			qctx->rpz_st->m.rpz->num);

	ns_client_qnamereplace(client, qctx->fname);

	/*
	 * Turn off DNSSEC because the results of a
	 * response policy zone cannot verify.
	 */
	client->attributes &= ~(NS_CLIENTATTR_WANTDNSSEC |
				NS_CLIENTATTR_WANTAD);

	return (ISC_R_SUCCESS);
}

/*%
 * Check the configured trust anchors for a root zone trust anchor
 * with a key id that matches qctx->client->query.root_key_sentinel_keyid.
 *
 * Return ISC_TRUE when found, otherwise return ISC_FALSE.
 */
static isc_boolean_t
has_ta(query_ctx_t *qctx) {
	dns_keytable_t *keytable = NULL;
	dns_keynode_t *keynode = NULL;
	isc_result_t result;

	result = dns_view_getsecroots(qctx->client->view, &keytable);
	if (result != ISC_R_SUCCESS) {
		return (ISC_FALSE);
	}

	result = dns_keytable_find(keytable, dns_rootname, &keynode);
	while (result == ISC_R_SUCCESS) {
		dns_keynode_t *nextnode = NULL;
		dns_keytag_t keyid = dst_key_id(dns_keynode_key(keynode));
		if (keyid == qctx->client->query.root_key_sentinel_keyid) {
			dns_keytable_detachkeynode(keytable, &keynode);
			dns_keytable_detach(&keytable);
			return (ISC_TRUE);
		}
		result = dns_keytable_nextkeynode(keytable, keynode, &nextnode);
		dns_keytable_detachkeynode(keytable, &keynode);
		keynode = nextnode;
	}
	dns_keytable_detach(&keytable);

	return (ISC_FALSE);
}

/*%
 * Check if a root key sentinel SERVFAIL should be returned.
 */
static isc_boolean_t
root_key_sentinel_return_servfail(query_ctx_t *qctx, isc_result_t result) {
	/*
	 * Are we looking at a "root-key-sentinel" query?
	 */
	if (!qctx->client->query.root_key_sentinel_is_ta &&
	    !qctx->client->query.root_key_sentinel_not_ta)
	{
		return (ISC_FALSE);
	}

	/*
	 * We only care about the query if 'result' indicates we have a cached
	 * answer.
	 */
	switch (result) {
	case ISC_R_SUCCESS:
	case DNS_R_CNAME:
	case DNS_R_DNAME:
	case DNS_R_NCACHENXDOMAIN:
	case DNS_R_NCACHENXRRSET:
		break;
	default:
		return (ISC_FALSE);
	}

	/*
	 * Do we meet the specified conditions to return SERVFAIL?
	 */
	if (!qctx->is_zone &&
	    qctx->rdataset->trust == dns_trust_secure &&
	    ((qctx->client->query.root_key_sentinel_is_ta && !has_ta(qctx)) ||
	     (qctx->client->query.root_key_sentinel_not_ta && has_ta(qctx))))
	{
		return (ISC_TRUE);
	}

	/*
	 * As special processing may only be triggered by the original QNAME,
	 * disable it after following a CNAME/DNAME.
	 */
	qctx->client->query.root_key_sentinel_is_ta = ISC_FALSE;
	qctx->client->query.root_key_sentinel_not_ta = ISC_FALSE;

	return (ISC_FALSE);
}

/*%
 * Continue after doing a database lookup or returning from
 * recursion, and call out to the next function depending on the
 * result from the search.
 */
static isc_result_t
query_gotanswer(query_ctx_t *qctx, isc_result_t result) {
	char errmsg[256];

	CCTRACE(ISC_LOG_DEBUG(3), "query_gotanswer");

	if (query_checkrrl(qctx, result) != ISC_R_SUCCESS) {
		return (query_done(qctx));
	}

	if (!RECURSING(qctx->client) &&
	    !dns_name_equal(qctx->client->query.qname, dns_rootname))
	{
		result = query_checkrpz(qctx, result);
		if (result == ISC_R_COMPLETE)
			return (query_done(qctx));
	}

	/*
	 * If required, handle special "root-key-sentinel-is-ta-<keyid>" and
	 * "root-key-sentinel-not-ta-<keyid>" labels by returning SERVFAIL.
	 */
	if (root_key_sentinel_return_servfail(qctx, result)) {
		/*
		 * Don't record this response in the SERVFAIL cache.
		 */
		qctx->client->attributes |= NS_CLIENTATTR_NOSETFC;
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	switch (result) {
	case ISC_R_SUCCESS:
		return (query_prepresponse(qctx));

	case DNS_R_GLUE:
	case DNS_R_ZONECUT:
		INSIST(qctx->is_zone);
		qctx->authoritative = ISC_FALSE;
		return (query_prepresponse(qctx));

	case ISC_R_NOTFOUND:
		return (query_notfound(qctx));

	case DNS_R_DELEGATION:
		return (query_delegation(qctx));

	case DNS_R_EMPTYNAME:
		return (query_nodata(qctx, DNS_R_EMPTYNAME));
	case DNS_R_NXRRSET:
		return (query_nodata(qctx, DNS_R_NXRRSET));

	case DNS_R_EMPTYWILD:
		return (query_nxdomain(qctx, ISC_TRUE));

	case DNS_R_NXDOMAIN:
		return (query_nxdomain(qctx, ISC_FALSE));

	case DNS_R_COVERINGNSEC:
		return (query_coveringnsec(qctx));

	case DNS_R_NCACHENXDOMAIN:
		result = query_redirect(qctx);
		if (result != ISC_R_COMPLETE)
			return (result);
		return (query_ncache(qctx, DNS_R_NCACHENXDOMAIN));

	case DNS_R_NCACHENXRRSET:
		return (query_ncache(qctx, DNS_R_NCACHENXRRSET));

	case DNS_R_CNAME:
		return (query_cname(qctx));

	case DNS_R_DNAME:
		return (query_dname(qctx));

	default:
		/*
		 * Something has gone wrong.
		 */
		snprintf(errmsg, sizeof(errmsg) - 1,
			 "query_gotanswer: unexpected error: %s",
			 isc_result_totext(result));
		CCTRACE(ISC_LOG_ERROR, errmsg);
		if (qctx->resuming) {
			qctx->want_stale = ISC_TRUE;
		} else {
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		}
		return (query_done(qctx));
	}
}

static void
query_addnoqnameproof(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	isc_buffer_t *dbuf, b;
	dns_name_t *fname = NULL;
	dns_rdataset_t *neg = NULL, *negsig = NULL;
	isc_result_t result = ISC_R_NOMEMORY;

	CTRACE(ISC_LOG_DEBUG(3), "query_addnoqnameproof");

	if (qctx->noqname == NULL) {
		return;
	}

	dbuf = query_getnamebuf(client);
	if (dbuf == NULL) {
		goto cleanup;
	}

	fname = query_newname(client, dbuf, &b);
	neg = query_newrdataset(client);
	negsig = query_newrdataset(client);
	if (fname == NULL || neg == NULL || negsig == NULL) {
		goto cleanup;
	}

	result = dns_rdataset_getnoqname(qctx->noqname, fname, neg, negsig);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	query_addrrset(client, &fname, &neg, &negsig, dbuf,
		       DNS_SECTION_AUTHORITY);

	if ((qctx->noqname->attributes & DNS_RDATASETATTR_CLOSEST) == 0) {
		goto cleanup;
	}

	if (fname == NULL) {
		dbuf = query_getnamebuf(client);
		if (dbuf == NULL)
			goto cleanup;
		fname = query_newname(client, dbuf, &b);
	}

	if (neg == NULL) {
		neg = query_newrdataset(client);
	} else if (dns_rdataset_isassociated(neg)) {
		dns_rdataset_disassociate(neg);
	}

	if (negsig == NULL) {
		negsig = query_newrdataset(client);
	} else if (dns_rdataset_isassociated(negsig)) {
		dns_rdataset_disassociate(negsig);
	}

	if (fname == NULL || neg == NULL || negsig == NULL)
		goto cleanup;
	result = dns_rdataset_getclosest(qctx->noqname, fname, neg, negsig);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	query_addrrset(client, &fname, &neg, &negsig, dbuf,
		       DNS_SECTION_AUTHORITY);

 cleanup:
	if (neg != NULL) {
		query_putrdataset(client, &neg);
	}
	if (negsig != NULL) {
		query_putrdataset(client, &negsig);
	}
	if (fname != NULL) {
		query_releasename(client, &fname);
	}
}

/*%
 * Build the response for a query for type ANY.
 */
static isc_result_t
query_respond_any(query_ctx_t *qctx) {
	dns_name_t *tname;
	int rdatasets_found = 0;
	dns_rdatasetiter_t *rdsiter = NULL;
	isc_result_t result;
	dns_rdatatype_t onetype = 0; 	/* type to use for minimal-any */
	isc_boolean_t have_aaaa, have_a, have_sig;

	/*
	 * If we are not authoritative, assume there is an A record
	 * even in if it is not in our cache.  This assumption could
	 * be wrong but it is a good bet.
	 */
	have_aaaa = ISC_FALSE;
	have_a = !qctx->authoritative;
	have_sig = ISC_FALSE;

	result = dns_db_allrdatasets(qctx->db, qctx->node,
				     qctx->version, 0, &rdsiter);
	if (result != ISC_R_SUCCESS) {
		CCTRACE(ISC_LOG_ERROR,
		       "query_respond_any: allrdatasets failed");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	/*
	 * Calling query_addrrset() with a non-NULL dbuf is going
	 * to either keep or release the name.  We don't want it to
	 * release fname, since we may have to call query_addrrset()
	 * more than once.  That means we have to call query_keepname()
	 * now, and pass a NULL dbuf to query_addrrset().
	 *
	 * If we do a query_addrrset() below, we must set qctx->fname to
	 * NULL before leaving this block, otherwise we might try to
	 * cleanup qctx->fname even though we're using it!
	 */
	query_keepname(qctx->client, qctx->fname, qctx->dbuf);
	tname = qctx->fname;

	result = dns_rdatasetiter_first(rdsiter);
	while (result == ISC_R_SUCCESS) {
		dns_rdatasetiter_current(rdsiter, qctx->rdataset);
		/*
		 * Notice the presence of A and AAAAs so
		 * that AAAAs can be hidden from IPv4 clients.
		 */
		if (qctx->client->filter_aaaa != dns_aaaa_ok) {
			if (qctx->rdataset->type == dns_rdatatype_aaaa)
				have_aaaa = ISC_TRUE;
			else if (qctx->rdataset->type == dns_rdatatype_a)
				have_a = ISC_TRUE;
		}

		/*
		 * We found an NS RRset; no need to add one later.
		 */
		if (qctx->qtype == dns_rdatatype_any &&
		    qctx->rdataset->type == dns_rdatatype_ns)
		{
			qctx->answer_has_ns = ISC_TRUE;
		}

		/*
		 * Note: if we're in this function, then qctx->type
		 * is guaranteed to be ANY, but qctx->qtype (i.e. the
		 * original type requested) might have been RRSIG or
		 * SIG; we need to check for that.
		 */
		if (qctx->is_zone && qctx->qtype == dns_rdatatype_any &&
		    !dns_db_issecure(qctx->db) &&
		    dns_rdatatype_isdnssec(qctx->rdataset->type))
		{
			/*
			 * The zone is transitioning from insecure
			 * to secure. Hide the dnssec records from
			 * ANY queries.
			 */
			dns_rdataset_disassociate(qctx->rdataset);
		} else if (qctx->client->view->minimal_any &&
			   !TCP(qctx->client) && !WANTDNSSEC(qctx->client) &&
			   qctx->qtype == dns_rdatatype_any &&
			   (qctx->rdataset->type == dns_rdatatype_sig ||
			    qctx->rdataset->type == dns_rdatatype_rrsig))
		{
			CCTRACE(ISC_LOG_DEBUG(5), "query_respond_any: "
			       "minimal-any skip signature");
			dns_rdataset_disassociate(qctx->rdataset);
		} else if (qctx->client->view->minimal_any &&
			   !TCP(qctx->client) && onetype != 0 &&
			   qctx->rdataset->type != onetype &&
			   qctx->rdataset->covers != onetype)
		{
			CCTRACE(ISC_LOG_DEBUG(5), "query_respond_any: "
			       "minimal-any skip rdataset");
			dns_rdataset_disassociate(qctx->rdataset);
		} else if ((qctx->qtype == dns_rdatatype_any ||
			    qctx->rdataset->type == qctx->qtype) &&
			   qctx->rdataset->type != 0)
		{
			if (dns_rdatatype_isdnssec(qctx->rdataset->type))
				have_sig = ISC_TRUE;

			if (NOQNAME(qctx->rdataset) && WANTDNSSEC(qctx->client))
			{
				qctx->noqname = qctx->rdataset;
			} else {
				qctx->noqname = NULL;
			}

			qctx->rpz_st = qctx->client->query.rpz_st;
			if (qctx->rpz_st != NULL)
				qctx->rdataset->ttl =
					ISC_MIN(qctx->rdataset->ttl,
						qctx->rpz_st->m.ttl);

			if (!qctx->is_zone && RECURSIONOK(qctx->client)) {
				dns_name_t *name;
				name = (qctx->fname != NULL)
					? qctx->fname
					: tname;
				query_prefetch(qctx->client, name,
					       qctx->rdataset);
			}

			/*
			 * Remember the first RRtype we find so we
			 * can skip others with minimal-any.
			 */
			if (qctx->rdataset->type == dns_rdatatype_sig ||
			    qctx->rdataset->type == dns_rdatatype_rrsig)
				onetype = qctx->rdataset->covers;
			else
				onetype = qctx->rdataset->type;

			query_addrrset(qctx->client,
				       (qctx->fname != NULL)
					? &qctx->fname
					: &tname,
				       &qctx->rdataset, NULL,
				       NULL, DNS_SECTION_ANSWER);

			query_addnoqnameproof(qctx);

			rdatasets_found++;
			INSIST(tname != NULL);

			/*
			 * rdataset is non-NULL only in certain
			 * pathological cases involving DNAMEs.
			 */
			if (qctx->rdataset != NULL)
				query_putrdataset(qctx->client,
						  &qctx->rdataset);

			qctx->rdataset = query_newrdataset(qctx->client);
			if (qctx->rdataset == NULL)
				break;
		} else {
			/*
			 * We're not interested in this rdataset.
			 */
			dns_rdataset_disassociate(qctx->rdataset);
		}

		result = dns_rdatasetiter_next(rdsiter);
	}

	/*
	 * Filter AAAAs if there is an A and there is no signature
	 * or we are supposed to break DNSSEC.
	 */
	if (qctx->client->filter_aaaa == dns_aaaa_break_dnssec)
		qctx->client->attributes |= NS_CLIENTATTR_FILTER_AAAA;
	else if (qctx->client->filter_aaaa != dns_aaaa_ok &&
		 have_aaaa && have_a &&
		 (!have_sig || !WANTDNSSEC(qctx->client)))
		  qctx->client->attributes |= NS_CLIENTATTR_FILTER_AAAA;

	if (qctx->fname != NULL)
		dns_message_puttempname(qctx->client->message, &qctx->fname);

	if (rdatasets_found == 0) {
		/*
		 * No matching rdatasets found in cache. If we were
		 * searching for RRSIG/SIG, that's probably okay;
		 * otherwise this is an error condition.
		 */
		if ((qctx->qtype == dns_rdatatype_rrsig ||
		     qctx->qtype == dns_rdatatype_sig) &&
		    result == ISC_R_NOMORE)
		{
			isc_buffer_t b;
			if (!qctx->is_zone) {
				qctx->authoritative = ISC_FALSE;
				dns_rdatasetiter_destroy(&rdsiter);
				qctx->client->attributes &= ~NS_CLIENTATTR_RA;
				query_addauth(qctx);
				return (query_done(qctx));
			}

			if (qctx->qtype == dns_rdatatype_rrsig &&
			    dns_db_issecure(qctx->db)) {
				char namebuf[DNS_NAME_FORMATSIZE];
				dns_name_format(qctx->client->query.qname,
						namebuf,
						sizeof(namebuf));
				ns_client_log(qctx->client,
					      DNS_LOGCATEGORY_DNSSEC,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_WARNING,
					      "missing signature for %s",
					      namebuf);
			}

			dns_rdatasetiter_destroy(&rdsiter);
			qctx->fname = query_newname(qctx->client,
						    qctx->dbuf, &b);
			return (query_sign_nodata(qctx));
		} else {
			CCTRACE(ISC_LOG_ERROR,
			       "query_respond_any: "
			       "no matching rdatasets in cache");
			result = DNS_R_SERVFAIL;
		}
	}

	dns_rdatasetiter_destroy(&rdsiter);
	if (result != ISC_R_NOMORE) {
		CCTRACE(ISC_LOG_ERROR,
		       "query_respond_any: dns_rdatasetiter_destroy failed");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
	} else {
		query_addauth(qctx);
	}

	return (query_done(qctx));
}

/*
 * Set the expire time, if requested, when answering from a
 * slave or master zone.
 */
static void
query_getexpire(query_ctx_t *qctx) {
	dns_zone_t *raw = NULL, *mayberaw;

	if (qctx->zone == NULL || !qctx->is_zone ||
	    qctx->qtype != dns_rdatatype_soa ||
	    qctx->client->query.restarts != 0 ||
	    (qctx->client->attributes & NS_CLIENTATTR_WANTEXPIRE) == 0)
	{
		return;
	}

	dns_zone_getraw(qctx->zone, &raw);
	mayberaw = (raw != NULL) ? raw : qctx->zone;

	if (dns_zone_gettype(mayberaw) == dns_zone_slave) {
		isc_time_t expiretime;
		isc_uint32_t secs;
		dns_zone_getexpiretime(qctx->zone, &expiretime);
		secs = isc_time_seconds(&expiretime);
		if (secs >= qctx->client->now &&
		    qctx->result == ISC_R_SUCCESS)
		{
			qctx->client->attributes |=
				NS_CLIENTATTR_HAVEEXPIRE;
			qctx->client->expire = secs - qctx->client->now;
		}
	} else if (dns_zone_gettype(mayberaw) == dns_zone_master) {
		isc_result_t result;
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_soa_t soa;

		result = dns_rdataset_first(qctx->rdataset);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		dns_rdataset_current(qctx->rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &soa, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		qctx->client->expire = soa.expire;
		qctx->client->attributes |= NS_CLIENTATTR_HAVEEXPIRE;
	}

	if (raw != NULL) {
		dns_zone_detach(&raw);
	}
}

/*
 * Optionally hide AAAAs from IPv4 clients if there is an A.
 *
 * We add the AAAAs now, but might refuse to render them later
 * after DNSSEC is figured out.
 *
 * This could be more efficient, but the whole idea is
 * so fundamentally wrong, unavoidably inaccurate, and
 * unneeded that it is best to keep it as short as possible.
 */
static isc_result_t
query_filter_aaaa(query_ctx_t *qctx) {
	isc_result_t result;

	if (qctx->client->filter_aaaa != dns_aaaa_break_dnssec &&
	    (qctx->client->filter_aaaa != dns_aaaa_filter ||
	     (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL &&
	      dns_rdataset_isassociated(qctx->sigrdataset))))
	{
		return (ISC_R_COMPLETE);
	}

	if (qctx->qtype == dns_rdatatype_aaaa) {
		dns_rdataset_t *trdataset;
		trdataset = query_newrdataset(qctx->client);
		result = dns_db_findrdataset(qctx->db, qctx->node,
					     qctx->version,
					     dns_rdatatype_a, 0,
					     qctx->client->now,
					     trdataset, NULL);
		if (dns_rdataset_isassociated(trdataset)) {
			dns_rdataset_disassociate(trdataset);
		}
		query_putrdataset(qctx->client, &trdataset);

		/*
		 * We have an AAAA but the A is not in our cache.
		 * Assume any result other than DNS_R_DELEGATION
		 * or ISC_R_NOTFOUND means there is no A and
		 * so AAAAs are ok.
		 *
		 * Assume there is no A if we can't recurse
		 * for this client, although that could be
		 * the wrong answer. What else can we do?
		 * Besides, that we have the AAAA and are using
		 * this mechanism suggests that we care more
		 * about As than AAAAs and would have cached
		 * the A if it existed.
		 */
		if (result == ISC_R_SUCCESS) {
			qctx->client->attributes |=
				    NS_CLIENTATTR_FILTER_AAAA;

		} else if (qctx->authoritative ||
			   !RECURSIONOK(qctx->client) ||
			   (result != DNS_R_DELEGATION &&
			    result != ISC_R_NOTFOUND))
		{
			qctx->client->attributes &=
				~NS_CLIENTATTR_FILTER_AAAA;
		} else {
			/*
			 * This is an ugly kludge to recurse
			 * for the A and discard the result.
			 *
			 * Continue to add the AAAA now.
			 * We'll make a note to not render it
			 * if the recursion for the A succeeds.
			 */
			INSIST(!REDIRECT(qctx->client));
			result = query_recurse(qctx->client,
					dns_rdatatype_a,
					qctx->client->query.qname,
					NULL, NULL, qctx->resuming);
			if (result == ISC_R_SUCCESS) {
				qctx->client->attributes |=
					NS_CLIENTATTR_FILTER_AAAA_RC;
				qctx->client->query.attributes |=
					NS_QUERYATTR_RECURSING;
			}
		}
	} else if (qctx->qtype == dns_rdatatype_a &&
		   (qctx->client->attributes &
		    NS_CLIENTATTR_FILTER_AAAA_RC) != 0)
	{
		qctx->client->attributes &= ~NS_CLIENTATTR_FILTER_AAAA_RC;
		qctx->client->attributes |= NS_CLIENTATTR_FILTER_AAAA;
		qctx_clean(qctx);

		return (query_done(qctx));
	}

	return (ISC_R_COMPLETE);
}

/*%
 * Build a repsonse for a "normal" query, for a type other than ANY,
 * for which we have an answer (either positive or negative).
 */
static isc_result_t
query_respond(query_ctx_t *qctx) {
	dns_rdataset_t **sigrdatasetp = NULL;
	isc_result_t result;

	/*
	 * If we have a zero ttl from the cache, refetch.
	 */
	if (!qctx->is_zone && !qctx->resuming && !STALE(qctx->rdataset) &&
	    qctx->rdataset->ttl == 0 && RECURSIONOK(qctx->client))
	{
		qctx_clean(qctx);

		INSIST(!REDIRECT(qctx->client));
		result = query_recurse(qctx->client, qctx->qtype,
				       qctx->client->query.qname,
				       NULL, NULL, qctx->resuming);
		if (result == ISC_R_SUCCESS) {
			qctx->client->query.attributes |=
				NS_QUERYATTR_RECURSING;
			if (qctx->dns64)
				qctx->client->query.attributes |=
					NS_QUERYATTR_DNS64;
			if (qctx->dns64_exclude)
				qctx->client->query.attributes |=
				      NS_QUERYATTR_DNS64EXCLUDE;
		} else {
			RECURSE_ERROR(qctx, result);
		}

		return (query_done(qctx));
	}

	result = query_filter_aaaa(qctx);
	if (result != ISC_R_COMPLETE)
		return (result);
	/*
	 * Check to see if the AAAA RRset has non-excluded addresses
	 * in it.  If not look for a A RRset.
	 */
	INSIST(qctx->client->query.dns64_aaaaok == NULL);

	if (qctx->qtype == dns_rdatatype_aaaa && !qctx->dns64_exclude &&
	    !ISC_LIST_EMPTY(qctx->client->view->dns64) &&
	    qctx->client->message->rdclass == dns_rdataclass_in &&
	    !dns64_aaaaok(qctx->client, qctx->rdataset, qctx->sigrdataset))
	{
		/*
		 * Look to see if there are A records for this name.
		 */
		qctx->client->query.dns64_ttl = qctx->rdataset->ttl;
		SAVE(qctx->client->query.dns64_aaaa, qctx->rdataset);
		SAVE(qctx->client->query.dns64_sigaaaa, qctx->sigrdataset);
		query_releasename(qctx->client, &qctx->fname);
		dns_db_detachnode(qctx->db, &qctx->node);
		qctx->type = qctx->qtype = dns_rdatatype_a;
		qctx->dns64_exclude = qctx->dns64 = ISC_TRUE;

		return (query_lookup(qctx));
	}

	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL) {
		sigrdatasetp = &qctx->sigrdataset;
	}

	if (NOQNAME(qctx->rdataset) && WANTDNSSEC(qctx->client)) {
		qctx->noqname = qctx->rdataset;
	} else {
		qctx->noqname = NULL;
	}

	/*
	 * Special case NS handling
	 */
	if (qctx->is_zone && qctx->qtype == dns_rdatatype_ns) {
		/*
		 * We've already got an NS, no need to add one in
		 * the authority section
		 */
		if (dns_name_equal(qctx->client->query.qname,
				   dns_db_origin(qctx->db)))
		{
			qctx->answer_has_ns = ISC_TRUE;
		}

		/*
		 * BIND 8 priming queries need the additional section.
		 */
		if (dns_name_equal(qctx->client->query.qname, dns_rootname)) {
			qctx->client->query.attributes &=
				~NS_QUERYATTR_NOADDITIONAL;
		}
	}

	/*
	 * Set expire time
	 */
	query_getexpire(qctx);

	if (qctx->dns64) {
		result = query_dns64(qctx);
		qctx->noqname = NULL;
		dns_rdataset_disassociate(qctx->rdataset);
		dns_message_puttemprdataset(qctx->client->message,
					    &qctx->rdataset);
		if (result == ISC_R_NOMORE) {
#ifndef dns64_bis_return_excluded_addresses
			if (qctx->dns64_exclude) {
				if (!qctx->is_zone)
					return (query_done(qctx));
				/*
				 * Add a fake SOA record.
				 */
				(void)query_addsoa(qctx, 600,
						   DNS_SECTION_AUTHORITY);
				return (query_done(qctx));
			}
#endif
			if (qctx->is_zone) {
				return (query_nodata(qctx, DNS_R_NXDOMAIN));
			} else {
				return (query_ncache(qctx, DNS_R_NXDOMAIN));
			}
		} else if (result != ISC_R_SUCCESS) {
			qctx->result = result;
			return (query_done(qctx));
		}
	} else if (qctx->client->query.dns64_aaaaok != NULL) {
		query_filter64(qctx);
		query_putrdataset(qctx->client, &qctx->rdataset);
	} else {
		if (!qctx->is_zone && RECURSIONOK(qctx->client))
			query_prefetch(qctx->client, qctx->fname,
				       qctx->rdataset);
		query_addrrset(qctx->client, &qctx->fname,
			       &qctx->rdataset, sigrdatasetp,
			       qctx->dbuf, DNS_SECTION_ANSWER);
	}

	query_addnoqnameproof(qctx);

	/*
	 * We shouldn't ever fail to add 'rdataset'
	 * because it's already in the answer.
	 */
	INSIST(qctx->rdataset == NULL);

	query_addauth(qctx);

	return (query_done(qctx));
}

static isc_result_t
query_dns64(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(client->interface->mgr);
	dns_name_t *name, *mname;
	dns_rdata_t *dns64_rdata;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t *dns64_rdatalist;
	dns_rdataset_t *dns64_rdataset;
	dns_rdataset_t *mrdataset;
	isc_buffer_t *buffer;
	isc_region_t r;
	isc_result_t result;
	dns_view_t *view = client->view;
	isc_netaddr_t netaddr;
	dns_dns64_t *dns64;
	unsigned int flags = 0;
	const dns_section_t section = DNS_SECTION_ANSWER;

	/*%
	 * To the current response for 'qctx->client', add the answer RRset
	 * '*rdatasetp' and an optional signature set '*sigrdatasetp', with
	 * owner name '*namep', to the answer section, unless they are
	 * already there.  Also add any pertinent additional data.
	 *
	 * If 'qctx->dbuf' is not NULL, then 'qctx->fname' is the name
	 * whose data is stored 'qctx->dbuf'.  In this case,
	 * query_addrrset() guarantees that when it returns the name
	 * will either have been kept or released.
	 */
	CTRACE(ISC_LOG_DEBUG(3), "query_dns64");

	qctx->qtype = qctx->type = dns_rdatatype_aaaa;

	name = qctx->fname;
	mname = NULL;
	mrdataset = NULL;
	buffer = NULL;
	dns64_rdata = NULL;
	dns64_rdataset = NULL;
	dns64_rdatalist = NULL;
	result = dns_message_findname(client->message, section,
				      name, dns_rdatatype_aaaa,
				      qctx->rdataset->covers,
				      &mname, &mrdataset);
	if (result == ISC_R_SUCCESS) {
		/*
		 * We've already got an RRset of the given name and type.
		 * There's nothing else to do;
		 */
		CTRACE(ISC_LOG_DEBUG(3),
		       "query_dns64: dns_message_findname succeeded: done");
		if (qctx->dbuf != NULL)
			query_releasename(client, &qctx->fname);
		return (ISC_R_SUCCESS);
	} else if (result == DNS_R_NXDOMAIN) {
		/*
		 * The name doesn't exist.
		 */
		if (qctx->dbuf != NULL)
			query_keepname(client, name, qctx->dbuf);
		dns_message_addname(client->message, name, section);
		qctx->fname = NULL;
		mname = name;
	} else {
		RUNTIME_CHECK(result == DNS_R_NXRRSET);
		if (qctx->dbuf != NULL)
			query_releasename(client, &qctx->fname);
	}

	if (qctx->rdataset->trust != dns_trust_secure) {
		client->query.attributes &= ~NS_QUERYATTR_SECURE;
	}

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);

	result = isc_buffer_allocate(client->mctx, &buffer,
				     view->dns64cnt * 16 *
				     dns_rdataset_count(qctx->rdataset));
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_gettemprdataset(client->message,
					     &dns64_rdataset);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_gettemprdatalist(client->message,
					      &dns64_rdatalist);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	dns_rdatalist_init(dns64_rdatalist);
	dns64_rdatalist->rdclass = dns_rdataclass_in;
	dns64_rdatalist->type = dns_rdatatype_aaaa;
	if (client->query.dns64_ttl != ISC_UINT32_MAX)
		dns64_rdatalist->ttl = ISC_MIN(qctx->rdataset->ttl,
					       client->query.dns64_ttl);
	else
		dns64_rdatalist->ttl = ISC_MIN(qctx->rdataset->ttl, 600);

	if (RECURSIONOK(client))
		flags |= DNS_DNS64_RECURSIVE;

	/*
	 * We use the signatures from the A lookup to set DNS_DNS64_DNSSEC
	 * as this provides a easy way to see if the answer was signed.
	 */
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL &&
	    dns_rdataset_isassociated(qctx->sigrdataset))
		flags |= DNS_DNS64_DNSSEC;

	for (result = dns_rdataset_first(qctx->rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(qctx->rdataset)) {
		for (dns64 = ISC_LIST_HEAD(client->view->dns64);
		     dns64 != NULL; dns64 = dns_dns64_next(dns64)) {

			dns_rdataset_current(qctx->rdataset, &rdata);
			isc_buffer_availableregion(buffer, &r);
			INSIST(r.length >= 16);
			result = dns_dns64_aaaafroma(dns64, &netaddr,
						     client->signer, env, flags,
						     rdata.data, r.base);
			if (result != ISC_R_SUCCESS) {
				dns_rdata_reset(&rdata);
				continue;
			}
			isc_buffer_add(buffer, 16);
			isc_buffer_remainingregion(buffer, &r);
			isc_buffer_forward(buffer, 16);
			result = dns_message_gettemprdata(client->message,
							  &dns64_rdata);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
			dns_rdata_init(dns64_rdata);
			dns_rdata_fromregion(dns64_rdata, dns_rdataclass_in,
					     dns_rdatatype_aaaa, &r);
			ISC_LIST_APPEND(dns64_rdatalist->rdata, dns64_rdata,
					link);
			dns64_rdata = NULL;
			dns_rdata_reset(&rdata);
		}
	}
	if (result != ISC_R_NOMORE)
		goto cleanup;

	if (ISC_LIST_EMPTY(dns64_rdatalist->rdata))
		goto cleanup;

	result = dns_rdatalist_tordataset(dns64_rdatalist, dns64_rdataset);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	dns_rdataset_setownercase(dns64_rdataset, mname);
	client->query.attributes |= NS_QUERYATTR_NOADDITIONAL;
	dns64_rdataset->trust = qctx->rdataset->trust;
	query_addrdataset(client, section, mname, dns64_rdataset);
	dns64_rdataset = NULL;
	dns64_rdatalist = NULL;
	dns_message_takebuffer(client->message, &buffer);
	inc_stats(client, ns_statscounter_dns64);
	result = ISC_R_SUCCESS;

 cleanup:
	if (buffer != NULL)
		isc_buffer_free(&buffer);

	if (dns64_rdata != NULL)
		dns_message_puttemprdata(client->message, &dns64_rdata);

	if (dns64_rdataset != NULL)
		dns_message_puttemprdataset(client->message, &dns64_rdataset);

	if (dns64_rdatalist != NULL) {
		for (dns64_rdata = ISC_LIST_HEAD(dns64_rdatalist->rdata);
		     dns64_rdata != NULL;
		     dns64_rdata = ISC_LIST_HEAD(dns64_rdatalist->rdata))
		{
			ISC_LIST_UNLINK(dns64_rdatalist->rdata,
					dns64_rdata, link);
			dns_message_puttemprdata(client->message, &dns64_rdata);
		}
		dns_message_puttemprdatalist(client->message, &dns64_rdatalist);
	}

	CTRACE(ISC_LOG_DEBUG(3), "query_dns64: done");
	return (result);
}

static void
query_filter64(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	dns_name_t *name, *mname;
	dns_rdata_t *myrdata;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdatalist_t *myrdatalist;
	dns_rdataset_t *myrdataset;
	isc_buffer_t *buffer;
	isc_region_t r;
	isc_result_t result;
	unsigned int i;
	const dns_section_t section = DNS_SECTION_ANSWER;

	CTRACE(ISC_LOG_DEBUG(3), "query_filter64");

	INSIST(client->query.dns64_aaaaok != NULL);
	INSIST(client->query.dns64_aaaaoklen ==
	       dns_rdataset_count(qctx->rdataset));

	name = qctx->fname;
	mname = NULL;
	buffer = NULL;
	myrdata = NULL;
	myrdataset = NULL;
	myrdatalist = NULL;
	result = dns_message_findname(client->message, section,
				      name, dns_rdatatype_aaaa,
				      qctx->rdataset->covers,
				      &mname, &myrdataset);
	if (result == ISC_R_SUCCESS) {
		/*
		 * We've already got an RRset of the given name and type.
		 * There's nothing else to do;
		 */
		CTRACE(ISC_LOG_DEBUG(3),
		       "query_filter64: dns_message_findname succeeded: done");
		if (qctx->dbuf != NULL)
			query_releasename(client, &qctx->fname);
		return;
	} else if (result == DNS_R_NXDOMAIN) {
		mname = name;
		qctx->fname = NULL;
	} else {
		RUNTIME_CHECK(result == DNS_R_NXRRSET);
		if (qctx->dbuf != NULL)
			query_releasename(client, &qctx->fname);
		qctx->dbuf = NULL;
	}

	if (qctx->rdataset->trust != dns_trust_secure) {
		client->query.attributes &= ~NS_QUERYATTR_SECURE;
	}

	result = isc_buffer_allocate(client->mctx, &buffer,
				     16 * dns_rdataset_count(qctx->rdataset));
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_gettemprdataset(client->message, &myrdataset);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_gettemprdatalist(client->message, &myrdatalist);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	dns_rdatalist_init(myrdatalist);
	myrdatalist->rdclass = dns_rdataclass_in;
	myrdatalist->type = dns_rdatatype_aaaa;
	myrdatalist->ttl = qctx->rdataset->ttl;

	i = 0;
	for (result = dns_rdataset_first(qctx->rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(qctx->rdataset)) {
		if (!client->query.dns64_aaaaok[i++])
			continue;
		dns_rdataset_current(qctx->rdataset, &rdata);
		INSIST(rdata.length == 16);
		isc_buffer_putmem(buffer, rdata.data, rdata.length);
		isc_buffer_remainingregion(buffer, &r);
		isc_buffer_forward(buffer, rdata.length);
		result = dns_message_gettemprdata(client->message, &myrdata);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		dns_rdata_init(myrdata);
		dns_rdata_fromregion(myrdata, dns_rdataclass_in,
				     dns_rdatatype_aaaa, &r);
		ISC_LIST_APPEND(myrdatalist->rdata, myrdata, link);
		myrdata = NULL;
		dns_rdata_reset(&rdata);
	}
	if (result != ISC_R_NOMORE)
		goto cleanup;

	result = dns_rdatalist_tordataset(myrdatalist, myrdataset);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	dns_rdataset_setownercase(myrdataset, name);
	client->query.attributes |= NS_QUERYATTR_NOADDITIONAL;
	if (mname == name) {
		if (qctx->dbuf != NULL)
			query_keepname(client, name, qctx->dbuf);
		dns_message_addname(client->message, name,
				    section);
		qctx->dbuf = NULL;
	}
	myrdataset->trust = qctx->rdataset->trust;
	query_addrdataset(client, section, mname, myrdataset);
	myrdataset = NULL;
	myrdatalist = NULL;
	dns_message_takebuffer(client->message, &buffer);

 cleanup:
	if (buffer != NULL)
		isc_buffer_free(&buffer);

	if (myrdata != NULL)
		dns_message_puttemprdata(client->message, &myrdata);

	if (myrdataset != NULL)
		dns_message_puttemprdataset(client->message, &myrdataset);

	if (myrdatalist != NULL) {
		for (myrdata = ISC_LIST_HEAD(myrdatalist->rdata);
		     myrdata != NULL;
		     myrdata = ISC_LIST_HEAD(myrdatalist->rdata))
		{
			ISC_LIST_UNLINK(myrdatalist->rdata, myrdata, link);
			dns_message_puttemprdata(client->message, &myrdata);
		}
		dns_message_puttemprdatalist(client->message, &myrdatalist);
	}
	if (qctx->dbuf != NULL)
		query_releasename(client, &name);

	CTRACE(ISC_LOG_DEBUG(3), "query_filter64: done");
}

/*%
 * Handle the case of a name not being found in a database lookup.
 * Called from query_gotanswer(). Passes off processing to
 * query_delegation() for a root referral if appropriate.
 */
static isc_result_t
query_notfound(query_ctx_t *qctx) {
	isc_result_t result;

	INSIST(!qctx->is_zone);

	if (qctx->db != NULL)
		dns_db_detach(&qctx->db);

	/*
	 * If the cache doesn't even have the root NS,
	 * try to get that from the hints DB.
	 */
	if (qctx->client->view->hints != NULL) {
		dns_clientinfomethods_t cm;
		dns_clientinfo_t ci;

		dns_clientinfomethods_init(&cm, ns_client_sourceip);
		dns_clientinfo_init(&ci, qctx->client, NULL);

		dns_db_attach(qctx->client->view->hints, &qctx->db);
		result = dns_db_findext(qctx->db, dns_rootname,
					NULL, dns_rdatatype_ns,
					0, qctx->client->now, &qctx->node,
					qctx->fname, &cm, &ci,
					qctx->rdataset, qctx->sigrdataset);
	} else {
		/* We have no hints. */
		result = ISC_R_FAILURE;
	}
	if (result != ISC_R_SUCCESS) {
		/*
		 * Nonsensical root hints may require cleanup.
		 */
		qctx_clean(qctx);

		/*
		 * We don't have any root server hints, but
		 * we may have working forwarders, so try to
		 * recurse anyway.
		 */
		if (RECURSIONOK(qctx->client)) {
			INSIST(!REDIRECT(qctx->client));
			result = query_recurse(qctx->client, qctx->qtype,
					       qctx->client->query.qname,
					       NULL, NULL, qctx->resuming);
			if (result == ISC_R_SUCCESS) {
				qctx->client->query.attributes |=
						NS_QUERYATTR_RECURSING;
				if (qctx->dns64)
					qctx->client->query.attributes |=
						NS_QUERYATTR_DNS64;
				if (qctx->dns64_exclude)
					qctx->client->query.attributes |=
						NS_QUERYATTR_DNS64EXCLUDE;
			} else
				RECURSE_ERROR(qctx, result);
			return (query_done(qctx));
		} else {
			/* Unable to give root server referral. */
			CCTRACE(ISC_LOG_ERROR,
				"unable to give root server referral");
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
			return (query_done(qctx));
		}
	}

	return (query_delegation(qctx));
}

/*%
 * Handle a delegation response from an authoritative lookup. This
 * may trigger additional lookups, e.g. from the cache database to
 * see if we have a better answer; if that is not allowed, return the
 * delegation to the client and call query_done().
 */
static isc_result_t
query_zone_delegation(query_ctx_t *qctx) {
	isc_result_t result;
	dns_rdataset_t **sigrdatasetp = NULL;
	isc_boolean_t detach = ISC_FALSE;

	/*
	 * If the query type is DS, look to see if we are
	 * authoritative for the child zone
	 */
	if (!RECURSIONOK(qctx->client) &&
	    (qctx->options & DNS_GETDB_NOEXACT) != 0 &&
	    qctx->qtype == dns_rdatatype_ds)
	{
		dns_db_t *tdb = NULL;
		dns_zone_t *tzone = NULL;
		dns_dbversion_t *tversion = NULL;
		result = query_getzonedb(qctx->client,
					 qctx->client->query.qname,
					 qctx->qtype,
					 DNS_GETDB_PARTIAL,
					 &tzone, &tdb,
					 &tversion);
		if (result != ISC_R_SUCCESS) {
			if (tdb != NULL)
				dns_db_detach(&tdb);
			if (tzone != NULL)
				dns_zone_detach(&tzone);
		} else {
			qctx->options &= ~DNS_GETDB_NOEXACT;
			query_putrdataset(qctx->client,
					  &qctx->rdataset);
			if (qctx->sigrdataset != NULL)
				query_putrdataset(qctx->client,
						  &qctx->sigrdataset);
			if (qctx->fname != NULL)
				query_releasename(qctx->client,
						  &qctx->fname);
			if (qctx->node != NULL)
				dns_db_detachnode(qctx->db,
						  &qctx->node);
			if (qctx->db != NULL)
				dns_db_detach(&qctx->db);
			if (qctx->zone != NULL)
				dns_zone_detach(&qctx->zone);
			qctx->version = NULL;
			RESTORE(qctx->version, tversion);
			RESTORE(qctx->db, tdb);
			RESTORE(qctx->zone, tzone);
			qctx->authoritative = ISC_TRUE;

			return (query_lookup(qctx));
		}
	}

	if (USECACHE(qctx->client) && RECURSIONOK(qctx->client)) {
		/*
		 * We might have a better answer or delegation in the
		 * cache.  We'll remember the current values of fname,
		 * rdataset, and sigrdataset.  We'll then go looking for
		 * QNAME in the cache.  If we find something better, we'll
		 * use it instead. If not, then query_lookup() calls
		 * query_notfound() which calls query_delegation(), and
		 * we'll restore these values there.
		 */
		query_keepname(qctx->client, qctx->fname, qctx->dbuf);
		dns_db_detachnode(qctx->db, &qctx->node);
		SAVE(qctx->zdb, qctx->db);
		SAVE(qctx->zfname, qctx->fname);
		SAVE(qctx->zversion, qctx->version);
		SAVE(qctx->zrdataset, qctx->rdataset);
		SAVE(qctx->zsigrdataset, qctx->sigrdataset);
		dns_db_attach(qctx->client->view->cachedb, &qctx->db);
		qctx->is_zone = ISC_FALSE;

		return (query_lookup(qctx));
	}

	/*
	 * qctx->fname could be released in
	 * query_addrrset(), so save a copy of it
	 * here in case we need it
	 */
	dns_fixedname_init(&qctx->dsname);
	dns_name_copy(qctx->fname, dns_fixedname_name(&qctx->dsname), NULL);

	/*
	 * If we don't have a cache, this is the best
	 * answer.
	 *
	 * If the client is making a nonrecursive
	 * query we always give out the authoritative
	 * delegation.  This way even if we get
	 * junk in our cache, we won't fail in our
	 * role as the delegating authority if another
	 * nameserver asks us about a delegated
	 * subzone.
	 *
	 * We enable the retrieval of glue for this
	 * database by setting client->query.gluedb.
	 */
	if (qctx->db != NULL && qctx->client->query.gluedb == NULL) {
		dns_db_attach(qctx->db, &qctx->client->query.gluedb);
		detach = ISC_TRUE;
	}
	qctx->client->query.isreferral = ISC_TRUE;
	/*
	 * We must ensure NOADDITIONAL is off,
	 * because the generation of
	 * additional data is required in
	 * delegations.
	 */
	qctx->client->query.attributes &=
		~NS_QUERYATTR_NOADDITIONAL;
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL)
		sigrdatasetp = &qctx->sigrdataset;
	query_addrrset(qctx->client, &qctx->fname,
		       &qctx->rdataset, sigrdatasetp,
		       qctx->dbuf, DNS_SECTION_AUTHORITY);
	if (detach) {
		dns_db_detach(&qctx->client->query.gluedb);
	}

	/*
	 * Add a DS if needed
	 */
	query_addds(qctx);

	return (query_done(qctx));
}

/*%
 * Handle delegation responses, including root referrals.
 *
 * If the delegation was returned from authoritative data,
 * call query_zone_delgation().  Otherwise, we can start
 * recursion if allowed; or else return the delegation to the
 * client and call query_done().
 */
static isc_result_t
query_delegation(query_ctx_t *qctx) {
	isc_result_t result;
	dns_rdataset_t **sigrdatasetp = NULL;
	isc_boolean_t detach = ISC_FALSE;

	qctx->authoritative = ISC_FALSE;

	if (qctx->is_zone) {
		return (query_zone_delegation(qctx));
	}

	if (qctx->zfname != NULL &&
	    (!dns_name_issubdomain(qctx->fname, qctx->zfname) ||
	     (qctx->is_staticstub_zone &&
	      dns_name_equal(qctx->fname, qctx->zfname))))
	{
		/*
		 * In the following cases use "authoritative"
		 * data instead of the cache delegation:
		 * 1. We've already got a delegation from
		 *    authoritative data, and it is better
		 *    than what we found in the cache.
		 *    (See the comment above.)
		 * 2. The query name matches the origin name
		 *    of a static-stub zone.  This needs to be
		 *    considered for the case where the NS of
		 *    the static-stub zone and the cached NS
		 *    are different.  We still need to contact
		 *    the nameservers configured in the
		 *    static-stub zone.
		 */
		query_releasename(qctx->client, &qctx->fname);

		/*
		 * We've already done query_keepname() on
		 * qctx->zfname, so we must set dbuf to NULL to
		 * prevent query_addrrset() from trying to
		 * call query_keepname() again.
		 */
		qctx->dbuf = NULL;
		query_putrdataset(qctx->client, &qctx->rdataset);
		if (qctx->sigrdataset != NULL)
			query_putrdataset(qctx->client,
					  &qctx->sigrdataset);
		qctx->version = NULL;

		RESTORE(qctx->fname, qctx->zfname);
		RESTORE(qctx->version, qctx->zversion);
		RESTORE(qctx->rdataset, qctx->zrdataset);
		RESTORE(qctx->sigrdataset, qctx->zsigrdataset);

		/*
		 * We don't clean up zdb here because we
		 * may still need it.  It will get cleaned
		 * up by the main cleanup code in query_done().
		 */
	}

	if (RECURSIONOK(qctx->client)) {
		/*
		 * We have a delegation and recursion is allowed,
		 * so we call query_recurse() to follow it.
		 * This phase of the query processing is done;
		 * we'll resume via fetch_callback() and
		 * query_resume() when the recursion is complete.
		 */
		dns_name_t *qname = qctx->client->query.qname;

		INSIST(!REDIRECT(qctx->client));

		if (dns_rdatatype_atparent(qctx->type)) {
			/*
			 * Parent is recursive for this rdata
			 * type (i.e., DS)
			 */
			result = query_recurse(qctx->client,
					       qctx->qtype, qname,
					       NULL, NULL,
					       qctx->resuming);
		} else if (qctx->dns64) {
			/*
			 * Look up an A record so we can
			 * synthesize DNS64
			 */
			result = query_recurse(qctx->client,
					       dns_rdatatype_a, qname,
					       NULL, NULL,
					       qctx->resuming);
		} else {
			/*
			 * Any other recursion
			 */
			result = query_recurse(qctx->client,
					       qctx->qtype, qname,
					       qctx->fname,
					       qctx->rdataset,
					       qctx->resuming);
		}
		if (result == ISC_R_SUCCESS) {
			qctx->client->query.attributes |=
				NS_QUERYATTR_RECURSING;
			if (qctx->dns64)
				qctx->client->query.attributes |=
					NS_QUERYATTR_DNS64;
			if (qctx->dns64_exclude)
				qctx->client->query.attributes |=
				      NS_QUERYATTR_DNS64EXCLUDE;
		} else if (result == DNS_R_DUPLICATE || result == DNS_R_DROP) {
			QUERY_ERROR(qctx, result);
		} else {
			RECURSE_ERROR(qctx, result);
		}

		return (query_done(qctx));
	}

	/*
	 * We have a delegation but recursion is not
	 * allowed, so return the delegation to the client.
	 *
	 * qctx->fname could be released in
	 * query_addrrset(), so save a copy of it
	 * here in case we need it
	 */
	dns_fixedname_init(&qctx->dsname);
	dns_name_copy(qctx->fname, dns_fixedname_name(&qctx->dsname), NULL);

	/*
	 * This is the best answer.
	 */
	qctx->client->query.attributes |= NS_QUERYATTR_CACHEGLUEOK;
	qctx->client->query.isreferral = ISC_TRUE;

	if (qctx->zdb != NULL && qctx->client->query.gluedb == NULL) {
		dns_db_attach(qctx->zdb, &qctx->client->query.gluedb);
		detach = ISC_TRUE;
	}

	/*
	 * We must ensure NOADDITIONAL is off,
	 * because the generation of
	 * additional data is required in
	 * delegations.
	 */
	qctx->client->query.attributes &= ~NS_QUERYATTR_NOADDITIONAL;
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL) {
		sigrdatasetp = &qctx->sigrdataset;
	}
	query_addrrset(qctx->client, &qctx->fname,
		       &qctx->rdataset, sigrdatasetp,
		       qctx->dbuf, DNS_SECTION_AUTHORITY);
	qctx->client->query.attributes &= ~NS_QUERYATTR_CACHEGLUEOK;
	if (detach) {
		dns_db_detach(&qctx->client->query.gluedb);
	}

	/*
	 * Add a DS if needed
	 */
	query_addds(qctx);

	return (query_done(qctx));
}

/*%
 * Add a DS record if needed.
 */
static void
query_addds(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	dns_fixedname_t fixed;
	dns_name_t *fname = NULL;
	dns_name_t *rname = NULL;
	dns_name_t *name;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	isc_buffer_t *dbuf, b;
	isc_result_t result;
	unsigned int count;

	CTRACE(ISC_LOG_DEBUG(3), "query_addds");

	/*
	 * DS not needed.
	 */
	if (!WANTDNSSEC(client)) {
		return;
	}

	/*
	 * We'll need some resources...
	 */
	rdataset = query_newrdataset(client);
	sigrdataset = query_newrdataset(client);
	if (rdataset == NULL || sigrdataset == NULL)
		goto cleanup;

	/*
	 * Look for the DS record, which may or may not be present.
	 */
	result = dns_db_findrdataset(qctx->db, qctx->node, qctx->version,
				     dns_rdatatype_ds, 0, client->now,
				     rdataset, sigrdataset);
	/*
	 * If we didn't find it, look for an NSEC.
	 */
	if (result == ISC_R_NOTFOUND)
		result = dns_db_findrdataset(qctx->db, qctx->node,
					     qctx->version,
					     dns_rdatatype_nsec,
					     0, client->now,
					     rdataset, sigrdataset);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND)
		goto addnsec3;
	if (!dns_rdataset_isassociated(rdataset) ||
	    !dns_rdataset_isassociated(sigrdataset))
		goto addnsec3;

	/*
	 * We've already added the NS record, so if the name's not there,
	 * we have other problems.  Use this name rather than calling
	 * query_addrrset().
	 */
	result = dns_message_firstname(client->message, DNS_SECTION_AUTHORITY);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	rname = NULL;
	dns_message_currentname(client->message, DNS_SECTION_AUTHORITY,
				&rname);
	result = dns_message_findtype(rname, dns_rdatatype_ns, 0, NULL);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	ISC_LIST_APPEND(rname->list, rdataset, link);
	ISC_LIST_APPEND(rname->list, sigrdataset, link);
	rdataset = NULL;
	sigrdataset = NULL;
	return;

 addnsec3:
	if (!dns_db_iszone(qctx->db))
		goto cleanup;
	/*
	 * Add the NSEC3 which proves the DS does not exist.
	 */
	dbuf = query_getnamebuf(client);
	if (dbuf == NULL)
		goto cleanup;
	fname = query_newname(client, dbuf, &b);
	dns_fixedname_init(&fixed);
	if (dns_rdataset_isassociated(rdataset))
		dns_rdataset_disassociate(rdataset);
	if (dns_rdataset_isassociated(sigrdataset))
		dns_rdataset_disassociate(sigrdataset);
	name = dns_fixedname_name(&qctx->dsname);
	query_findclosestnsec3(name, qctx->db, qctx->version, client, rdataset,
			       sigrdataset, fname, ISC_TRUE,
			       dns_fixedname_name(&fixed));
	if (!dns_rdataset_isassociated(rdataset))
		goto cleanup;
	query_addrrset(client, &fname, &rdataset, &sigrdataset, dbuf,
		       DNS_SECTION_AUTHORITY);
	/*
	 * Did we find the closest provable encloser instead?
	 * If so add the nearest to the closest provable encloser.
	 */
	if (!dns_name_equal(name, dns_fixedname_name(&fixed))) {
		count = dns_name_countlabels(dns_fixedname_name(&fixed)) + 1;
		dns_name_getlabelsequence(name,
					  dns_name_countlabels(name) - count,
					  count, dns_fixedname_name(&fixed));
		fixfname(client, &fname, &dbuf, &b);
		fixrdataset(client, &rdataset);
		fixrdataset(client, &sigrdataset);
		if (fname == NULL || rdataset == NULL || sigrdataset == NULL)
				goto cleanup;
		query_findclosestnsec3(dns_fixedname_name(&fixed),
				       qctx->db, qctx->version, client,
				       rdataset, sigrdataset, fname,
				       ISC_FALSE, NULL);
		if (!dns_rdataset_isassociated(rdataset))
			goto cleanup;
		query_addrrset(client, &fname, &rdataset, &sigrdataset, dbuf,
			       DNS_SECTION_AUTHORITY);
	}

 cleanup:
	if (rdataset != NULL)
		query_putrdataset(client, &rdataset);
	if (sigrdataset != NULL)
		query_putrdataset(client, &sigrdataset);
	if (fname != NULL)
		query_releasename(client, &fname);
}

/*%
 * Handle authoritative NOERROR/NODATA responses.
 */
static isc_result_t
query_nodata(query_ctx_t *qctx, isc_result_t result) {
#ifdef dns64_bis_return_excluded_addresses
	if (qctx->dns64)
#else
	if (qctx->dns64 && !qctx->dns64_exclude)
#endif
	{
		isc_buffer_t b;
		/*
		 * Restore the answers from the previous AAAA lookup.
		 */
		if (qctx->rdataset != NULL)
			query_putrdataset(qctx->client, &qctx->rdataset);
		if (qctx->sigrdataset != NULL)
			query_putrdataset(qctx->client, &qctx->sigrdataset);
		RESTORE(qctx->rdataset, qctx->client->query.dns64_aaaa);
		RESTORE(qctx->sigrdataset, qctx->client->query.dns64_sigaaaa);
		if (qctx->fname == NULL) {
			qctx->dbuf = query_getnamebuf(qctx->client);
			if (qctx->dbuf == NULL) {
				CCTRACE(ISC_LOG_ERROR,
				       "query_nodata: "
				       "query_getnamebuf failed (3)");
				QUERY_ERROR(qctx, DNS_R_SERVFAIL);
				return (query_done(qctx));;
			}
			qctx->fname = query_newname(qctx->client,
						    qctx->dbuf, &b);
			if (qctx->fname == NULL) {
				CCTRACE(ISC_LOG_ERROR,
				       "query_nodata: "
				       "query_newname failed (3)");
				QUERY_ERROR(qctx, DNS_R_SERVFAIL);
				return (query_done(qctx));;
			}
		}
		dns_name_copy(qctx->client->query.qname, qctx->fname, NULL);
		qctx->dns64 = ISC_FALSE;
#ifdef dns64_bis_return_excluded_addresses
		/*
		 * Resume the diverted processing of the AAAA response?
		 */
		if (qctx->dns64_exclude)
			return (query_prepresponse(qctx));
#endif
	} else if ((result == DNS_R_NXRRSET ||
		    result == DNS_R_NCACHENXRRSET) &&
		   !ISC_LIST_EMPTY(qctx->client->view->dns64) &&
		   qctx->client->message->rdclass == dns_rdataclass_in &&
		   qctx->qtype == dns_rdatatype_aaaa)
	{
		/*
		 * Look to see if there are A records for this name.
		 */
		switch (result) {
		case DNS_R_NCACHENXRRSET:
			/*
			 * This is from the negative cache; if the ttl is
			 * zero, we need to work out whether we have just
			 * decremented to zero or there was no negative
			 * cache ttl in the answer.
			 */
			if (qctx->rdataset->ttl != 0) {
				qctx->client->query.dns64_ttl =
					qctx->rdataset->ttl;
				break;
			}
			if (dns_rdataset_first(qctx->rdataset) == ISC_R_SUCCESS)
				qctx->client->query.dns64_ttl = 0;
			break;
		case DNS_R_NXRRSET:
			qctx->client->query.dns64_ttl =
				dns64_ttl(qctx->db, qctx->version);
			break;
		default:
			INSIST(0);
		}

		SAVE(qctx->client->query.dns64_aaaa, qctx->rdataset);
		SAVE(qctx->client->query.dns64_sigaaaa, qctx->sigrdataset);
		query_releasename(qctx->client, &qctx->fname);
		dns_db_detachnode(qctx->db, &qctx->node);
		qctx->type = qctx->qtype = dns_rdatatype_a;
		qctx->dns64 = ISC_TRUE;
		return (query_lookup(qctx));
	}

	if (qctx->is_zone) {
		return (query_sign_nodata(qctx));
	} else {
		/*
		 * We don't call query_addrrset() because we don't need any
		 * of its extra features (and things would probably break!).
		 */
		if (dns_rdataset_isassociated(qctx->rdataset)) {
			query_keepname(qctx->client, qctx->fname, qctx->dbuf);
			dns_message_addname(qctx->client->message,
					    qctx->fname,
					    DNS_SECTION_AUTHORITY);
			ISC_LIST_APPEND(qctx->fname->list,
					qctx->rdataset, link);
			qctx->fname = NULL;
			qctx->rdataset = NULL;
		}
	}

	return (query_done(qctx));
}

/*%
 * Add RRSIGs for NOERROR/NODATA responses when answering authoritatively.
 */
isc_result_t
query_sign_nodata(query_ctx_t *qctx) {
	isc_result_t result;
	/*
	 * Look for a NSEC3 record if we don't have a NSEC record.
	 */
	if (qctx->redirected)
		return (query_done(qctx));
	if (!dns_rdataset_isassociated(qctx->rdataset) &&
	     WANTDNSSEC(qctx->client))
	{
		if ((qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) == 0) {
			dns_name_t *found;
			dns_name_t *qname;
			dns_fixedname_t fixed;
			isc_buffer_t b;

			found = dns_fixedname_initname(&fixed);
			qname = qctx->client->query.qname;

			query_findclosestnsec3(qname, qctx->db, qctx->version,
					       qctx->client, qctx->rdataset,
					       qctx->sigrdataset, qctx->fname,
					       ISC_TRUE, found);
			/*
			 * Did we find the closest provable encloser
			 * instead? If so add the nearest to the
			 * closest provable encloser.
			 */
			if (dns_rdataset_isassociated(qctx->rdataset) &&
			    !dns_name_equal(qname, found) &&
			    (((qctx->client->sctx->options &
			       NS_SERVER_NONEAREST) == 0) ||
			     qctx->qtype == dns_rdatatype_ds))
			{
				unsigned int count;
				unsigned int skip;

				/*
				 * Add the closest provable encloser.
				 */
				query_addrrset(qctx->client, &qctx->fname,
					       &qctx->rdataset,
					       &qctx->sigrdataset,
					       qctx->dbuf,
					       DNS_SECTION_AUTHORITY);

				count = dns_name_countlabels(found)
						 + 1;
				skip = dns_name_countlabels(qname) -
						 count;
				dns_name_getlabelsequence(qname, skip,
							  count,
							  found);

				fixfname(qctx->client, &qctx->fname,
					 &qctx->dbuf, &b);
				fixrdataset(qctx->client, &qctx->rdataset);
				fixrdataset(qctx->client, &qctx->sigrdataset);
				if (qctx->fname == NULL ||
				    qctx->rdataset == NULL ||
				    qctx->sigrdataset == NULL) {
					CCTRACE(ISC_LOG_ERROR,
					       "query_sign_nodata: "
					       "failure getting "
					       "closest encloser");
					QUERY_ERROR(qctx, DNS_R_SERVFAIL);
					return (query_done(qctx));
				}
				/*
				 * 'nearest' doesn't exist so
				 * 'exist' is set to ISC_FALSE.
				 */
				query_findclosestnsec3(found, qctx->db,
						       qctx->version,
						       qctx->client,
						       qctx->rdataset,
						       qctx->sigrdataset,
						       qctx->fname,
						       ISC_FALSE,
						       NULL);
			}
		} else {
			query_releasename(qctx->client, &qctx->fname);
			query_addwildcardproof(qctx, ISC_FALSE, ISC_TRUE);
		}
	}
	if (dns_rdataset_isassociated(qctx->rdataset)) {
		/*
		 * If we've got a NSEC record, we need to save the
		 * name now because we're going call query_addsoa()
		 * below, and it needs to use the name buffer.
		 */
		query_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else if (qctx->fname != NULL) {
		/*
		 * We're not going to use fname, and need to release
		 * our hold on the name buffer so query_addsoa()
		 * may use it.
		 */
		query_releasename(qctx->client, &qctx->fname);
	}

	/*
	 * The RPZ SOA has already been added to the additional section
	 * if this was an RPZ rewrite, but if it wasn't, add it now.
	 */
	if (!qctx->nxrewrite) {
		result = query_addsoa(qctx, ISC_UINT32_MAX,
				      DNS_SECTION_AUTHORITY);
		if (result != ISC_R_SUCCESS) {
			QUERY_ERROR(qctx, result);
			return (query_done(qctx));
		}
	}

	/*
	 * Add NSEC record if we found one.
	 */
	if (WANTDNSSEC(qctx->client) &&
	    dns_rdataset_isassociated(qctx->rdataset))
	{
		query_addnxrrsetnsec(qctx);
	}

	return (query_done(qctx));
}

static void
query_addnxrrsetnsec(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	dns_rdata_t sigrdata;
	dns_rdata_rrsig_t sig;
	unsigned int labels;
	isc_buffer_t *dbuf, b;
	dns_name_t *fname;

	INSIST(qctx->fname != NULL);

	if ((qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) == 0) {
		query_addrrset(client, &qctx->fname,
			       &qctx->rdataset, &qctx->sigrdataset,
			       NULL, DNS_SECTION_AUTHORITY);
		return;
	}

	if (qctx->sigrdataset == NULL ||
	    !dns_rdataset_isassociated(qctx->sigrdataset))
	{
		return;
	}

	if (dns_rdataset_first(qctx->sigrdataset) != ISC_R_SUCCESS) {
		return;
	}

	dns_rdata_init(&sigrdata);
	dns_rdataset_current(qctx->sigrdataset, &sigrdata);
	if (dns_rdata_tostruct(&sigrdata, &sig, NULL) != ISC_R_SUCCESS) {
		return;
	}

	labels = dns_name_countlabels(qctx->fname);
	if ((unsigned int)sig.labels + 1 >= labels) {
		return;
	}

	query_addwildcardproof(qctx, ISC_TRUE, ISC_FALSE);

	/*
	 * We'll need some resources...
	 */
	dbuf = query_getnamebuf(client);
	if (dbuf == NULL) {
		return;
	}

	fname = query_newname(client, dbuf, &b);
	if (fname == NULL) {
		return;
	}

	dns_name_split(qctx->fname, sig.labels + 1, NULL, fname);
	/* This will succeed, since we've stripped labels. */
	RUNTIME_CHECK(dns_name_concatenate(dns_wildcardname, fname, fname,
					   NULL) == ISC_R_SUCCESS);
	query_addrrset(client, &fname, &qctx->rdataset, &qctx->sigrdataset,
		       dbuf, DNS_SECTION_AUTHORITY);
}

/*%
 * Handle NXDOMAIN and empty wildcard responses.
 */
static isc_result_t
query_nxdomain(query_ctx_t *qctx, isc_boolean_t empty_wild) {
	dns_section_t section;
	isc_uint32_t ttl;
	isc_result_t result;

	INSIST(qctx->is_zone || REDIRECT(qctx->client));

	if (!empty_wild) {
		result = query_redirect(qctx);
		if (result != ISC_R_COMPLETE)
			return (result);
	}

	if (dns_rdataset_isassociated(qctx->rdataset)) {
		/*
		 * If we've got a NSEC record, we need to save the
		 * name now because we're going call query_addsoa()
		 * below, and it needs to use the name buffer.
		 */
		query_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else if (qctx->fname != NULL) {
		/*
		 * We're not going to use fname, and need to release
		 * our hold on the name buffer so query_addsoa()
		 * may use it.
		 */
		query_releasename(qctx->client, &qctx->fname);
	}

	/*
	 * Add SOA to the additional section if generated by a
	 * RPZ rewrite.
	 *
	 * If the query was for a SOA record force the
	 * ttl to zero so that it is possible for clients to find
	 * the containing zone of an arbitrary name with a stub
	 * resolver and not have it cached.
	 */
	section = qctx->nxrewrite ? DNS_SECTION_ADDITIONAL
				  : DNS_SECTION_AUTHORITY;
	ttl = ISC_UINT32_MAX;
	if (!qctx->nxrewrite && qctx->qtype == dns_rdatatype_soa &&
	    qctx->zone != NULL && dns_zone_getzeronosoattl(qctx->zone))
	{
		ttl = 0;
	}
	result = query_addsoa(qctx, ttl, section);
	if (result != ISC_R_SUCCESS) {
		QUERY_ERROR(qctx, result);
		return (query_done(qctx));
	}

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NSEC record if we found one.
		 */
		if (dns_rdataset_isassociated(qctx->rdataset))
			query_addrrset(qctx->client, &qctx->fname,
				       &qctx->rdataset, &qctx->sigrdataset,
				       NULL, DNS_SECTION_AUTHORITY);
		query_addwildcardproof(qctx, ISC_FALSE, ISC_FALSE);
	}

	/*
	 * Set message rcode.
	 */
	if (empty_wild)
		qctx->client->message->rcode = dns_rcode_noerror;
	else
		qctx->client->message->rcode = dns_rcode_nxdomain;

	return (query_done(qctx));
}

/*
 * Handle both types of NXDOMAIN redirection, calling redirect()
 * (which implements type redirect zones) and redirect2() (which
 * implements recursive nxdomain-redirect lookups).
 *
 * Any result code other than ISC_R_COMPLETE means redirection was
 * successful and the result code should be returned up the call stack.
 *
 * ISC_R_COMPLETE means we reached the end of this function without
 * redirecting, so query processing should continue past it.
 */
static isc_result_t
query_redirect(query_ctx_t *qctx)  {
	isc_result_t result;

	result = redirect(qctx->client, qctx->fname, qctx->rdataset,
			  &qctx->node, &qctx->db, &qctx->version,
			  qctx->type);
	switch (result) {
	case ISC_R_SUCCESS:
		inc_stats(qctx->client,
			  ns_statscounter_nxdomainredirect);
		return (query_prepresponse(qctx));
	case DNS_R_NXRRSET:
		qctx->redirected = ISC_TRUE;
		qctx->is_zone = ISC_TRUE;
		return (query_nodata(qctx, DNS_R_NXRRSET));
	case DNS_R_NCACHENXRRSET:
		qctx->redirected = ISC_TRUE;
		qctx->is_zone = ISC_FALSE;
		return (query_ncache(qctx, DNS_R_NCACHENXRRSET));
	default:
		break;
	}

	result = redirect2(qctx->client, qctx->fname, qctx->rdataset,
			   &qctx->node, &qctx->db, &qctx->version,
			   qctx->type, &qctx->is_zone);
	switch (result) {
	case ISC_R_SUCCESS:
		inc_stats(qctx->client,
			  ns_statscounter_nxdomainredirect);
		return (query_prepresponse(qctx));
	case DNS_R_CONTINUE:
		inc_stats(qctx->client,
			  ns_statscounter_nxdomainredirect_rlookup);
		SAVE(qctx->client->query.redirect.db, qctx->db);
		SAVE(qctx->client->query.redirect.node, qctx->node);
		SAVE(qctx->client->query.redirect.zone, qctx->zone);
		qctx->client->query.redirect.qtype = qctx->qtype;
		INSIST(qctx->rdataset != NULL);
		SAVE(qctx->client->query.redirect.rdataset, qctx->rdataset);
		SAVE(qctx->client->query.redirect.sigrdataset,
		     qctx->sigrdataset);
		qctx->client->query.redirect.result = DNS_R_NCACHENXDOMAIN;
		dns_name_copy(qctx->fname, qctx->client->query.redirect.fname,
			      NULL);
		qctx->client->query.redirect.authoritative =
			qctx->authoritative;
		qctx->client->query.redirect.is_zone = qctx->is_zone;
		return (query_done(qctx));
	case DNS_R_NXRRSET:
		qctx->redirected = ISC_TRUE;
		qctx->is_zone = ISC_TRUE;
		return (query_nodata(qctx, DNS_R_NXRRSET));
	case DNS_R_NCACHENXRRSET:
		qctx->redirected = ISC_TRUE;
		qctx->is_zone = ISC_FALSE;
		return (query_ncache(qctx, DNS_R_NCACHENXRRSET));
	default:
		break;
	}

	return (ISC_R_COMPLETE);
}

/*%
 * Logging function to be passed to dns_nsec_noexistnodata.
 */
static void
log_noexistnodata(void *val, int level, const char *fmt, ...) {
	query_ctx_t *qctx = val;
	va_list ap;

	va_start(ap, fmt);
	ns_client_logv(qctx->client, NS_LOGCATEGORY_QUERIES,
		       NS_LOGMODULE_QUERY, level, fmt, ap);
	va_end(ap);
}

static dns_ttl_t
query_synthttl(dns_rdataset_t *soardataset, dns_rdataset_t *sigsoardataset,
	       dns_rdataset_t *p1rdataset, dns_rdataset_t *sigp1rdataset,
	       dns_rdataset_t *p2rdataset, dns_rdataset_t *sigp2rdataset)
{
	dns_rdata_soa_t soa;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_ttl_t ttl;
	isc_result_t result;

	REQUIRE(soardataset != NULL);
	REQUIRE(sigsoardataset != NULL);
	REQUIRE(p1rdataset != NULL);
	REQUIRE(sigp1rdataset != NULL);

	result = dns_rdataset_first(soardataset);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	dns_rdataset_current(soardataset, &rdata);
	dns_rdata_tostruct(&rdata, &soa, NULL);

	ttl = ISC_MIN(soa.minimum, soardataset->ttl);
	ttl = ISC_MIN(ttl, sigsoardataset->ttl);
	ttl = ISC_MIN(ttl, p1rdataset->ttl);
	ttl = ISC_MIN(ttl, sigp1rdataset->ttl);
	if (p2rdataset != NULL)
		ttl = ISC_MIN(ttl, p2rdataset->ttl);
	if (sigp2rdataset != NULL)
		ttl = ISC_MIN(ttl, sigp2rdataset->ttl);

	return (ttl);
}

/*
 * Synthesize a NODATA response from the SOA and covering NSEC in cache.
 */
static isc_result_t
query_synthnodata(query_ctx_t *qctx, const dns_name_t *signer,
		  dns_rdataset_t **soardatasetp,
		  dns_rdataset_t **sigsoardatasetp)
{
	dns_name_t *name = NULL;
	dns_ttl_t ttl;
	isc_buffer_t *dbuf, b;
	isc_result_t result;

	/*
	 * Detemine the correct TTL to use for the SOA and RRSIG
	 */
	ttl = query_synthttl(*soardatasetp, *sigsoardatasetp,
			     qctx->rdataset, qctx->sigrdataset,
			     NULL, NULL);
	(*soardatasetp)->ttl = (*sigsoardatasetp)->ttl = ttl;

	/*
	 * We want the SOA record to be first, so save the
	 * NODATA proof's name now or else discard it.
	 */
	if (WANTDNSSEC(qctx->client)) {
		query_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else {
		query_releasename(qctx->client, &qctx->fname);
	}

	dbuf = query_getnamebuf(qctx->client);
	if (dbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	name = query_newname(qctx->client, dbuf, &b);
	if (name == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	dns_name_copy(signer, name, NULL);

	/*
	 * Add SOA record. Omit the RRSIG if DNSSEC was not requested.
	 */
	if (!WANTDNSSEC(qctx->client)) {
		sigsoardatasetp = NULL;
	}
	query_addrrset(qctx->client, &name, soardatasetp, sigsoardatasetp,
		       dbuf, DNS_SECTION_AUTHORITY);

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NODATA proof.
		 */
		query_addrrset(qctx->client, &qctx->fname,
			       &qctx->rdataset, &qctx->sigrdataset,
			       NULL, DNS_SECTION_AUTHORITY);
	}

	result = ISC_R_SUCCESS;
	inc_stats(qctx->client, ns_statscounter_nodatasynth);

cleanup:
	if (name != NULL) {
		query_releasename(qctx->client, &name);
	}
	return (result);
}

/*
 * Synthesize a wildcard answer using the contents of 'rdataset'.
 * qctx contains the NODATA proof.
 */
static isc_result_t
query_synthwildcard(query_ctx_t *qctx, dns_rdataset_t *rdataset,
		    dns_rdataset_t *sigrdataset)
{
	dns_name_t *name = NULL;
	isc_buffer_t *dbuf, b;
	isc_result_t result;
	dns_rdataset_t *clone = NULL, *sigclone = NULL;
	dns_rdataset_t **sigrdatasetp;

	/*
	 * We want the answer to be first, so save the
	 * NOQNAME proof's name now or else discard it.
	 */
	if (WANTDNSSEC(qctx->client)) {
		query_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else {
		query_releasename(qctx->client, &qctx->fname);
	}

	dbuf = query_getnamebuf(qctx->client);
	if (dbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	name = query_newname(qctx->client, dbuf, &b);
	if (name == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	dns_name_copy(qctx->client->query.qname, name, NULL);

	clone = query_newrdataset(qctx->client);
	if (clone == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	dns_rdataset_clone(rdataset, clone);

	/*
	 * Add answer RRset. Omit the RRSIG if DNSSEC was not requested.
	 */
	if (WANTDNSSEC(qctx->client)) {
		sigclone = query_newrdataset(qctx->client);
		if (sigclone == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		dns_rdataset_clone(sigrdataset, sigclone);
		sigrdatasetp = &sigclone;
	} else {
		sigrdatasetp = NULL;
	}

	query_addrrset(qctx->client, &name, &clone, sigrdatasetp,
		       dbuf, DNS_SECTION_ANSWER);

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NOQNAME proof.
		 */
		query_addrrset(qctx->client, &qctx->fname,
			       &qctx->rdataset, &qctx->sigrdataset,
			       NULL, DNS_SECTION_AUTHORITY);
	}

	result = ISC_R_SUCCESS;
	inc_stats(qctx->client, ns_statscounter_wildcardsynth);

cleanup:
	if (name != NULL) {
		query_releasename(qctx->client, &name);
	}
	if (clone != NULL) {
		query_putrdataset(qctx->client, &clone);
	}
	if (sigclone != NULL) {
		query_putrdataset(qctx->client, &sigclone);
	}
	return (result);
}

/*
 * Add a synthesized CNAME record from the wildard RRset (rdataset)
 * and NODATA proof by calling query_synthwildcard then setup to
 * follow the CNAME.
 */
static isc_result_t
query_synthcnamewildcard(query_ctx_t *qctx, dns_rdataset_t *rdataset,
			 dns_rdataset_t *sigrdataset)
{
	isc_result_t result;
	dns_name_t *tname = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_cname_t cname;

	result = query_synthwildcard(qctx, rdataset, sigrdataset);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	qctx->client->query.attributes |= NS_QUERYATTR_PARTIALANSWER;

	/*
	 * Reset qname to be the target name of the CNAME and restart
	 * the query.
	 */
	result = dns_message_gettempname(qctx->client->message, &tname);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_rdataset_first(rdataset);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (result);
	}

	dns_rdataset_current(rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &cname, NULL);
	dns_rdata_reset(&rdata);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (result);
	}

	dns_name_init(tname, NULL);
	result = dns_name_dup(&cname.cname, qctx->client->mctx, tname);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		dns_rdata_freestruct(&cname);
		return (result);
	}

	dns_rdata_freestruct(&cname);
	ns_client_qnamereplace(qctx->client, tname);
	qctx->want_restart = ISC_TRUE;
	if (!WANTRECURSION(qctx->client)) {
		qctx->options |= DNS_GETDB_NOLOG;
	}

	return (result);
}

/*
 * Synthesize a NXDOMAIN response from qctx (which contains the
 * NODATA proof), nowild + nowildrdataset + signowildrdataset (which
 * contains the NOWILDCARD proof) and signer + soardatasetp + sigsoardatasetp
 * which contain the SOA record + RRSIG for the negative answer.
 */
static isc_result_t
query_synthnxdomain(query_ctx_t *qctx,
		    dns_name_t *nowild,
		    dns_rdataset_t *nowildrdataset,
		    dns_rdataset_t *signowildrdataset,
		    dns_name_t *signer,
		    dns_rdataset_t **soardatasetp,
		    dns_rdataset_t **sigsoardatasetp)
{
	dns_name_t *name = NULL;
	dns_ttl_t ttl;
	isc_buffer_t *dbuf, b;
	isc_result_t result;
	dns_rdataset_t *clone = NULL, *sigclone = NULL;

	/*
	 * Detemine the correct TTL to use for the SOA and RRSIG
	 */
	ttl = query_synthttl(*soardatasetp, *sigsoardatasetp,
			     qctx->rdataset, qctx->sigrdataset,
			     nowildrdataset, signowildrdataset);
	(*soardatasetp)->ttl = (*sigsoardatasetp)->ttl = ttl;

	/*
	 * We want the SOA record to be first, so save the
	 * NOQNAME proof's name now or else discard it.
	 */
	if (WANTDNSSEC(qctx->client)) {
		query_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else {
		query_releasename(qctx->client, &qctx->fname);
	}

	dbuf = query_getnamebuf(qctx->client);
	if (dbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	name = query_newname(qctx->client, dbuf, &b);
	if (name == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	dns_name_copy(signer, name, NULL);

	/*
	 * Add SOA record. Omit the RRSIG if DNSSEC was not requested.
	 */
	if (!WANTDNSSEC(qctx->client)) {
		sigsoardatasetp = NULL;
	}
	query_addrrset(qctx->client, &name, soardatasetp, sigsoardatasetp,
		       dbuf, DNS_SECTION_AUTHORITY);

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NOQNAME proof.
		 */
		query_addrrset(qctx->client, &qctx->fname,
			       &qctx->rdataset, &qctx->sigrdataset,
			       NULL, DNS_SECTION_AUTHORITY);

		dbuf = query_getnamebuf(qctx->client);
		if (dbuf == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		name = query_newname(qctx->client, dbuf, &b);
		if (name == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		dns_name_copy(nowild, name, NULL);

		clone = query_newrdataset(qctx->client);
		sigclone = query_newrdataset(qctx->client);
		if (clone == NULL || sigclone == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		dns_rdataset_clone(nowildrdataset, clone);
		dns_rdataset_clone(signowildrdataset, sigclone);

		/*
		 * Add NOWILDCARD proof.
		 */
		query_addrrset(qctx->client, &name, &clone, &sigclone,
			       dbuf, DNS_SECTION_AUTHORITY);
	}

	qctx->client->message->rcode = dns_rcode_nxdomain;
	result = ISC_R_SUCCESS;
	inc_stats(qctx->client, ns_statscounter_nxdomainsynth);

cleanup:
	if (name != NULL) {
		query_releasename(qctx->client, &name);
	}
	if (clone != NULL) {
		query_putrdataset(qctx->client, &clone);
	}
	if (sigclone != NULL) {
		query_putrdataset(qctx->client, &sigclone);
	}
	return (result);
}

/*
 * Check that all signer names in sigrdataset match the expected signer.
 */
static isc_result_t
checksignames(dns_name_t *signer, dns_rdataset_t *sigrdataset) {
	isc_result_t result;

	for (result = dns_rdataset_first(sigrdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(sigrdataset)) {
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_rrsig_t rrsig;

		dns_rdataset_current(sigrdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &rrsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (dns_name_countlabels(signer) == 0) {
			dns_name_copy(&rrsig.signer, signer, NULL);
		} else if (!dns_name_equal(signer, &rrsig.signer)) {
			return (ISC_R_FAILURE);
		}
	}

	return (ISC_R_SUCCESS);
}

/*%
 * Handle covering NSEC responses.
 *
 * Verify the NSEC record is apropriate for the QNAME; if not,
 * redo the initial query without DNS_DBFIND_COVERINGNSEC.
 *
 * If the covering NSEC proves that the name exists but not the type,
 * synthesize a NODATA response.
 *
 * If the name doesn't exist, compute the wildcard record and check whether
 * the wildcard name exists or not.  If we can't determine this, redo the
 * initial query without DNS_DBFIND_COVERINGNSEC.
 *
 * If the wildcard name does not exist, compute the SOA name and look that
 * up.  If the SOA record does not exist, redo the initial query without
 * DNS_DBFIND_COVERINGNSEC.  If the SOA record exists, synthesize an
 * NXDOMAIN response from the found records.
 *
 * If the wildcard name does exist, perform a lookup for the requested
 * type at the wildcard name.
 */
static isc_result_t
query_coveringnsec(query_ctx_t *qctx) {
	dns_db_t *db = NULL;
	dns_clientinfo_t ci;
	dns_clientinfomethods_t cm;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t fixed;
	dns_fixedname_t fnowild;
	dns_fixedname_t fsigner;
	dns_fixedname_t fwild;
	dns_name_t *fname = NULL;
	dns_name_t *nowild = NULL;
	dns_name_t *signer = NULL;
	dns_name_t *wild = NULL;
	dns_rdataset_t *soardataset = NULL, *sigsoardataset = NULL;
	dns_rdataset_t rdataset, sigrdataset;
	isc_boolean_t done = ISC_FALSE;
	isc_boolean_t exists = ISC_TRUE, data = ISC_TRUE;
	isc_boolean_t redirected = ISC_FALSE;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int dboptions = qctx->client->query.dboptions;

	dns_rdataset_init(&rdataset);
	dns_rdataset_init(&sigrdataset);

	/*
	 * If we have no signer name, stop immediately.
	 */
	if (!dns_rdataset_isassociated(qctx->sigrdataset)) {
		goto cleanup;
	}

	wild = dns_fixedname_initname(&fwild);
	fname = dns_fixedname_initname(&fixed);
	signer = dns_fixedname_initname(&fsigner);
	nowild = dns_fixedname_initname(&fnowild);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, qctx->client, NULL);

	/*
	 * All signer names must be the same to accept.
	 */
	result = checksignames(signer, qctx->sigrdataset);
	if (result != ISC_R_SUCCESS) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	/*
	 * Check that we have the correct NOQNAME NSEC record.
	 */
	result = dns_nsec_noexistnodata(qctx->qtype, qctx->client->query.qname,
					qctx->fname, qctx->rdataset,
					&exists, &data, wild,
					log_noexistnodata, qctx);

	if (result != ISC_R_SUCCESS || (exists && data)) {
		goto cleanup;
	}

	if (exists) {
		if (qctx->type == dns_rdatatype_any) {	/* XXX not yet */
			goto cleanup;
		}
		if (qctx->client->filter_aaaa != dns_aaaa_ok &&
		    (qctx->type == dns_rdatatype_a ||
		     qctx->type == dns_rdatatype_aaaa)) /* XXX not yet */
		{
			goto cleanup;
		}
		if (!ISC_LIST_EMPTY(qctx->client->view->dns64) &&
		    (qctx->type == dns_rdatatype_a ||
		     qctx->type == dns_rdatatype_aaaa)) /* XXX not yet */
		{
			goto cleanup;
		}
		if (!qctx->resuming && !STALE(qctx->rdataset) &&
		    qctx->rdataset->ttl == 0 && RECURSIONOK(qctx->client))
		{
			goto cleanup;
		}

		soardataset = query_newrdataset(qctx->client);
		sigsoardataset = query_newrdataset(qctx->client);
		if (soardataset == NULL || sigsoardataset == NULL) {
			goto cleanup;
		}

		/*
		 * Look for SOA record to construct NODATA response.
		 */
		dns_db_attach(qctx->db, &db);
		result = dns_db_findext(db, signer, qctx->version,
					dns_rdatatype_soa, dboptions,
					qctx->client->now, &node,
					fname, &cm, &ci, soardataset,
					sigsoardataset);

		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		(void)query_synthnodata(qctx, signer,
					&soardataset, &sigsoardataset);
		done = ISC_TRUE;
		goto cleanup;
	}

	/*
	 * Look up the no-wildcard proof.
	 */
	dns_db_attach(qctx->db, &db);
	result = dns_db_findext(db, wild, qctx->version, qctx->type,
				dboptions | DNS_DBFIND_COVERINGNSEC,
				qctx->client->now, &node, nowild,
				&cm, &ci, &rdataset, &sigrdataset);

	if (rdataset.trust != dns_trust_secure ||
	    sigrdataset.trust != dns_trust_secure)
	{
		goto cleanup;
	}

	/*
	 * Zero TTL handling of wildcard record.
	 *
	 * We don't yet have code to handle synthesis and type ANY,
	 * AAAA filtering or dns64 processing so we abort the
	 * synthesis here if there would be a interaction.
	 */
	switch (result) {
	case ISC_R_SUCCESS:
		if (qctx->type == dns_rdatatype_any) {	/* XXX not yet */
			goto cleanup;
		}
		if (qctx->client->filter_aaaa != dns_aaaa_ok &&
		    (qctx->type == dns_rdatatype_a ||
		     qctx->type == dns_rdatatype_aaaa)) /* XXX not yet */
		{
			goto cleanup;
		}
		if (!ISC_LIST_EMPTY(qctx->client->view->dns64) &&
		    (qctx->type == dns_rdatatype_a ||
		     qctx->type == dns_rdatatype_aaaa)) /* XXX not yet */
		{
			goto cleanup;
		}
		/* FALLTHROUGH */
	case DNS_R_CNAME:
		if (!qctx->resuming && !STALE(&rdataset) &&
		    rdataset.ttl == 0 && RECURSIONOK(qctx->client))
		{
			goto cleanup;
		}
	default:
		break;
	}

	switch (result) {
	case DNS_R_COVERINGNSEC:
		result = dns_nsec_noexistnodata(qctx->qtype, wild,
						nowild, &rdataset,
						&exists, &data, NULL,
						log_noexistnodata, qctx);
		if (result != ISC_R_SUCCESS || exists) {
			goto cleanup;
		}
		break;
	case ISC_R_SUCCESS:		/* wild card match */
		(void)query_synthwildcard(qctx, &rdataset, &sigrdataset);
		done = ISC_TRUE;
		goto cleanup;
	case DNS_R_CNAME:		/* wild card cname */
		(void)query_synthcnamewildcard(qctx, &rdataset, &sigrdataset);
		done = ISC_TRUE;
		goto cleanup;
	case DNS_R_NCACHENXRRSET:	/* wild card nodata */
	case DNS_R_NCACHENXDOMAIN:	/* direct nxdomain */
	default:
		goto cleanup;
	}

	/*
	 * We now have the proof that we have an NXDOMAIN.  Apply
	 * NXDOMAIN redirection if configured.
	 */
	result = query_redirect(qctx);
	if (result != ISC_R_COMPLETE) {
		redirected = ISC_TRUE;
		goto cleanup;
	}

	/*
	 * Must be signed to accept.
	 */
	if (!dns_rdataset_isassociated(&sigrdataset)) {
		goto cleanup;
	}

	/*
	 * Check signer signer names again.
	 */
	result = checksignames(signer, &sigrdataset);
	if (result != ISC_R_SUCCESS) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}

	soardataset = query_newrdataset(qctx->client);
	sigsoardataset = query_newrdataset(qctx->client);
	if (soardataset == NULL || sigsoardataset == NULL) {
		goto cleanup;
	}

	/*
	 * Look for SOA record to construct NXDOMAIN response.
	 */
	result = dns_db_findext(db, signer, qctx->version,
				dns_rdatatype_soa, dboptions,
				qctx->client->now, &node,
				fname, &cm, &ci, soardataset,
				sigsoardataset);

	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	(void)query_synthnxdomain(qctx, nowild, &rdataset, &sigrdataset,
				  signer, &soardataset, &sigsoardataset);
	done = ISC_TRUE;

 cleanup:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}
	if (dns_rdataset_isassociated(&sigrdataset)) {
		dns_rdataset_disassociate(&sigrdataset);
	}
	if (soardataset != NULL) {
		query_putrdataset(qctx->client, &soardataset);
	}
	if (sigsoardataset != NULL) {
		query_putrdataset(qctx->client, &sigsoardataset);
	}
	if (db != NULL) {
		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}
		dns_db_detach(&db);
	}

	if (redirected) {
		return (result);
	}

	if (!done) {
		/*
		 * No covering NSEC was found; proceed with recursion.
		 */
		qctx->findcoveringnsec = ISC_FALSE;
		if (qctx->fname != NULL) {
			query_releasename(qctx->client, &qctx->fname);
		}
		if (qctx->node != NULL) {
			dns_db_detachnode(qctx->db, &qctx->node);
		}
		query_putrdataset(qctx->client, &qctx->rdataset);
		if (qctx->sigrdataset != NULL) {
			query_putrdataset(qctx->client, &qctx->sigrdataset);
		}
		return (query_lookup(qctx));
	}

	return (query_done(qctx));
}

/*%
 * Handle negative cache responses, DNS_R_NCACHENXRRSET or
 * DNS_R_NCACHENXDOMAIN. (Note: may also be called with result
 * set to DNS_R_NXDOMAIN when handling DNS64 lookups.)
 */
static isc_result_t
query_ncache(query_ctx_t *qctx, isc_result_t result) {
	INSIST(!qctx->is_zone);
	INSIST(result == DNS_R_NCACHENXDOMAIN ||
	       result == DNS_R_NCACHENXRRSET ||
	       result == DNS_R_NXDOMAIN);

	qctx->authoritative = ISC_FALSE;

	if (result == DNS_R_NCACHENXDOMAIN) {
		/*
		 * Set message rcode. (This is not done when
		 * result == DNS_R_NXDOMAIN because that means we're
		 * being called after a DNS64 lookup and don't want
		 * to update the rcode now.)
		 */
		qctx->client->message->rcode = dns_rcode_nxdomain;

		/* Look for RFC 1918 leakage from Internet. */
		if (qctx->qtype == dns_rdatatype_ptr &&
		    qctx->client->message->rdclass == dns_rdataclass_in &&
		    dns_name_countlabels(qctx->fname) == 7)
		{
			warn_rfc1918(qctx->client, qctx->fname, qctx->rdataset);
		}
	}

	return (query_nodata(qctx, result));
}

/*
 * Handle CNAME responses.
 */
static isc_result_t
query_cname(query_ctx_t *qctx) {
	isc_result_t result;
	dns_name_t *tname;
	dns_rdataset_t *trdataset;
	dns_rdataset_t **sigrdatasetp = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_cname_t cname;

	/*
	 * If we have a zero ttl from the cache refetch it.
	 */
	if (!qctx->is_zone && !qctx->resuming && !STALE(qctx->rdataset) &&
	    qctx->rdataset->ttl == 0 && RECURSIONOK(qctx->client))
	{
		qctx_clean(qctx);

		INSIST(!REDIRECT(qctx->client));

		result = query_recurse(qctx->client, qctx->qtype,
				       qctx->client->query.qname,
				       NULL, NULL, qctx->resuming);
		if (result == ISC_R_SUCCESS) {
			qctx->client->query.attributes |=
				NS_QUERYATTR_RECURSING;
			if (qctx->dns64)
				qctx->client->query.attributes |=
					NS_QUERYATTR_DNS64;
			if (qctx->dns64_exclude)
				qctx->client->query.attributes |=
					NS_QUERYATTR_DNS64EXCLUDE;
		} else {
			RECURSE_ERROR(qctx, result);
		}

		return (query_done(qctx));
	}

	/*
	 * Keep a copy of the rdataset.  We have to do this because
	 * query_addrrset may clear 'rdataset' (to prevent the
	 * cleanup code from cleaning it up).
	 */
	trdataset = qctx->rdataset;

	/*
	 * Add the CNAME to the answer section.
	 */
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL)
		sigrdatasetp = &qctx->sigrdataset;

	if (WANTDNSSEC(qctx->client) &&
	    (qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) != 0)
	{
		dns_fixedname_init(&qctx->wildcardname);
		dns_name_copy(qctx->fname,
			      dns_fixedname_name(&qctx->wildcardname), NULL);
		qctx->need_wildcardproof = ISC_TRUE;
	}

	if (NOQNAME(qctx->rdataset) && WANTDNSSEC(qctx->client)) {
		qctx->noqname = qctx->rdataset;
	} else {
		qctx->noqname = NULL;
	}

	if (!qctx->is_zone && RECURSIONOK(qctx->client))
		query_prefetch(qctx->client, qctx->fname, qctx->rdataset);

	query_addrrset(qctx->client, &qctx->fname,
		       &qctx->rdataset, sigrdatasetp, qctx->dbuf,
		       DNS_SECTION_ANSWER);

	query_addnoqnameproof(qctx);

	/*
	 * We set the PARTIALANSWER attribute so that if anything goes
	 * wrong later on, we'll return what we've got so far.
	 */
	qctx->client->query.attributes |= NS_QUERYATTR_PARTIALANSWER;

	/*
	 * Reset qname to be the target name of the CNAME and restart
	 * the query.
	 */
	tname = NULL;
	result = dns_message_gettempname(qctx->client->message, &tname);
	if (result != ISC_R_SUCCESS)
		return (query_done(qctx));

	result = dns_rdataset_first(trdataset);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (query_done(qctx));
	}

	dns_rdataset_current(trdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &cname, NULL);
	dns_rdata_reset(&rdata);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (query_done(qctx));
	}

	dns_name_init(tname, NULL);
	result = dns_name_dup(&cname.cname, qctx->client->mctx, tname);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		dns_rdata_freestruct(&cname);
		return (query_done(qctx));
	}

	dns_rdata_freestruct(&cname);
	ns_client_qnamereplace(qctx->client, tname);
	qctx->want_restart = ISC_TRUE;
	if (!WANTRECURSION(qctx->client))
		qctx->options |= DNS_GETDB_NOLOG;

	query_addauth(qctx);

	return (query_done(qctx));
}

/*
 * Handle DNAME responses.
 */
static isc_result_t
query_dname(query_ctx_t *qctx) {
	dns_name_t *tname, *prefix;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_dname_t dname;
	dns_fixedname_t fixed;
	dns_rdataset_t *trdataset;
	dns_rdataset_t **sigrdatasetp = NULL;
	dns_namereln_t namereln;
	isc_buffer_t b;
	int order;
	isc_result_t result;
	unsigned int nlabels;

	/*
	 * Compare the current qname to the found name.  We need
	 * to know how many labels and bits are in common because
	 * we're going to have to split qname later on.
	 */
	namereln = dns_name_fullcompare(qctx->client->query.qname, qctx->fname,
					&order, &nlabels);
	INSIST(namereln == dns_namereln_subdomain);

	/*
	 * Keep a copy of the rdataset.  We have to do this because
	 * query_addrrset may clear 'rdataset' (to prevent the
	 * cleanup code from cleaning it up).
	 */
	trdataset = qctx->rdataset;

	/*
	 * Add the DNAME to the answer section.
	 */
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL)
		sigrdatasetp = &qctx->sigrdataset;

	if (WANTDNSSEC(qctx->client) &&
	    (qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) != 0)
	{
		dns_fixedname_init(&qctx->wildcardname);
		dns_name_copy(qctx->fname,
			      dns_fixedname_name(&qctx->wildcardname), NULL);
		qctx->need_wildcardproof = ISC_TRUE;
	}

	if (!qctx->is_zone && RECURSIONOK(qctx->client))
		query_prefetch(qctx->client, qctx->fname, qctx->rdataset);
	query_addrrset(qctx->client, &qctx->fname,
		       &qctx->rdataset, sigrdatasetp, qctx->dbuf,
		       DNS_SECTION_ANSWER);

	/*
	 * We set the PARTIALANSWER attribute so that if anything goes
	 * wrong later on, we'll return what we've got so far.
	 */
	qctx->client->query.attributes |= NS_QUERYATTR_PARTIALANSWER;

	/*
	 * Get the target name of the DNAME.
	 */
	tname = NULL;
	result = dns_message_gettempname(qctx->client->message, &tname);
	if (result != ISC_R_SUCCESS)
		return (query_done(qctx));

	result = dns_rdataset_first(trdataset);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (query_done(qctx));
	}

	dns_rdataset_current(trdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &dname, NULL);
	dns_rdata_reset(&rdata);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (query_done(qctx));
	}

	dns_name_clone(&dname.dname, tname);
	dns_rdata_freestruct(&dname);

	/*
	 * Construct the new qname consisting of
	 * <found name prefix>.<dname target>
	 */
	prefix = dns_fixedname_initname(&fixed);
	dns_name_split(qctx->client->query.qname, nlabels, prefix, NULL);
	INSIST(qctx->fname == NULL);
	qctx->dbuf = query_getnamebuf(qctx->client);
	if (qctx->dbuf == NULL) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (query_done(qctx));
	}
	qctx->fname = query_newname(qctx->client, qctx->dbuf, &b);
	if (qctx->fname == NULL) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (query_done(qctx));
	}
	result = dns_name_concatenate(prefix, tname, qctx->fname, NULL);
	dns_message_puttempname(qctx->client->message, &tname);

	/*
	 * RFC2672, section 4.1, subsection 3c says
	 * we should return YXDOMAIN if the constructed
	 * name would be too long.
	 */
	if (result == DNS_R_NAMETOOLONG)
		qctx->client->message->rcode = dns_rcode_yxdomain;
	if (result != ISC_R_SUCCESS)
		return (query_done(qctx));

	query_keepname(qctx->client, qctx->fname, qctx->dbuf);

	/*
	 * Synthesize a CNAME consisting of
	 *   <old qname> <dname ttl> CNAME <new qname>
	 *	    with <dname trust value>
	 *
	 * Synthesize a CNAME so old old clients that don't understand
	 * DNAME can chain.
	 *
	 * We do not try to synthesize a signature because we hope
	 * that security aware servers will understand DNAME.  Also,
	 * even if we had an online key, making a signature
	 * on-the-fly is costly, and not really legitimate anyway
	 * since the synthesized CNAME is NOT in the zone.
	 */
	result = query_addcname(qctx, trdataset->trust, trdataset->ttl);
	if (result != ISC_R_SUCCESS)
		return (query_done(qctx));

	/*
	 * Switch to the new qname and restart.
	 */
	ns_client_qnamereplace(qctx->client, qctx->fname);
	qctx->fname = NULL;
	qctx->want_restart = ISC_TRUE;
	if (!WANTRECURSION(qctx->client))
		qctx->options |= DNS_GETDB_NOLOG;

	query_addauth(qctx);

	return (query_done(qctx));
}

/*%
 * Add CNAME to repsonse.
 */
static isc_result_t
query_addcname(query_ctx_t *qctx, dns_trust_t trust, dns_ttl_t ttl) {
	ns_client_t *client = qctx->client;
	dns_rdataset_t *rdataset = NULL;
	dns_rdatalist_t *rdatalist = NULL;
	dns_rdata_t *rdata = NULL;
	isc_region_t r;
	dns_name_t *aname = NULL;
	isc_result_t result;

	result = dns_message_gettempname(client->message, &aname);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = dns_name_dup(client->query.qname, client->mctx, aname);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(client->message, &aname);
		return (result);
	}

	result = dns_message_gettemprdatalist(client->message, &rdatalist);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(client->message, &aname);
		return (result);
	}

	result = dns_message_gettemprdata(client->message, &rdata);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(client->message, &aname);
		dns_message_puttemprdatalist(client->message, &rdatalist);
		return (result);
	}

	result = dns_message_gettemprdataset(client->message, &rdataset);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(client->message, &aname);
		dns_message_puttemprdatalist(client->message, &rdatalist);
		dns_message_puttemprdata(client->message, &rdata);
		return (result);
	}

	rdatalist->type = dns_rdatatype_cname;
	rdatalist->rdclass = client->message->rdclass;
	rdatalist->ttl = ttl;

	dns_name_toregion(qctx->fname, &r);
	rdata->data = r.base;
	rdata->length = r.length;
	rdata->rdclass = client->message->rdclass;
	rdata->type = dns_rdatatype_cname;

	ISC_LIST_APPEND(rdatalist->rdata, rdata, link);
	RUNTIME_CHECK(dns_rdatalist_tordataset(rdatalist, rdataset)
		      == ISC_R_SUCCESS);
	rdataset->trust = trust;
	dns_rdataset_setownercase(rdataset, aname);

	query_addrrset(client, &aname, &rdataset, NULL, NULL,
		       DNS_SECTION_ANSWER);
	if (rdataset != NULL) {
		if (dns_rdataset_isassociated(rdataset))
			dns_rdataset_disassociate(rdataset);
		dns_message_puttemprdataset(client->message, &rdataset);
	}
	if (aname != NULL)
		dns_message_puttempname(client->message, &aname);

	return (ISC_R_SUCCESS);
}

/*%
 * Prepare to respond: determine whether a wildcard proof is needed,
 * check whether to filter AAAA answers, then hand off to query_respond()
 * or (for type ANY queries) query_respond_any().
 */
static isc_result_t
query_prepresponse(query_ctx_t *qctx) {
	if (WANTDNSSEC(qctx->client) &&
	    (qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) != 0)
	{
		dns_fixedname_init(&qctx->wildcardname);
		dns_name_copy(qctx->fname,
			      dns_fixedname_name(&qctx->wildcardname), NULL);
		qctx->need_wildcardproof = ISC_TRUE;
	}

	/*
	 * The filter-aaaa-on-v4 option should suppress AAAAs for IPv4
	 * clients if there is an A; filter-aaaa-on-v6 option does the same
	 * for IPv6 clients.
	 */
	qctx->client->filter_aaaa = dns_aaaa_ok;
	if (qctx->client->view->v4_aaaa != dns_aaaa_ok ||
	    qctx->client->view->v6_aaaa != dns_aaaa_ok)
	{
		isc_result_t result;
		result = ns_client_checkaclsilent(qctx->client, NULL,
						  qctx->client->view->aaaa_acl,
						  ISC_TRUE);
		if (result == ISC_R_SUCCESS &&
		    qctx->client->view->v4_aaaa != dns_aaaa_ok &&
		    is_v4_client(qctx->client))
			qctx->client->filter_aaaa = qctx->client->view->v4_aaaa;
		else if (result == ISC_R_SUCCESS &&
			 qctx->client->view->v6_aaaa != dns_aaaa_ok &&
			 is_v6_client(qctx->client))
			qctx->client->filter_aaaa = qctx->client->view->v6_aaaa;
	}


	if (qctx->type == dns_rdatatype_any) {
		return (query_respond_any(qctx));
	} else {
		return (query_respond(qctx));
	}
}

/*%
 * Add SOA to the authority section when sending negative responses
 * (or to the additional section if sending negative responses triggered
 * by RPZ rewriting.)
 */
static isc_result_t
query_addsoa(query_ctx_t *qctx, unsigned int override_ttl,
	     dns_section_t section)
{
	ns_client_t *client = qctx->client;
	dns_name_t *name;
	dns_dbnode_t *node;
	isc_result_t result, eresult;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	dns_rdataset_t **sigrdatasetp = NULL;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	CTRACE(ISC_LOG_DEBUG(3), "query_addsoa");
	/*
	 * Initialization.
	 */
	eresult = ISC_R_SUCCESS;
	name = NULL;
	rdataset = NULL;
	node = NULL;

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * Don't add the SOA record for test which set "-T nosoa".
	 */
	if (((client->sctx->options & NS_SERVER_NOSOA) != 0) &&
	    (!WANTDNSSEC(client) || !dns_rdataset_isassociated(qctx->rdataset)))
	{
		return (ISC_R_SUCCESS);
	}

	/*
	 * Get resources and make 'name' be the database origin.
	 */
	result = dns_message_gettempname(client->message, &name);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_name_init(name, NULL);
	dns_name_clone(dns_db_origin(qctx->db), name);
	rdataset = query_newrdataset(client);
	if (rdataset == NULL) {
		CTRACE(ISC_LOG_ERROR, "unable to allocate rdataset");
		eresult = DNS_R_SERVFAIL;
		goto cleanup;
	}
	if (WANTDNSSEC(client) && dns_db_issecure(qctx->db)) {
		sigrdataset = query_newrdataset(client);
		if (sigrdataset == NULL) {
			CTRACE(ISC_LOG_ERROR, "unable to allocate sigrdataset");
			eresult = DNS_R_SERVFAIL;
			goto cleanup;
		}
	}

	/*
	 * Find the SOA.
	 */
	result = dns_db_getoriginnode(qctx->db, &node);
	if (result == ISC_R_SUCCESS) {
		result = dns_db_findrdataset(qctx->db, node, qctx->version,
					     dns_rdatatype_soa, 0,
					     client->now,
					     rdataset, sigrdataset);
	} else {
		dns_fixedname_t foundname;
		dns_name_t *fname;

		fname = dns_fixedname_initname(&foundname);

		result = dns_db_findext(qctx->db, name, qctx->version,
					dns_rdatatype_soa,
					client->query.dboptions,
					0, &node, fname, &cm, &ci,
					rdataset, sigrdataset);
	}
	if (result != ISC_R_SUCCESS) {
		/*
		 * This is bad.  We tried to get the SOA RR at the zone top
		 * and it didn't work!
		 */
		CTRACE(ISC_LOG_ERROR, "unable to find SOA RR at zone apex");
		eresult = DNS_R_SERVFAIL;
	} else {
		/*
		 * Extract the SOA MINIMUM.
		 */
		dns_rdata_soa_t soa;
		dns_rdata_t rdata = DNS_RDATA_INIT;
		result = dns_rdataset_first(rdataset);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &soa, NULL);
		if (result != ISC_R_SUCCESS)
			goto cleanup;

		if (override_ttl != ISC_UINT32_MAX &&
		    override_ttl < rdataset->ttl)
		{
			rdataset->ttl = override_ttl;
			if (sigrdataset != NULL)
				sigrdataset->ttl = override_ttl;
		}

		/*
		 * Add the SOA and its SIG to the response, with the
		 * TTLs adjusted per RFC2308 section 3.
		 */
		if (rdataset->ttl > soa.minimum)
			rdataset->ttl = soa.minimum;
		if (sigrdataset != NULL && sigrdataset->ttl > soa.minimum)
			sigrdataset->ttl = soa.minimum;

		if (sigrdataset != NULL)
			sigrdatasetp = &sigrdataset;
		else
			sigrdatasetp = NULL;

		if (section == DNS_SECTION_ADDITIONAL)
			rdataset->attributes |= DNS_RDATASETATTR_REQUIRED;
		query_addrrset(client, &name, &rdataset,
			       sigrdatasetp, NULL, section);
	}

 cleanup:
	query_putrdataset(client, &rdataset);
	if (sigrdataset != NULL)
		query_putrdataset(client, &sigrdataset);
	if (name != NULL)
		query_releasename(client, &name);
	if (node != NULL)
		dns_db_detachnode(qctx->db, &node);

	return (eresult);
}

/*%
 * Add NS to authority section (used when the zone apex is already known).
 */
static isc_result_t
query_addns(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	isc_result_t result, eresult;
	dns_name_t *name = NULL, *fname;
	dns_dbnode_t *node = NULL;
	dns_fixedname_t foundname;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	dns_rdataset_t **sigrdatasetp = NULL;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	CTRACE(ISC_LOG_DEBUG(3), "query_addns");

	/*
	 * Initialization.
	 */
	eresult = ISC_R_SUCCESS;
	fname = dns_fixedname_initname(&foundname);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * Get resources and make 'name' be the database origin.
	 */
	result = dns_message_gettempname(client->message, &name);
	if (result != ISC_R_SUCCESS) {
		CTRACE(ISC_LOG_DEBUG(3),
		       "query_addns: dns_message_gettempname failed: done");
		return (result);
	}
	dns_name_init(name, NULL);
	dns_name_clone(dns_db_origin(qctx->db), name);
	rdataset = query_newrdataset(client);
	if (rdataset == NULL) {
		CTRACE(ISC_LOG_ERROR,
		       "query_addns: query_newrdataset failed");
		eresult = DNS_R_SERVFAIL;
		goto cleanup;
	}

	if (WANTDNSSEC(client) && dns_db_issecure(qctx->db)) {
		sigrdataset = query_newrdataset(client);
		if (sigrdataset == NULL) {
			CTRACE(ISC_LOG_ERROR,
			       "query_addns: query_newrdataset failed");
			eresult = DNS_R_SERVFAIL;
			goto cleanup;
		}
	}

	/*
	 * Find the NS rdataset.
	 */
	result = dns_db_getoriginnode(qctx->db, &node);
	if (result == ISC_R_SUCCESS) {
		result = dns_db_findrdataset(qctx->db, node, qctx->version,
					     dns_rdatatype_ns, 0, client->now,
					     rdataset, sigrdataset);
	} else {
		CTRACE(ISC_LOG_DEBUG(3), "query_addns: calling dns_db_find");
		result = dns_db_findext(qctx->db, name, NULL, dns_rdatatype_ns,
					client->query.dboptions, 0, &node,
					fname, &cm, &ci, rdataset, sigrdataset);
		CTRACE(ISC_LOG_DEBUG(3), "query_addns: dns_db_find complete");
	}
	if (result != ISC_R_SUCCESS) {
		CTRACE(ISC_LOG_ERROR,
		       "query_addns: "
		       "dns_db_findrdataset or dns_db_find failed");
		/*
		 * This is bad.  We tried to get the NS rdataset at the zone
		 * top and it didn't work!
		 */
		eresult = DNS_R_SERVFAIL;
	} else {
		if (sigrdataset != NULL) {
			sigrdatasetp = &sigrdataset;
		}
		query_addrrset(client, &name, &rdataset, sigrdatasetp, NULL,
			       DNS_SECTION_AUTHORITY);
	}

 cleanup:
	CTRACE(ISC_LOG_DEBUG(3), "query_addns: cleanup");
	query_putrdataset(client, &rdataset);
	if (sigrdataset != NULL) {
		query_putrdataset(client, &sigrdataset);
	}
	if (name != NULL) {
		query_releasename(client, &name);
	}
	if (node != NULL) {
		dns_db_detachnode(qctx->db, &node);
	}

	CTRACE(ISC_LOG_DEBUG(3), "query_addns: done");
	return (eresult);
}

/*%
 * Find the zone cut and add the best NS rrset to the authority section.
 */
static void
query_addbestns(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	dns_db_t *db = NULL, *zdb = NULL;
	dns_dbnode_t *node = NULL;
	dns_name_t *fname = NULL, *zfname = NULL;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	dns_rdataset_t *zrdataset = NULL, *zsigrdataset = NULL;
	isc_boolean_t is_zone = ISC_FALSE, use_zone = ISC_FALSE;
	isc_buffer_t *dbuf = NULL;
	isc_result_t result;
	dns_dbversion_t *version = NULL;
	dns_zone_t *zone = NULL;
	isc_buffer_t b;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	CTRACE(ISC_LOG_DEBUG(3), "query_addbestns");

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * Find the right database.
	 */
	result = query_getdb(client, client->query.qname, dns_rdatatype_ns, 0,
			     &zone, &db, &version, &is_zone);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

 db_find:
	/*
	 * We'll need some resources...
	 */
	dbuf = query_getnamebuf(client);
	if (dbuf == NULL) {
		goto cleanup;
	}
	fname = query_newname(client, dbuf, &b);
	rdataset = query_newrdataset(client);
	if (fname == NULL || rdataset == NULL) {
		goto cleanup;
	}

	/*
	 * Get the RRSIGs if the client requested them or if we may
	 * need to validate answers from the cache.
	 */
	if (WANTDNSSEC(client) || !is_zone) {
		sigrdataset = query_newrdataset(client);
		if (sigrdataset == NULL) {
			goto cleanup;
		}
	}

	/*
	 * Now look for the zonecut.
	 */
	if (is_zone) {
		result = dns_db_findext(db, client->query.qname, version,
					dns_rdatatype_ns,
					client->query.dboptions,
					client->now, &node, fname,
					&cm, &ci, rdataset, sigrdataset);
		if (result != DNS_R_DELEGATION) {
			goto cleanup;
		}
		if (USECACHE(client)) {
			query_keepname(client, fname, dbuf);
			dns_db_detachnode(db, &node);
			SAVE(zdb, db);
			SAVE(zfname, fname);
			SAVE(zrdataset, rdataset);
			SAVE(zsigrdataset, sigrdataset);
			version = NULL;
			dns_db_attach(client->view->cachedb, &db);
			is_zone = ISC_FALSE;
			goto db_find;
		}
	} else {
		result = dns_db_findzonecut(db, client->query.qname,
					    client->query.dboptions,
					    client->now, &node, fname,
					    rdataset, sigrdataset);
		if (result == ISC_R_SUCCESS) {
			if (zfname != NULL &&
			    !dns_name_issubdomain(fname, zfname)) {
				/*
				 * We found a zonecut in the cache, but our
				 * zone delegation is better.
				 */
				use_zone = ISC_TRUE;
			}
		} else if (result == ISC_R_NOTFOUND && zfname != NULL) {
			/*
			 * We didn't find anything in the cache, but we
			 * have a zone delegation, so use it.
			 */
			use_zone = ISC_TRUE;
		} else {
			goto cleanup;
		}
	}

	if (use_zone) {
		query_releasename(client, &fname);
		/*
		 * We've already done query_keepname() on
		 * zfname, so we must set dbuf to NULL to
		 * prevent query_addrrset() from trying to
		 * call query_keepname() again.
		 */
		dbuf = NULL;
		query_putrdataset(client, &rdataset);
		if (sigrdataset != NULL) {
			query_putrdataset(client, &sigrdataset);
		}

		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}
		dns_db_detach(&db);

		RESTORE(db, zdb);
		RESTORE(fname, zfname);
		RESTORE(rdataset, zrdataset);
		RESTORE(sigrdataset, zsigrdataset);
	}

	/*
	 * Attempt to validate RRsets that are pending or that are glue.
	 */
	if ((DNS_TRUST_PENDING(rdataset->trust) ||
	     (sigrdataset != NULL && DNS_TRUST_PENDING(sigrdataset->trust))) &&
	    !validate(client, db, fname, rdataset, sigrdataset) &&
	    !PENDINGOK(client->query.dboptions))
	{
		goto cleanup;
	}

	if ((DNS_TRUST_GLUE(rdataset->trust) ||
	     (sigrdataset != NULL && DNS_TRUST_GLUE(sigrdataset->trust))) &&
	    !validate(client, db, fname, rdataset, sigrdataset) &&
	    SECURE(client) && WANTDNSSEC(client))
	{
		goto cleanup;
	}

	/*
	 * If the answer is secure only add NS records if they are secure
	 * when the client may be looking for AD in the response.
	 */
	if (SECURE(client) && (WANTDNSSEC(client) || WANTAD(client)) &&
	    ((rdataset->trust != dns_trust_secure) ||
	    (sigrdataset != NULL && sigrdataset->trust != dns_trust_secure)))
	{
		goto cleanup;
	}

	/*
	 * If the client doesn't want DNSSEC we can discard the sigrdataset
	 * now.
	 */
	if (!WANTDNSSEC(client)) {
		query_putrdataset(client, &sigrdataset);
	}

	query_addrrset(client, &fname, &rdataset, &sigrdataset, dbuf,
		       DNS_SECTION_AUTHORITY);

 cleanup:
	if (rdataset != NULL) {
		query_putrdataset(client, &rdataset);
	}
	if (sigrdataset != NULL) {
		query_putrdataset(client, &sigrdataset);
	}
	if (fname != NULL) {
		query_releasename(client, &fname);
	}
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
	if (db != NULL) {
		dns_db_detach(&db);
	}
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}
	if (zdb != NULL) {
		query_putrdataset(client, &zrdataset);
		if (zsigrdataset != NULL)
			query_putrdataset(client, &zsigrdataset);
		if (zfname != NULL)
			query_releasename(client, &zfname);
		dns_db_detach(&zdb);
	}
}

static void
query_addwildcardproof(query_ctx_t *qctx, isc_boolean_t ispositive,
		       isc_boolean_t nodata)
{
	ns_client_t *client = qctx->client;
	isc_buffer_t *dbuf, b;
	dns_name_t *name;
	dns_name_t *fname = NULL;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	dns_fixedname_t wfixed;
	dns_name_t *wname;
	dns_dbnode_t *node = NULL;
	unsigned int options;
	unsigned int olabels, nlabels, labels;
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_nsec_t nsec;
	isc_boolean_t have_wname;
	int order;
	dns_fixedname_t cfixed;
	dns_name_t *cname;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	CTRACE(ISC_LOG_DEBUG(3), "query_addwildcardproof");

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL);

	/*
	 * If a name has been specifically flagged as needing
	 * a wildcard proof then it will have been copied to
	 * qctx->wildcardname. Otherwise we just use the client
	 * QNAME.
	 */
	if (qctx->need_wildcardproof) {
		name = dns_fixedname_name(&qctx->wildcardname);
	} else {
		name = client->query.qname;
	}

	/*
	 * Get the NOQNAME proof then if !ispositive
	 * get the NOWILDCARD proof.
	 *
	 * DNS_DBFIND_NOWILD finds the NSEC records that covers the
	 * name ignoring any wildcard.  From the owner and next names
	 * of this record you can compute which wildcard (if it exists)
	 * will match by finding the longest common suffix of the
	 * owner name and next names with the qname and prefixing that
	 * with the wildcard label.
	 *
	 * e.g.
	 *   Given:
	 *	example SOA
	 *	example NSEC b.example
	 *	b.example A
	 *	b.example NSEC a.d.example
	 *	a.d.example A
	 *	a.d.example NSEC g.f.example
	 *	g.f.example A
	 *	g.f.example NSEC z.i.example
	 *	z.i.example A
	 *	z.i.example NSEC example
	 *
	 *   QNAME:
	 *   a.example -> example NSEC b.example
	 *	owner common example
	 *	next common example
	 *	wild *.example
	 *   d.b.example -> b.example NSEC a.d.example
	 *	owner common b.example
	 *	next common example
	 *	wild *.b.example
	 *   a.f.example -> a.d.example NSEC g.f.example
	 *	owner common example
	 *	next common f.example
	 *	wild *.f.example
	 *  j.example -> z.i.example NSEC example
	 *	owner common example
	 *	next common example
	 *	wild *.example
	 */
	options = client->query.dboptions | DNS_DBFIND_NOWILD;
	wname = dns_fixedname_initname(&wfixed);
 again:
	have_wname = ISC_FALSE;
	/*
	 * We'll need some resources...
	 */
	dbuf = query_getnamebuf(client);
	if (dbuf == NULL)
		goto cleanup;
	fname = query_newname(client, dbuf, &b);
	rdataset = query_newrdataset(client);
	sigrdataset = query_newrdataset(client);
	if (fname == NULL || rdataset == NULL || sigrdataset == NULL)
		goto cleanup;

	result = dns_db_findext(qctx->db, name, qctx->version,
				dns_rdatatype_nsec, options, 0, &node,
				fname, &cm, &ci, rdataset, sigrdataset);
	if (node != NULL)
		dns_db_detachnode(qctx->db, &node);

	if (!dns_rdataset_isassociated(rdataset)) {
		/*
		 * No NSEC proof available, return NSEC3 proofs instead.
		 */
		cname = dns_fixedname_initname(&cfixed);
		/*
		 * Find the closest encloser.
		 */
		dns_name_copy(name, cname, NULL);
		while (result == DNS_R_NXDOMAIN) {
			labels = dns_name_countlabels(cname) - 1;
			/*
			 * Sanity check.
			 */
			if (labels == 0U)
				goto cleanup;
			dns_name_split(cname, labels, NULL, cname);
			result = dns_db_findext(qctx->db, cname, qctx->version,
						dns_rdatatype_nsec,
						options, 0, NULL, fname,
						&cm, &ci, NULL, NULL);
		}
		/*
		 * Add closest (provable) encloser NSEC3.
		 */
		query_findclosestnsec3(cname, qctx->db, qctx->version,
				       client, rdataset, sigrdataset,
				       fname, ISC_TRUE, cname);
		if (!dns_rdataset_isassociated(rdataset))
			goto cleanup;
		if (!ispositive)
			query_addrrset(client, &fname, &rdataset, &sigrdataset,
				       dbuf, DNS_SECTION_AUTHORITY);

		/*
		 * Replace resources which were consumed by query_addrrset.
		 */
		if (fname == NULL) {
			dbuf = query_getnamebuf(client);
			if (dbuf == NULL)
				goto cleanup;
			fname = query_newname(client, dbuf, &b);
		}

		if (rdataset == NULL)
			rdataset = query_newrdataset(client);
		else if (dns_rdataset_isassociated(rdataset))
			dns_rdataset_disassociate(rdataset);

		if (sigrdataset == NULL)
			sigrdataset = query_newrdataset(client);
		else if (dns_rdataset_isassociated(sigrdataset))
			dns_rdataset_disassociate(sigrdataset);

		if (fname == NULL || rdataset == NULL || sigrdataset == NULL)
			goto cleanup;
		/*
		 * Add no qname proof.
		 */
		labels = dns_name_countlabels(cname) + 1;
		if (dns_name_countlabels(name) == labels)
			dns_name_copy(name, wname, NULL);
		else
			dns_name_split(name, labels, NULL, wname);

		query_findclosestnsec3(wname, qctx->db, qctx->version,
				       client, rdataset, sigrdataset,
				       fname, ISC_FALSE, NULL);
		if (!dns_rdataset_isassociated(rdataset))
			goto cleanup;
		query_addrrset(client, &fname, &rdataset, &sigrdataset,
			       dbuf, DNS_SECTION_AUTHORITY);

		if (ispositive)
			goto cleanup;

		/*
		 * Replace resources which were consumed by query_addrrset.
		 */
		if (fname == NULL) {
			dbuf = query_getnamebuf(client);
			if (dbuf == NULL)
				goto cleanup;
			fname = query_newname(client, dbuf, &b);
		}

		if (rdataset == NULL)
			rdataset = query_newrdataset(client);
		else if (dns_rdataset_isassociated(rdataset))
			dns_rdataset_disassociate(rdataset);

		if (sigrdataset == NULL)
			sigrdataset = query_newrdataset(client);
		else if (dns_rdataset_isassociated(sigrdataset))
			dns_rdataset_disassociate(sigrdataset);

		if (fname == NULL || rdataset == NULL || sigrdataset == NULL)
			goto cleanup;
		/*
		 * Add the no wildcard proof.
		 */
		result = dns_name_concatenate(dns_wildcardname,
					      cname, wname, NULL);
		if (result != ISC_R_SUCCESS)
			goto cleanup;

		query_findclosestnsec3(wname, qctx->db, qctx->version,
				       client, rdataset, sigrdataset,
				       fname, nodata, NULL);
		if (!dns_rdataset_isassociated(rdataset))
			goto cleanup;
		query_addrrset(client, &fname, &rdataset, &sigrdataset,
			       dbuf, DNS_SECTION_AUTHORITY);

		goto cleanup;
	} else if (result == DNS_R_NXDOMAIN) {
		if (!ispositive)
			result = dns_rdataset_first(rdataset);
		if (result == ISC_R_SUCCESS) {
			dns_rdataset_current(rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &nsec, NULL);
		}
		if (result == ISC_R_SUCCESS) {
			(void)dns_name_fullcompare(name, fname, &order,
						   &olabels);
			(void)dns_name_fullcompare(name, &nsec.next, &order,
						   &nlabels);
			/*
			 * Check for a pathological condition created when
			 * serving some malformed signed zones and bail out.
			 */
			if (dns_name_countlabels(name) == nlabels)
				goto cleanup;

			if (olabels > nlabels)
				dns_name_split(name, olabels, NULL, wname);
			else
				dns_name_split(name, nlabels, NULL, wname);
			result = dns_name_concatenate(dns_wildcardname,
						      wname, wname, NULL);
			if (result == ISC_R_SUCCESS)
				have_wname = ISC_TRUE;
			dns_rdata_freestruct(&nsec);
		}
		query_addrrset(client, &fname, &rdataset, &sigrdataset,
			       dbuf, DNS_SECTION_AUTHORITY);
	}
	if (rdataset != NULL)
		query_putrdataset(client, &rdataset);
	if (sigrdataset != NULL)
		query_putrdataset(client, &sigrdataset);
	if (fname != NULL)
		query_releasename(client, &fname);
	if (have_wname) {
		ispositive = ISC_TRUE;	/* prevent loop */
		if (!dns_name_equal(name, wname)) {
			name = wname;
			goto again;
		}
	}
 cleanup:
	if (rdataset != NULL)
		query_putrdataset(client, &rdataset);
	if (sigrdataset != NULL)
		query_putrdataset(client, &sigrdataset);
	if (fname != NULL)
		query_releasename(client, &fname);
}

/*%
 * Add NS records, and NSEC/NSEC3 wildcard proof records if needed,
 * to the authority section.
 */
static void
query_addauth(query_ctx_t *qctx) {
	CCTRACE(ISC_LOG_DEBUG(3), "query_addauth");
	/*
	 * Add NS records to the authority section (if we haven't already
	 * added them to the answer section).
	 */
	if (!qctx->want_restart && !NOAUTHORITY(qctx->client)) {
		if (qctx->is_zone) {
			if (!qctx->answer_has_ns) {
				(void)query_addns(qctx);
			}
		} else if (!qctx->answer_has_ns &&
			   qctx->qtype != dns_rdatatype_ns)
		{
			if (qctx->fname != NULL) {
				query_releasename(qctx->client, &qctx->fname);
			}
			query_addbestns(qctx);
		}
	}

	/*
	 * Add NSEC records to the authority section if they're needed for
	 * DNSSEC wildcard proofs.
	 */
	if (qctx->need_wildcardproof && dns_db_issecure(qctx->db))
		query_addwildcardproof(qctx, ISC_TRUE, ISC_FALSE);
}

/*
 * Find the sort order of 'rdata' in the topology-like
 * ACL forming the second element in a 2-element top-level
 * sortlist statement.
 */
static int
query_sortlist_order_2element(const dns_rdata_t *rdata, const void *arg) {
	isc_netaddr_t netaddr;

	if (rdata_tonetaddr(rdata, &netaddr) != ISC_R_SUCCESS)
		return (INT_MAX);
	return (ns_sortlist_addrorder2(&netaddr, arg));
}

/*
 * Find the sort order of 'rdata' in the matching element
 * of a 1-element top-level sortlist statement.
 */
static int
query_sortlist_order_1element(const dns_rdata_t *rdata, const void *arg) {
	isc_netaddr_t netaddr;

	if (rdata_tonetaddr(rdata, &netaddr) != ISC_R_SUCCESS)
		return (INT_MAX);
	return (ns_sortlist_addrorder1(&netaddr, arg));
}

/*
 * Find the sortlist statement that applies to 'client' and set up
 * the sortlist info in in client->message appropriately.
 */
static void
query_setup_sortlist(query_ctx_t *qctx) {
	isc_netaddr_t netaddr;
	ns_client_t *client = qctx->client;
	dns_aclenv_t *env = ns_interfacemgr_getaclenv(client->interface->mgr);
	const void *order_arg = NULL;

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	switch (ns_sortlist_setup(client->view->sortlist, env,
				  &netaddr, &order_arg))
	{
	case NS_SORTLISTTYPE_1ELEMENT:
		dns_message_setsortorder(client->message,
					 query_sortlist_order_1element,
					 env, NULL, order_arg);
		break;
	case NS_SORTLISTTYPE_2ELEMENT:
		dns_message_setsortorder(client->message,
					 query_sortlist_order_2element,
					 env, order_arg, NULL);
		break;
	case NS_SORTLISTTYPE_NONE:
		break;
	default:
		INSIST(0);
		break;
	}
}

/*
 * When sending a referral, if the answer to the question is
 * in the glue, sort it to the start of the additional section.
 */
static inline void
query_glueanswer(query_ctx_t *qctx) {
	const dns_namelist_t *secs = qctx->client->message->sections;
	const dns_section_t section = DNS_SECTION_ADDITIONAL;
	dns_name_t *name;
	dns_message_t *msg;
	dns_rdataset_t *rdataset = NULL;

	if (!ISC_LIST_EMPTY(secs[DNS_SECTION_ANSWER]) ||
	    qctx->client->message->rcode != dns_rcode_noerror ||
	    (qctx->qtype != dns_rdatatype_a &&
	     qctx->qtype != dns_rdatatype_aaaa))
	{
		return;
	}

	msg = qctx->client->message;
	for (name = ISC_LIST_HEAD(msg->sections[section]);
	     name != NULL;
	     name = ISC_LIST_NEXT(name, link))
		if (dns_name_equal(name, qctx->client->query.qname)) {
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link))
				if (rdataset->type == qctx->qtype)
					break;
			break;
		}
	if (rdataset != NULL) {
		ISC_LIST_UNLINK(msg->sections[section], name, link);
		ISC_LIST_PREPEND(msg->sections[section], name, link);
		ISC_LIST_UNLINK(name->list, rdataset, link);
		ISC_LIST_PREPEND(name->list, rdataset, link);
		rdataset->attributes |= DNS_RDATASETATTR_REQUIRED;
	}
}

/*%
 * Finalize this phase of the query process:
 *
 * - Clean up
 * - If we have an answer ready (positive or negative), send it.
 * - If we need to restart for a chaining query, call ns__query_start() again.
 * - If we've started recursion, then just clean up; things will be
 *   restarted via fetch_callback()/query_resume().
 */
static isc_result_t
query_done(query_ctx_t *qctx) {
	const dns_namelist_t *secs = qctx->client->message->sections;
	CCTRACE(ISC_LOG_DEBUG(3), "query_done");

	PROCESS_HOOK(NS_QUERY_DONE_BEGIN, qctx);

	/*
	 * General cleanup.
	 */
	qctx->rpz_st = qctx->client->query.rpz_st;
	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) == 0)
	{
		rpz_match_clear(qctx->rpz_st);
		qctx->rpz_st->state &= ~DNS_RPZ_DONE_QNAME;
	}

	qctx_clean(qctx);
	qctx_freedata(qctx);

	/*
	 * Clear the AA bit if we're not authoritative.
	 */
	if (qctx->client->query.restarts == 0 && !qctx->authoritative) {
		qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AA;
	}

	/*
	 * Do we need to restart the query (e.g. for CNAME chaining)?
	 */
	if (qctx->want_restart && qctx->client->query.restarts < MAX_RESTARTS) {
		qctx->client->query.restarts++;
		return (ns__query_start(qctx));
	}

	if (qctx->want_stale) {
		dns_ttl_t stale_ttl = 0;
		isc_result_t result;
		isc_boolean_t staleanswersok = ISC_FALSE;

		/*
		 * Stale answers only make sense if stale_ttl > 0 but
		 * we want rndc to be able to control returning stale
		 * answers if they are configured.
		 */
		dns_db_attach(qctx->client->view->cachedb, &qctx->db);
		result = dns_db_getservestalettl(qctx->db, &stale_ttl);
		if (result == ISC_R_SUCCESS && stale_ttl > 0)  {
			switch (qctx->client->view->staleanswersok) {
			case dns_stale_answer_yes:
				staleanswersok = ISC_TRUE;
				break;
			case dns_stale_answer_conf:
				staleanswersok =
					qctx->client->view->staleanswersenable;
				break;
			case dns_stale_answer_no:
				staleanswersok = ISC_FALSE;
				break;
			}
		} else {
			staleanswersok = ISC_FALSE;
		}

		if (staleanswersok) {
			qctx->client->query.dboptions |= DNS_DBFIND_STALEOK;
			inc_stats(qctx->client, ns_statscounter_trystale);
			if (qctx->client->query.fetch != NULL)
				dns_resolver_destroyfetch(
						   &qctx->client->query.fetch);
			return (query_lookup(qctx));
		}
		dns_db_detach(&qctx->db);
		qctx->want_stale = ISC_FALSE;
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (query_done(qctx));
	}

	if (qctx->result != ISC_R_SUCCESS &&
	    (!PARTIALANSWER(qctx->client) || WANTRECURSION(qctx->client) ||
	     qctx->result == DNS_R_DROP))
	{
		if (qctx->result == DNS_R_DUPLICATE ||
		    qctx->result == DNS_R_DROP)
		{
			/*
			 * This was a duplicate query that we are
			 * recursing on or the result of rate limiting.
			 * Don't send a response now for a duplicate query,
			 * because the original will still cause a response.
			 */
			query_next(qctx->client, qctx->result);
		} else {
			/*
			 * If we don't have any answer to give the client,
			 * or if the client requested recursion and thus wanted
			 * the complete answer, send an error response.
			 */
			INSIST(qctx->line >= 0);
			query_error(qctx->client, qctx->result, qctx->line);
		}

		ns_client_detach(&qctx->client);
		return (qctx->result);
	}

	/*
	 * If we're recursing then just return; the query will
	 * resume when recursion ends.
	 */
	if (RECURSING(qctx->client)) {
		return (qctx->result);
	}

	/*
	 * We are done.  Set up sortlist data for the message
	 * rendering code, sort the answer to the front of the
	 * additional section if necessary, make a final tweak
	 * to the AA bit if the auth-nxdomain config option
	 * says so, then render and send the response.
	 */
	query_setup_sortlist(qctx);
	query_glueanswer(qctx);

	if (qctx->client->message->rcode == dns_rcode_nxdomain &&
	    qctx->client->view->auth_nxdomain == ISC_TRUE)
	{
		qctx->client->message->flags |= DNS_MESSAGEFLAG_AA;
	}

	/*
	 * If the response is somehow unexpected for the client and this
	 * is a result of recursion, return an error to the caller
	 * to indicate it may need to be logged.
	 */
	if (qctx->resuming &&
	    (ISC_LIST_EMPTY(secs[DNS_SECTION_ANSWER]) ||
	     qctx->client->message->rcode != dns_rcode_noerror))
	{
		qctx->result = ISC_R_FAILURE;
	}

	query_send(qctx->client);

	ns_client_detach(&qctx->client);
	return (qctx->result);
}

static inline void
log_tat(ns_client_t *client) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char clientbuf[ISC_NETADDR_FORMATSIZE];
	char classname[DNS_RDATACLASS_FORMATSIZE];
	isc_netaddr_t netaddr;
	char *tags = NULL;
	size_t taglen = 0;

	if (!isc_log_wouldlog(ns_lctx, ISC_LOG_INFO)) {
		return;
	}

	if ((client->query.qtype != dns_rdatatype_null ||
	     !dns_name_istat(client->query.qname)) &&
	    (client->keytag == NULL ||
	     client->query.qtype != dns_rdatatype_dnskey))
	{
		return;
	}

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	dns_name_format(client->query.qname, namebuf, sizeof(namebuf));
	isc_netaddr_format(&client->destaddr, clientbuf, sizeof(clientbuf));
	dns_rdataclass_format(client->view->rdclass, classname,
			      sizeof(classname));

	if (client->query.qtype == dns_rdatatype_dnskey) {
		isc_uint16_t keytags = client->keytag_len / 2;
		size_t len = taglen = sizeof("65000") * keytags + 1;
		char *cp = tags = isc_mem_get(client->mctx, taglen);
		int i = 0;

		INSIST(client->keytag != NULL);
		if (tags != NULL) {
			while (keytags-- > 0U) {
				int n;
				isc_uint16_t keytag;
				keytag = (client->keytag[i * 2] << 8) |
					 client->keytag[i * 2 + 1];
				n = snprintf(cp, len, " %u", keytag);
				if (n > 0 && (size_t)n <= len) {
					cp += n;
					len -= n;
					i++;
				} else {
					break;
				}
			}
		}
	}

	isc_log_write(ns_lctx, NS_LOGCATEGORY_TAT, NS_LOGMODULE_QUERY,
		      ISC_LOG_INFO, "trust-anchor-telemetry '%s/%s' from %s%s",
		      namebuf, classname, clientbuf, tags != NULL? tags : "");
	if (tags != NULL) {
		isc_mem_put(client->mctx, tags, taglen);
	}
}

static inline void
log_query(ns_client_t *client, unsigned int flags, unsigned int extflags) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typename[DNS_RDATATYPE_FORMATSIZE];
	char classname[DNS_RDATACLASS_FORMATSIZE];
	char onbuf[ISC_NETADDR_FORMATSIZE];
	char ecsbuf[DNS_ECS_FORMATSIZE + sizeof(" [ECS ]") - 1] = { 0 };
	char ednsbuf[sizeof("E(65535)")] = { 0 };
	dns_rdataset_t *rdataset;
	int level = ISC_LOG_INFO;

	if (! isc_log_wouldlog(ns_lctx, level))
		return;

	rdataset = ISC_LIST_HEAD(client->query.qname->list);
	INSIST(rdataset != NULL);
	dns_name_format(client->query.qname, namebuf, sizeof(namebuf));
	dns_rdataclass_format(rdataset->rdclass, classname, sizeof(classname));
	dns_rdatatype_format(rdataset->type, typename, sizeof(typename));
	isc_netaddr_format(&client->destaddr, onbuf, sizeof(onbuf));

	if (client->ednsversion >= 0)
		snprintf(ednsbuf, sizeof(ednsbuf), "E(%hd)",
			 client->ednsversion);

	if (HAVEECS(client)) {
		strlcpy(ecsbuf, " [ECS ", sizeof(ecsbuf));
		dns_ecs_format(&client->ecs, ecsbuf + 6, sizeof(ecsbuf) - 6);
		strlcat(ecsbuf, "]", sizeof(ecsbuf));
	}

	ns_client_log(client, NS_LOGCATEGORY_QUERIES, NS_LOGMODULE_QUERY,
		      level, "query: %s %s %s %s%s%s%s%s%s%s (%s)%s",
		      namebuf, classname, typename,
		      WANTRECURSION(client) ? "+" : "-",
		      (client->signer != NULL) ? "S" : "", ednsbuf,
		      TCP(client) ? "T" : "",
		      ((extflags & DNS_MESSAGEEXTFLAG_DO) != 0) ? "D" : "",
		      ((flags & DNS_MESSAGEFLAG_CD) != 0) ? "C" : "",
		      HAVECOOKIE(client) ? "V" : WANTCOOKIE(client) ? "K" : "",
		      onbuf, ecsbuf);
}

static inline void
log_queryerror(ns_client_t *client, isc_result_t result, int line, int level) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typename[DNS_RDATATYPE_FORMATSIZE];
	char classname[DNS_RDATACLASS_FORMATSIZE];
	const char *namep, *typep, *classp, *sep1, *sep2;
	dns_rdataset_t *rdataset;

	if (!isc_log_wouldlog(ns_lctx, level))
		return;

	namep = typep = classp = sep1 = sep2 = "";

	/*
	 * Query errors can happen for various reasons.  In some cases we cannot
	 * even assume the query contains a valid question section, so we should
	 * expect exceptional cases.
	 */
	if (client->query.origqname != NULL) {
		dns_name_format(client->query.origqname, namebuf,
				sizeof(namebuf));
		namep = namebuf;
		sep1 = " for ";

		rdataset = ISC_LIST_HEAD(client->query.origqname->list);
		if (rdataset != NULL) {
			dns_rdataclass_format(rdataset->rdclass, classname,
					      sizeof(classname));
			classp = classname;
			dns_rdatatype_format(rdataset->type, typename,
					     sizeof(typename));
			typep = typename;
			sep2 = "/";
		}
	}

	ns_client_log(client, NS_LOGCATEGORY_QUERY_ERRORS, NS_LOGMODULE_QUERY,
		      level, "query failed (%s)%s%s%s%s%s%s at %s:%d",
		      isc_result_totext(result), sep1, namep, sep2,
		      classp, sep2, typep, __FILE__, line);
}

void
ns_query_start(ns_client_t *client) {
	isc_result_t result;
	dns_message_t *message = client->message;
	dns_rdataset_t *rdataset;
	ns_client_t *qclient;
	dns_rdatatype_t qtype;
	unsigned int saved_extflags = client->extflags;
	unsigned int saved_flags = client->message->flags;

	REQUIRE(NS_CLIENT_VALID(client));

	CTRACE(ISC_LOG_DEBUG(3), "ns_query_start");

	/*
	 * Test only.
	 */
	if (((client->sctx->options & NS_SERVER_CLIENTTEST) != 0) &&
	    !TCP(client))
	{
		result = ns_client_replace(client);
		if (result == ISC_R_SHUTTINGDOWN) {
			ns_client_next(client, result);
			return;
		} else if (result != ISC_R_SUCCESS) {
			query_error(client, result, __LINE__);
			return;
		}
	}

	/*
	 * Ensure that appropriate cleanups occur.
	 */
	client->next = query_next_callback;

	/*
	 * Behave as if we don't support DNSSEC if not enabled.
	 */
	if (!client->view->enablednssec) {
		message->flags &= ~DNS_MESSAGEFLAG_CD;
		client->extflags &= ~DNS_MESSAGEEXTFLAG_DO;
	}

	if ((message->flags & DNS_MESSAGEFLAG_RD) != 0)
		client->query.attributes |= NS_QUERYATTR_WANTRECURSION;

	if ((client->extflags & DNS_MESSAGEEXTFLAG_DO) != 0)
		client->attributes |= NS_CLIENTATTR_WANTDNSSEC;

	switch (client->view->minimalresponses) {
	case dns_minimal_no:
		break;
	case dns_minimal_yes:
		client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
					     NS_QUERYATTR_NOADDITIONAL);
		break;
	case dns_minimal_noauth:
		client->query.attributes |= NS_QUERYATTR_NOAUTHORITY;
		break;
	case dns_minimal_noauthrec:
		if ((message->flags & DNS_MESSAGEFLAG_RD) != 0)
			client->query.attributes |= NS_QUERYATTR_NOAUTHORITY;
		break;
	}

	if (client->view->cachedb == NULL || !client->view->recursion) {
		/*
		 * We don't have a cache.  Turn off cache support and
		 * recursion.
		 */
		client->query.attributes &=
			~(NS_QUERYATTR_RECURSIONOK|NS_QUERYATTR_CACHEOK);
		client->attributes |= NS_CLIENTATTR_NOSETFC;
	} else if ((client->attributes & NS_CLIENTATTR_RA) == 0 ||
		   (message->flags & DNS_MESSAGEFLAG_RD) == 0) {
		/*
		 * If the client isn't allowed to recurse (due to
		 * "recursion no", the allow-recursion ACL, or the
		 * lack of a resolver in this view), or if it
		 * doesn't want recursion, turn recursion off.
		 */
		client->query.attributes &= ~NS_QUERYATTR_RECURSIONOK;
		client->attributes |= NS_CLIENTATTR_NOSETFC;
	}

	/*
	 * Check for multiple question queries, since edns1 is dead.
	 */
	if (message->counts[DNS_SECTION_QUESTION] > 1) {
		query_error(client, DNS_R_FORMERR, __LINE__);
		return;
	}

	/*
	 * Get the question name.
	 */
	result = dns_message_firstname(message, DNS_SECTION_QUESTION);
	if (result != ISC_R_SUCCESS) {
		query_error(client, result, __LINE__);
		return;
	}
	dns_message_currentname(message, DNS_SECTION_QUESTION,
				&client->query.qname);
	client->query.origqname = client->query.qname;
	result = dns_message_nextname(message, DNS_SECTION_QUESTION);
	if (result != ISC_R_NOMORE) {
		if (result == ISC_R_SUCCESS) {
			/*
			 * There's more than one QNAME in the question
			 * section.
			 */
			query_error(client, DNS_R_FORMERR, __LINE__);
		} else
			query_error(client, result, __LINE__);
		return;
	}

	if ((client->sctx->options & NS_SERVER_LOGQUERIES) != 0)
		log_query(client, saved_flags, saved_extflags);

	/*
	 * Check for meta-queries like IXFR and AXFR.
	 */
	rdataset = ISC_LIST_HEAD(client->query.qname->list);
	INSIST(rdataset != NULL);
	client->query.qtype = qtype = rdataset->type;
	dns_rdatatypestats_increment(client->sctx->rcvquerystats, qtype);

	log_tat(client);

	if (dns_rdatatype_ismeta(qtype)) {
		switch (qtype) {
		case dns_rdatatype_any:
			break; /* Let the query logic handle it. */
		case dns_rdatatype_ixfr:
		case dns_rdatatype_axfr:
			ns_xfr_start(client, rdataset->type);
			return;
		case dns_rdatatype_maila:
		case dns_rdatatype_mailb:
			query_error(client, DNS_R_NOTIMP, __LINE__);
			return;
		case dns_rdatatype_tkey:
			result = dns_tkey_processquery(client->message,
						    client->sctx->tkeyctx,
						    client->view->dynamickeys);
			if (result == ISC_R_SUCCESS)
				query_send(client);
			else
				query_error(client, result, __LINE__);
			return;
		default: /* TSIG, etc. */
			query_error(client, DNS_R_FORMERR, __LINE__);
			return;
		}
	}

	/*
	 * Turn on minimal response for (C)DNSKEY and (C)DS queries.
	 */
	if (qtype == dns_rdatatype_dnskey || qtype == dns_rdatatype_ds ||
	    qtype == dns_rdatatype_cdnskey || qtype == dns_rdatatype_cds)
	{
		client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
					     NS_QUERYATTR_NOADDITIONAL);
	}

	/*
	 * Maybe turn on minimal responses for ANY queries.
	 */
	if (qtype == dns_rdatatype_any &&
	    client->view->minimal_any && !TCP(client))
		client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
					     NS_QUERYATTR_NOADDITIONAL);

	/*
	 * Turn on minimal responses for EDNS/UDP bufsize 512 queries.
	 */
	if (client->ednsversion >= 0 && client->udpsize <= 512U && !TCP(client))
		client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
					     NS_QUERYATTR_NOADDITIONAL);

	/*
	 * If the client has requested that DNSSEC checking be disabled,
	 * allow lookups to return pending data and instruct the resolver
	 * to return data before validation has completed.
	 *
	 * We don't need to set DNS_DBFIND_PENDINGOK when validation is
	 * disabled as there will be no pending data.
	 */
	if (message->flags & DNS_MESSAGEFLAG_CD ||
	    qtype == dns_rdatatype_rrsig)
	{
		client->query.dboptions |= DNS_DBFIND_PENDINGOK;
		client->query.fetchoptions |= DNS_FETCHOPT_NOVALIDATE;
	} else if (!client->view->enablevalidation)
		client->query.fetchoptions |= DNS_FETCHOPT_NOVALIDATE;

	/*
	 * Allow glue NS records to be added to the authority section
	 * if the answer is secure.
	 */
	if (message->flags & DNS_MESSAGEFLAG_CD)
		client->query.attributes &= ~NS_QUERYATTR_SECURE;

	/*
	 * Set NS_CLIENTATTR_WANTAD if the client has set AD in the query.
	 * This allows AD to be returned on queries without DO set.
	 */
	if ((message->flags & DNS_MESSAGEFLAG_AD) != 0)
		client->attributes |= NS_CLIENTATTR_WANTAD;

	/*
	 * This is an ordinary query.
	 */
	result = dns_message_reply(message, ISC_TRUE);
	if (result != ISC_R_SUCCESS) {
		query_next(client, result);
		return;
	}

	/*
	 * Assume authoritative response until it is known to be
	 * otherwise.
	 *
	 * If "-T noaa" has been set on the command line don't set
	 * AA on authoritative answers.
	 */
	if ((client->sctx->options & NS_SERVER_NOAA) == 0)
		message->flags |= DNS_MESSAGEFLAG_AA;

	/*
	 * Set AD.  We must clear it if we add non-validated data to a
	 * response.
	 */
	if (WANTDNSSEC(client) || WANTAD(client))
		message->flags |= DNS_MESSAGEFLAG_AD;

	qclient = NULL;
	ns_client_attach(client, &qclient);
	(void)query_setup(qclient, qtype);
}
