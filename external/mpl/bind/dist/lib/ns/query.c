/*	$NetBSD: query.c,v 1.16 2023/01/25 21:43:32 christos Exp $	*/

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

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <isc/hex.h>
#include <isc/mem.h>
#include <isc/once.h>
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
#include <ns/hooks.h>
#include <ns/interfacemgr.h>
#include <ns/log.h>
#include <ns/server.h>
#include <ns/sortlist.h>
#include <ns/stats.h>
#include <ns/xfrout.h>

#include <ns/pfilter.h>

#if 0
/*
 * It has been recommended that DNS64 be changed to return excluded
 * AAAA addresses if DNS64 synthesis does not occur.  This minimises
 * the impact on the lookup results.  While most DNS AAAA lookups are
 * done to send IP packets to a host, not all of them are and filtering
 * excluded addresses has a negative impact on those uses.
 */
#define dns64_bis_return_excluded_addresses 1
#endif /* if 0 */

/*%
 * Maximum number of chained queries before we give up
 * to prevent CNAME loops.
 */
#define MAX_RESTARTS 16

#define QUERY_ERROR(qctx, r)                  \
	do {                                  \
		(qctx)->result = r;           \
		(qctx)->want_restart = false; \
		(qctx)->line = __LINE__;      \
	} while (0)

/*% Partial answer? */
#define PARTIALANSWER(c) \
	(((c)->query.attributes & NS_QUERYATTR_PARTIALANSWER) != 0)
/*% Use Cache? */
#define USECACHE(c) (((c)->query.attributes & NS_QUERYATTR_CACHEOK) != 0)
/*% Recursion OK? */
#define RECURSIONOK(c) (((c)->query.attributes & NS_QUERYATTR_RECURSIONOK) != 0)
/*% Recursing? */
#define RECURSING(c) (((c)->query.attributes & NS_QUERYATTR_RECURSING) != 0)
/*% Want Recursion? */
#define WANTRECURSION(c) \
	(((c)->query.attributes & NS_QUERYATTR_WANTRECURSION) != 0)
/*% Is TCP? */
#define TCP(c) (((c)->attributes & NS_CLIENTATTR_TCP) != 0)

/*% Want DNSSEC? */
#define WANTDNSSEC(c) (((c)->attributes & NS_CLIENTATTR_WANTDNSSEC) != 0)
/*% Want WANTAD? */
#define WANTAD(c) (((c)->attributes & NS_CLIENTATTR_WANTAD) != 0)
/*% Client presented a valid COOKIE. */
#define HAVECOOKIE(c) (((c)->attributes & NS_CLIENTATTR_HAVECOOKIE) != 0)
/*% Client presented a COOKIE. */
#define WANTCOOKIE(c) (((c)->attributes & NS_CLIENTATTR_WANTCOOKIE) != 0)
/*% Client presented a CLIENT-SUBNET option. */
#define HAVEECS(c) (((c)->attributes & NS_CLIENTATTR_HAVEECS) != 0)
/*% No authority? */
#define NOAUTHORITY(c) (((c)->query.attributes & NS_QUERYATTR_NOAUTHORITY) != 0)
/*% No additional? */
#define NOADDITIONAL(c) \
	(((c)->query.attributes & NS_QUERYATTR_NOADDITIONAL) != 0)
/*% Secure? */
#define SECURE(c) (((c)->query.attributes & NS_QUERYATTR_SECURE) != 0)
/*% DNS64 A lookup? */
#define DNS64(c) (((c)->query.attributes & NS_QUERYATTR_DNS64) != 0)

#define DNS64EXCLUDE(c) \
	(((c)->query.attributes & NS_QUERYATTR_DNS64EXCLUDE) != 0)

#define REDIRECT(c) (((c)->query.attributes & NS_QUERYATTR_REDIRECT) != 0)

/*% Was the client already sent a response? */
#define QUERY_ANSWERED(q) (((q)->attributes & NS_QUERYATTR_ANSWERED) != 0)

/*% Have we already processed an answer via stale-answer-client-timeout? */
#define QUERY_STALEPENDING(q) \
	(((q)->attributes & NS_QUERYATTR_STALEPENDING) != 0)

/*% Does the query allow stale data in the response? */
#define QUERY_STALEOK(q) (((q)->attributes & NS_QUERYATTR_STALEOK) != 0)

/*% Does the query wants to check for stale RRset due to a timeout? */
#define QUERY_STALETIMEOUT(q) (((q)->dboptions & DNS_DBFIND_STALETIMEOUT) != 0)

/*% Does the rdataset 'r' have an attached 'No QNAME Proof'? */
#define NOQNAME(r) (((r)->attributes & DNS_RDATASETATTR_NOQNAME) != 0)

/*% Does the rdataset 'r' contain a stale answer? */
#define STALE(r) (((r)->attributes & DNS_RDATASETATTR_STALE) != 0)

/*% Does the rdataset 'r' is stale and within stale-refresh-time? */
#define STALE_WINDOW(r) (((r)->attributes & DNS_RDATASETATTR_STALE_WINDOW) != 0)

#ifdef WANT_QUERYTRACE
static void
client_trace(ns_client_t *client, int level, const char *message) {
	if (client != NULL && client->query.qname != NULL) {
		if (isc_log_wouldlog(ns_lctx, level)) {
			char qbuf[DNS_NAME_FORMATSIZE];
			char tbuf[DNS_RDATATYPE_FORMATSIZE];
			dns_name_format(client->query.qname, qbuf,
					sizeof(qbuf));
			dns_rdatatype_format(client->query.qtype, tbuf,
					     sizeof(tbuf));
			isc_log_write(ns_lctx, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_QUERY, level,
				      "query client=%p thread=0x%" PRIxPTR
				      "(%s/%s): %s",
				      client, isc_thread_self(), qbuf, tbuf,
				      message);
		}
	} else {
		isc_log_write(ns_lctx, NS_LOGCATEGORY_CLIENT,
			      NS_LOGMODULE_QUERY, level,
			      "query client=%p thread=0x%" PRIxPTR
			      "(<unknown-query>): %s",
			      client, isc_thread_self(), message);
	}
}
#define CTRACE(l, m)  client_trace(client, l, m)
#define CCTRACE(l, m) client_trace(qctx->client, l, m)
#else /* ifdef WANT_QUERYTRACE */
#define CTRACE(l, m)  ((void)m)
#define CCTRACE(l, m) ((void)m)
#endif /* WANT_QUERYTRACE */

#define DNS_GETDB_NOEXACT    0x01U
#define DNS_GETDB_NOLOG	     0x02U
#define DNS_GETDB_PARTIAL    0x04U
#define DNS_GETDB_IGNOREACL  0x08U
#define DNS_GETDB_STALEFIRST 0X0CU

#define PENDINGOK(x) (((x)&DNS_DBFIND_PENDINGOK) != 0)

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
#define SAVE(a, b)                 \
	do {                       \
		INSIST(a == NULL); \
		a = b;             \
		b = NULL;          \
	} while (0)
#define RESTORE(a, b) SAVE(a, b)

static bool
validate(ns_client_t *client, dns_db_t *db, dns_name_t *name,
	 dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset);

static void
query_findclosestnsec3(dns_name_t *qname, dns_db_t *db,
		       dns_dbversion_t *version, ns_client_t *client,
		       dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset,
		       dns_name_t *fname, bool exact, dns_name_t *found);

static void
log_queryerror(ns_client_t *client, isc_result_t result, int line, int level);

static void
rpz_st_clear(ns_client_t *client);

static bool
rpz_ck_dnssec(ns_client_t *client, isc_result_t qresult,
	      dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset);

static void
log_noexistnodata(void *val, int level, const char *fmt, ...)
	ISC_FORMAT_PRINTF(3, 4);

/*
 * Return the hooktable in use with 'qctx', or if there isn't one
 * set, return the default hooktable.
 */
static ns_hooktable_t *
get_hooktab(query_ctx_t *qctx) {
	if (qctx == NULL || qctx->view == NULL || qctx->view->hooktable == NULL)
	{
		return (ns__hook_table);
	}

	return (qctx->view->hooktable);
}

/*
 * Call the specified hook function in every configured module that implements
 * that function. If any hook function returns NS_HOOK_RETURN, we
 * set 'result' and terminate processing by jumping to the 'cleanup' tag.
 *
 * (Note that a hook function may set the 'result' to ISC_R_SUCCESS but
 * still terminate processing within the calling function. That's why this
 * is a macro instead of a static function; it needs to be able to use
 * 'goto cleanup' regardless of the return value.)
 */
#define CALL_HOOK(_id, _qctx)                                       \
	do {                                                        \
		isc_result_t _res;                                  \
		ns_hooktable_t *_tab = get_hooktab(_qctx);          \
		ns_hook_t *_hook;                                   \
		_hook = ISC_LIST_HEAD((*_tab)[_id]);                \
		while (_hook != NULL) {                             \
			ns_hook_action_t _func = _hook->action;     \
			void *_data = _hook->action_data;           \
			INSIST(_func != NULL);                      \
			switch (_func(_qctx, _data, &_res)) {       \
			case NS_HOOK_CONTINUE:                      \
				_hook = ISC_LIST_NEXT(_hook, link); \
				break;                              \
			case NS_HOOK_RETURN:                        \
				result = _res;                      \
				goto cleanup;                       \
			default:                                    \
				UNREACHABLE();                      \
			}                                           \
		}                                                   \
	} while (false)

/*
 * Call the specified hook function in every configured module that
 * implements that function. All modules are called; hook function return
 * codes are ignored. This is intended for use with initialization and
 * destruction calls which *must* run in every configured module.
 *
 * (This could be implemented as a static void function, but is left as a
 * macro for symmetry with CALL_HOOK above.)
 */
#define CALL_HOOK_NORETURN(_id, _qctx)                          \
	do {                                                    \
		isc_result_t _res;                              \
		ns_hooktable_t *_tab = get_hooktab(_qctx);      \
		ns_hook_t *_hook;                               \
		_hook = ISC_LIST_HEAD((*_tab)[_id]);            \
		while (_hook != NULL) {                         \
			ns_hook_action_t _func = _hook->action; \
			void *_data = _hook->action_data;       \
			INSIST(_func != NULL);                  \
			_func(_qctx, _data, &_res);             \
			_hook = ISC_LIST_NEXT(_hook, link);     \
		}                                               \
	} while (false)

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
 * 5. If recursion is allowed, begin recursion (ns_query_recurse()).
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
 * 9. Handle a delegation response (query_delegation()). If we need
 *    to and are allowed to recurse (query_delegation_recurse()), go to 5,
 *    otherwise go to 15 to clean up and return the delegation to the client.
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
 * DNS64, RPZ, RRL, and the SERVFAIL cache. It also doesn't discuss
 * plugins.)
 */

static void
query_trace(query_ctx_t *qctx);

static void
qctx_init(ns_client_t *client, dns_fetchevent_t **eventp, dns_rdatatype_t qtype,
	  query_ctx_t *qctx);

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

static isc_result_t
query_delegation_recurse(query_ctx_t *qctx);

static void
query_addds(query_ctx_t *qctx);

static isc_result_t
query_nodata(query_ctx_t *qctx, isc_result_t result);

static isc_result_t
query_sign_nodata(query_ctx_t *qctx);

static void
query_addnxrrsetnsec(query_ctx_t *qctx);

static isc_result_t
query_nxdomain(query_ctx_t *qctx, bool empty_wild);

static isc_result_t
query_redirect(query_ctx_t *qctx);

static isc_result_t
query_ncache(query_ctx_t *qctx, isc_result_t result);

static isc_result_t
query_coveringnsec(query_ctx_t *qctx);

static isc_result_t
query_zerottl_refetch(query_ctx_t *qctx);

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
query_addwildcardproof(query_ctx_t *qctx, bool ispositive, bool nodata);

static void
query_addauth(query_ctx_t *qctx);

static void
query_clear_stale(ns_client_t *client);

/*
 * Increment query statistics counters.
 */
static void
inc_stats(ns_client_t *client, isc_statscounter_t counter) {
	dns_zone_t *zone = client->query.authzone;
	dns_rdatatype_t qtype;
	dns_rdataset_t *rdataset;
	isc_stats_t *zonestats;
	dns_stats_t *querystats = NULL;

	ns_stats_increment(client->sctx->nsstats, counter);

	if (zone == NULL) {
		return;
	}

	/* Do regular response type stats */
	zonestats = dns_zone_getrequeststats(zone);

	if (zonestats != NULL) {
		isc_stats_increment(zonestats, counter);
	}

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

	if ((client->message->flags & DNS_MESSAGEFLAG_AA) == 0) {
		inc_stats(client, ns_statscounter_nonauthans);
	} else {
		inc_stats(client, ns_statscounter_authans);
	}

	if (client->message->rcode == dns_rcode_noerror) {
		dns_section_t answer = DNS_SECTION_ANSWER;
		if (ISC_LIST_EMPTY(client->message->sections[answer])) {
			if (client->query.isreferral) {
				counter = ns_statscounter_referral;
			} else {
				counter = ns_statscounter_nxrrset;
			}
		} else {
			counter = ns_statscounter_success;
		}
	} else if (client->message->rcode == dns_rcode_nxdomain) {
		counter = ns_statscounter_nxdomain;
	} else if (client->message->rcode == dns_rcode_badcookie) {
		counter = ns_statscounter_badcookie;
	} else { /* We end up here in case of YXDOMAIN, and maybe others */
		counter = ns_statscounter_failure;
	}

	inc_stats(client, counter);
	ns_client_send(client);

	if (!client->nodetach) {
		isc_nmhandle_detach(&client->reqhandle);
	}
}

static void
query_error(ns_client_t *client, isc_result_t result, int line) {
	int loglevel = ISC_LOG_DEBUG(3);

	switch (dns_result_torcode(result)) {
	case dns_rcode_servfail:
		loglevel = ISC_LOG_DEBUG(1);
		inc_stats(client, ns_statscounter_servfail);
		break;
	case dns_rcode_formerr:
		inc_stats(client, ns_statscounter_formerr);
		break;
	default:
		inc_stats(client, ns_statscounter_failure);
		break;
	}

	if ((client->sctx->options & NS_SERVER_LOGQUERIES) != 0) {
		loglevel = ISC_LOG_INFO;
	}

	log_queryerror(client, result, line, loglevel);

	ns_client_error(client, result);

	if (!client->nodetach) {
		isc_nmhandle_detach(&client->reqhandle);
	}
}

static void
query_next(ns_client_t *client, isc_result_t result) {
	if (result == DNS_R_DUPLICATE) {
		inc_stats(client, ns_statscounter_duplicate);
	} else if (result == DNS_R_DROP) {
		inc_stats(client, ns_statscounter_dropped);
	} else {
		inc_stats(client, ns_statscounter_failure);
	}
	ns_client_drop(client, result);

	if (!client->nodetach) {
		isc_nmhandle_detach(&client->reqhandle);
	}
}

static void
query_freefreeversions(ns_client_t *client, bool everything) {
	ns_dbversion_t *dbversion, *dbversion_next;
	unsigned int i;

	for (dbversion = ISC_LIST_HEAD(client->query.freeversions), i = 0;
	     dbversion != NULL; dbversion = dbversion_next, i++)
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

static void
query_reset(ns_client_t *client, bool everything) {
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
	     dbversion != NULL; dbversion = dbversion_next)
	{
		dbversion_next = ISC_LIST_NEXT(dbversion, link);
		dns_db_closeversion(dbversion->db, &dbversion->version, false);
		dns_db_detach(&dbversion->db);
		ISC_LIST_INITANDAPPEND(client->query.freeversions, dbversion,
				       link);
	}
	ISC_LIST_INIT(client->query.activeversions);

	if (client->query.authdb != NULL) {
		dns_db_detach(&client->query.authdb);
	}
	if (client->query.authzone != NULL) {
		dns_zone_detach(&client->query.authzone);
	}

	if (client->query.dns64_aaaa != NULL) {
		ns_client_putrdataset(client, &client->query.dns64_aaaa);
	}
	if (client->query.dns64_sigaaaa != NULL) {
		ns_client_putrdataset(client, &client->query.dns64_sigaaaa);
	}
	if (client->query.dns64_aaaaok != NULL) {
		isc_mem_put(client->mctx, client->query.dns64_aaaaok,
			    client->query.dns64_aaaaoklen * sizeof(bool));
		client->query.dns64_aaaaok = NULL;
		client->query.dns64_aaaaoklen = 0;
	}

	ns_client_putrdataset(client, &client->query.redirect.rdataset);
	ns_client_putrdataset(client, &client->query.redirect.sigrdataset);
	if (client->query.redirect.db != NULL) {
		if (client->query.redirect.node != NULL) {
			dns_db_detachnode(client->query.redirect.db,
					  &client->query.redirect.node);
		}
		dns_db_detach(&client->query.redirect.db);
	}
	if (client->query.redirect.zone != NULL) {
		dns_zone_detach(&client->query.redirect.zone);
	}

	query_freefreeversions(client, everything);

	for (dbuf = ISC_LIST_HEAD(client->query.namebufs); dbuf != NULL;
	     dbuf = dbuf_next)
	{
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
		dns_message_puttempname(client->message, &client->query.qname);
	}
	client->query.qname = NULL;
	client->query.attributes = (NS_QUERYATTR_RECURSIONOK |
				    NS_QUERYATTR_CACHEOK | NS_QUERYATTR_SECURE);
	client->query.restarts = 0;
	client->query.timerset = false;
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
	client->query.authdbset = false;
	client->query.isreferral = false;
	client->query.dns64_options = 0;
	client->query.dns64_ttl = UINT32_MAX;
	recparam_update(&client->query.recparam, 0, NULL, NULL);
	client->query.root_key_sentinel_keyid = 0;
	client->query.root_key_sentinel_is_ta = false;
	client->query.root_key_sentinel_not_ta = false;
}

static void
query_cleanup(ns_client_t *client) {
	query_reset(client, false);
}

void
ns_query_free(ns_client_t *client) {
	REQUIRE(NS_CLIENT_VALID(client));

	query_reset(client, true);
}

isc_result_t
ns_query_init(ns_client_t *client) {
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(NS_CLIENT_VALID(client));

	ISC_LIST_INIT(client->query.namebufs);
	ISC_LIST_INIT(client->query.activeversions);
	ISC_LIST_INIT(client->query.freeversions);
	client->query.restarts = 0;
	client->query.timerset = false;
	client->query.rpz_st = NULL;
	client->query.qname = NULL;
	/*
	 * This mutex is destroyed when the client is destroyed in
	 * exit_check().
	 */
	isc_mutex_init(&client->query.fetchlock);

	client->query.fetch = NULL;
	client->query.prefetch = NULL;
	client->query.authdb = NULL;
	client->query.authzone = NULL;
	client->query.authdbset = false;
	client->query.isreferral = false;
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
	client->query.redirect.authoritative = false;
	client->query.redirect.is_zone = false;
	client->query.redirect.fname =
		dns_fixedname_initname(&client->query.redirect.fixed);
	query_reset(client, false);
	ns_client_newdbversion(client, 3);
	ns_client_newnamebuf(client);

	return (result);
}

/*%
 * Check if 'client' is allowed to query the cache of its associated view.
 * Unless 'options' has DNS_GETDB_NOLOG set, log the result of cache ACL
 * evaluation using the appropriate level, along with 'name' and 'qtype'.
 *
 * The cache ACL is only evaluated once for each client and then the result is
 * cached: if NS_QUERYATTR_CACHEACLOKVALID is set in client->query.attributes,
 * cache ACL evaluation has already been performed.  The evaluation result is
 * also stored in client->query.attributes: if NS_QUERYATTR_CACHEACLOK is set,
 * the client is allowed cache access.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS	'client' is allowed to access cache
 *\li	#DNS_R_REFUSED	'client' is not allowed to access cache
 */
static isc_result_t
query_checkcacheaccess(ns_client_t *client, const dns_name_t *name,
		       dns_rdatatype_t qtype, unsigned int options) {
	isc_result_t result;

	if ((client->query.attributes & NS_QUERYATTR_CACHEACLOKVALID) == 0) {
		/*
		 * The view's cache ACLs have not yet been evaluated.
		 * Do it now. Both allow-query-cache and
		 * allow-query-cache-on must be satsified.
		 */
		bool log = ((options & DNS_GETDB_NOLOG) == 0);
		char msg[NS_CLIENT_ACLMSGSIZE("query (cache)")];

		result = ns_client_checkaclsilent(client, NULL,
						  client->view->cacheacl, true);
		if (result == ISC_R_SUCCESS) {
			result = ns_client_checkaclsilent(
				client, &client->destaddr,
				client->view->cacheonacl, true);
		}
		if (result == ISC_R_SUCCESS) {
			/*
			 * We were allowed by the "allow-query-cache" ACL.
			 */
			client->query.attributes |= NS_QUERYATTR_CACHEACLOK;
			if (log && isc_log_wouldlog(ns_lctx, ISC_LOG_DEBUG(3)))
			{
				ns_client_aclmsg("query (cache)", name, qtype,
						 client->view->rdclass, msg,
						 sizeof(msg));
				ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_DEBUG(3), "%s approved",
					      msg);
			}
		} else if (log) {
			pfilter_notify(result, client, "checkcacheaccess");

			/*
			 * We were denied by the "allow-query-cache" ACL.
			 * There is no need to clear NS_QUERYATTR_CACHEACLOK
			 * since it is cleared by query_reset(), before query
			 * processing starts.
			 */
			ns_client_aclmsg("query (cache)", name, qtype,
					 client->view->rdclass, msg,
					 sizeof(msg));
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
				      "%s denied", msg);
		}

		/*
		 * Evaluation has been finished; make sure we will just consult
		 * NS_QUERYATTR_CACHEACLOK for this client from now on.
		 */
		client->query.attributes |= NS_QUERYATTR_CACHEACLOKVALID;
	}

	return ((client->query.attributes & NS_QUERYATTR_CACHEACLOK) != 0
			? ISC_R_SUCCESS
			: DNS_R_REFUSED);
}

static isc_result_t
query_validatezonedb(ns_client_t *client, const dns_name_t *name,
		     dns_rdatatype_t qtype, unsigned int options,
		     dns_zone_t *zone, dns_db_t *db,
		     dns_dbversion_t **versionp) {
	isc_result_t result;
	dns_acl_t *queryacl, *queryonacl;
	ns_dbversion_t *dbversion;

	REQUIRE(zone != NULL);
	REQUIRE(db != NULL);

	/*
	 * Mirror zone data is treated as cache data.
	 */
	if (dns_zone_gettype(zone) == dns_zone_mirror) {
		return (query_checkcacheaccess(client, name, qtype, options));
	}

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
	    !RECURSIONOK(client))
	{
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
	dbversion = ns_client_findversion(client, db);
	if (dbversion == NULL) {
		CTRACE(ISC_LOG_ERROR, "unable to get db version");
		return (DNS_R_SERVFAIL);
	}

	if ((options & DNS_GETDB_IGNOREACL) != 0) {
		goto approved;
	}
	if (dbversion->acl_checked) {
		if (!dbversion->queryok) {
			return (DNS_R_REFUSED);
		}
		goto approved;
	}

	queryacl = dns_zone_getqueryacl(zone);
	if (queryacl == NULL) {
		queryacl = client->view->queryacl;
		if ((client->query.attributes & NS_QUERYATTR_QUERYOKVALID) != 0)
		{
			/*
			 * We've evaluated the view's queryacl already.  If
			 * NS_QUERYATTR_QUERYOK is set, then the client is
			 * allowed to make queries, otherwise the query should
			 * be refused.
			 */
			dbversion->acl_checked = true;
			if ((client->query.attributes & NS_QUERYATTR_QUERYOK) ==
			    0)
			{
				dbversion->queryok = false;
				return (DNS_R_REFUSED);
			}
			dbversion->queryok = true;
			goto approved;
		}
	}

	result = ns_client_checkaclsilent(client, NULL, queryacl, true);
	if ((options & DNS_GETDB_NOLOG) == 0) {
		char msg[NS_CLIENT_ACLMSGSIZE("query")];
		if (result == ISC_R_SUCCESS) {
			if (isc_log_wouldlog(ns_lctx, ISC_LOG_DEBUG(3))) {
				ns_client_aclmsg("query", name, qtype,
						 client->view->rdclass, msg,
						 sizeof(msg));
				ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_DEBUG(3), "%s approved",
					      msg);
			}
		} else {
			pfilter_notify(result, client, "validatezonedb");
			ns_client_aclmsg("query", name, qtype,
					 client->view->rdclass, msg,
					 sizeof(msg));
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
		if (queryonacl == NULL) {
			queryonacl = client->view->queryonacl;
		}

		result = ns_client_checkaclsilent(client, &client->destaddr,
						  queryonacl, true);
		if ((options & DNS_GETDB_NOLOG) == 0 && result != ISC_R_SUCCESS)
		{
			ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
				      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
				      "query-on denied");
		}
	}

	dbversion->acl_checked = true;
	if (result != ISC_R_SUCCESS) {
		dbversion->queryok = false;
		return (DNS_R_REFUSED);
	}
	dbversion->queryok = true;

approved:
	/* Transfer ownership, if necessary. */
	if (versionp != NULL) {
		*versionp = dbversion->version;
	}
	return (ISC_R_SUCCESS);
}

static isc_result_t
query_getzonedb(ns_client_t *client, const dns_name_t *name,
		dns_rdatatype_t qtype, unsigned int options, dns_zone_t **zonep,
		dns_db_t **dbp, dns_dbversion_t **versionp) {
	isc_result_t result;
	unsigned int ztoptions;
	dns_zone_t *zone = NULL;
	dns_db_t *db = NULL;
	bool partial = false;

	REQUIRE(zonep != NULL && *zonep == NULL);
	REQUIRE(dbp != NULL && *dbp == NULL);

	/*%
	 * Find a zone database to answer the query.
	 */
	ztoptions = DNS_ZTFIND_MIRROR;
	if ((options & DNS_GETDB_NOEXACT) != 0) {
		ztoptions |= DNS_ZTFIND_NOEXACT;
	}

	result = dns_zt_find(client->view->zonetable, name, ztoptions, NULL,
			     &zone);

	if (result == DNS_R_PARTIALMATCH) {
		partial = true;
	}
	if (result == ISC_R_SUCCESS || result == DNS_R_PARTIALMATCH) {
		result = dns_zone_getdb(zone, &db);
	}

	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	result = query_validatezonedb(client, name, qtype, options, zone, db,
				      versionp);

	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	/* Transfer ownership. */
	*zonep = zone;
	*dbp = db;

	if (partial && (options & DNS_GETDB_PARTIAL) != 0) {
		return (DNS_R_PARTIALMATCH);
	}
	return (ISC_R_SUCCESS);

fail:
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}
	if (db != NULL) {
		dns_db_detach(&db);
	}

	return (result);
}

static void
rpz_log_rewrite(ns_client_t *client, bool disabled, dns_rpz_policy_t policy,
		dns_rpz_type_t type, dns_zone_t *p_zone, dns_name_t *p_name,
		dns_name_t *cname, dns_rpz_num_t rpz_num) {
	char cname_buf[DNS_NAME_FORMATSIZE] = { 0 };
	char p_name_buf[DNS_NAME_FORMATSIZE];
	char qname_buf[DNS_NAME_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	const char *s1 = cname_buf, *s2 = cname_buf;
	dns_rdataset_t *rdataset;
	dns_rpz_st_t *st;
	isc_stats_t *zonestats;

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
		if (zonestats != NULL) {
			isc_stats_increment(zonestats,
					    ns_statscounter_rpz_rewrites);
		}
	}

	if (!isc_log_wouldlog(ns_lctx, DNS_RPZ_INFO_LEVEL)) {
		return;
	}

	st = client->query.rpz_st;
	if ((st->popt.no_log & DNS_RPZ_ZBIT(rpz_num)) != 0) {
		return;
	}

	dns_name_format(client->query.qname, qname_buf, sizeof(qname_buf));
	dns_name_format(p_name, p_name_buf, sizeof(p_name_buf));
	if (cname != NULL) {
		s1 = " (CNAME to: ";
		dns_name_format(cname, cname_buf, sizeof(cname_buf));
		s2 = ")";
	}

	/*
	 *  Log Qclass and Qtype in addition to existing
	 *  fields.
	 */
	rdataset = ISC_LIST_HEAD(client->query.origqname->list);
	INSIST(rdataset != NULL);
	dns_rdataclass_format(rdataset->rdclass, classbuf, sizeof(classbuf));
	dns_rdatatype_format(rdataset->type, typebuf, sizeof(typebuf));

	ns_client_log(client, DNS_LOGCATEGORY_RPZ, NS_LOGMODULE_QUERY,
		      DNS_RPZ_INFO_LEVEL,
		      "%srpz %s %s rewrite %s/%s/%s via %s%s%s%s",
		      disabled ? "disabled " : "", dns_rpz_type2str(type),
		      dns_rpz_policy2str(policy), qname_buf, typebuf, classbuf,
		      p_name_buf, s1, cname_buf, s2);
}

static void
rpz_log_fail_helper(ns_client_t *client, int level, dns_name_t *p_name,
		    dns_rpz_type_t rpz_type1, dns_rpz_type_t rpz_type2,
		    const char *str, isc_result_t result) {
	char qnamebuf[DNS_NAME_FORMATSIZE];
	char p_namebuf[DNS_NAME_FORMATSIZE];
	const char *failed, *via, *slash, *str_blank;
	const char *rpztypestr1;
	const char *rpztypestr2;

	if (!isc_log_wouldlog(ns_lctx, level)) {
		return;
	}

	/*
	 * bin/tests/system/rpz/tests.sh looks for "rpz.*failed" for problems.
	 */
	if (level <= DNS_RPZ_DEBUG_LEVEL1) {
		failed = " failed: ";
	} else {
		failed = ": ";
	}

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

	ns_client_log(client, NS_LOGCATEGORY_QUERY_ERRORS, NS_LOGMODULE_QUERY,
		      level, "rpz %s%s%s rewrite %s%s%s%s%s%s%s", rpztypestr1,
		      slash, rpztypestr2, qnamebuf, via, p_namebuf, str_blank,
		      str, failed, isc_result_totext(result));
}

static void
rpz_log_fail(ns_client_t *client, int level, dns_name_t *p_name,
	     dns_rpz_type_t rpz_type, const char *str, isc_result_t result) {
	rpz_log_fail_helper(client, level, p_name, rpz_type, DNS_RPZ_TYPE_BAD,
			    str, result);
}

/*
 * Get a policy rewrite zone database.
 */
static isc_result_t
rpz_getdb(ns_client_t *client, dns_name_t *p_name, dns_rpz_type_t rpz_type,
	  dns_zone_t **zonep, dns_db_t **dbp, dns_dbversion_t **versionp) {
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
				      dns_rpz_type2str(rpz_type), qnamebuf,
				      p_namebuf);
		}
		*versionp = rpz_version;
		return (ISC_R_SUCCESS);
	}
	rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, p_name, rpz_type,
		     "query_getzonedb()", result);
	return (result);
}

/*%
 * Find a cache database to answer the query.  This may fail with DNS_R_REFUSED
 * if the client is not allowed to use the cache.
 */
static isc_result_t
query_getcachedb(ns_client_t *client, const dns_name_t *name,
		 dns_rdatatype_t qtype, dns_db_t **dbp, unsigned int options) {
	isc_result_t result;
	dns_db_t *db = NULL;

	REQUIRE(dbp != NULL && *dbp == NULL);

	if (!USECACHE(client)) {
		return (DNS_R_REFUSED);
	}

	dns_db_attach(client->view->cachedb, &db);

	result = query_checkcacheaccess(client, name, qtype, options);
	if (result != ISC_R_SUCCESS) {
		dns_db_detach(&db);
	}

	/*
	 * If query_checkcacheaccess() succeeded, transfer ownership of 'db'.
	 * Otherwise, 'db' will be NULL due to the dns_db_detach() call above.
	 */
	*dbp = db;

	return (result);
}

static isc_result_t
query_getdb(ns_client_t *client, dns_name_t *name, dns_rdatatype_t qtype,
	    unsigned int options, dns_zone_t **zonep, dns_db_t **dbp,
	    dns_dbversion_t **versionp, bool *is_zonep) {
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
	result = query_getzonedb(client, name, qtype, options, &zone, dbp,
				 versionp);

	/* See how many labels are in the zone's name.	  */
	if (result == ISC_R_SUCCESS && zone != NULL) {
		zonelabels = dns_name_countlabels(dns_zone_getorigin(zone));
	}

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
		dns_clientinfo_init(&ci, client, &client->ecs, NULL);

		tdbp = NULL;
		tresult = dns_view_searchdlz(client->view, name, zonelabels,
					     &cm, &ci, &tdbp);
		/* If we successful, we found a better match. */
		if (tresult == ISC_R_SUCCESS) {
			ns_dbversion_t *dbversion;

			/*
			 * If the previous search returned a zone, detach it.
			 */
			if (zone != NULL) {
				dns_zone_detach(&zone);
			}

			/*
			 * If the previous search returned a database,
			 * detach it.
			 */
			if (*dbp != NULL) {
				dns_db_detach(dbp);
			}

			/*
			 * If the previous search returned a version, clear it.
			 */
			*versionp = NULL;

			dbversion = ns_client_findversion(client, tdbp);
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
		*is_zonep = true;
	} else {
		if (result == ISC_R_NOTFOUND) {
			result = query_getcachedb(client, name, qtype, dbp,
						  options);
		}
		*is_zonep = false;
	}
	return (result);
}

static bool
query_isduplicate(ns_client_t *client, dns_name_t *name, dns_rdatatype_t type,
		  dns_name_t **mnamep) {
	dns_section_t section;
	dns_name_t *mname = NULL;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "query_isduplicate");

	for (section = DNS_SECTION_ANSWER; section <= DNS_SECTION_ADDITIONAL;
	     section++)
	{
		result = dns_message_findname(client->message, section, name,
					      type, 0, &mname, NULL);
		if (result == ISC_R_SUCCESS) {
			/*
			 * We've already got this RRset in the response.
			 */
			CTRACE(ISC_LOG_DEBUG(3), "query_isduplicate: true: "
						 "done");
			return (true);
		} else if (result == DNS_R_NXRRSET) {
			/*
			 * The name exists, but the rdataset does not.
			 */
			if (section == DNS_SECTION_ADDITIONAL) {
				break;
			}
		} else {
			RUNTIME_CHECK(result == DNS_R_NXDOMAIN);
		}
		mname = NULL;
	}

	if (mnamep != NULL) {
		*mnamep = mname;
	}

	CTRACE(ISC_LOG_DEBUG(3), "query_isduplicate: false: done");
	return (false);
}

/*
 * Look up data for given 'name' and 'type' in given 'version' of 'db' for
 * 'client'. Called from query_additionalauth().
 *
 * If the lookup is successful:
 *
 *   - store the node containing the result at 'nodep',
 *
 *   - store the owner name of the returned node in 'fname',
 *
 *   - if 'type' is not ANY, dns_db_findext() will put the exact rdataset being
 *     looked for in 'rdataset' and its signatures (if any) in 'sigrdataset',
 *
 *   - if 'type' is ANY, dns_db_findext() will leave 'rdataset' and
 *     'sigrdataset' disassociated and the returned node will be iterated in
 *     query_additional_cb().
 *
 * If the lookup is not successful:
 *
 *   - 'nodep' will not be written to,
 *   - 'fname' may still be modified as it is passed to dns_db_findext(),
 *   - 'rdataset' and 'sigrdataset' will remain disassociated.
 */
static isc_result_t
query_additionalauthfind(dns_db_t *db, dns_dbversion_t *version,
			 const dns_name_t *name, dns_rdatatype_t type,
			 ns_client_t *client, dns_dbnode_t **nodep,
			 dns_name_t *fname, dns_rdataset_t *rdataset,
			 dns_rdataset_t *sigrdataset) {
	dns_clientinfomethods_t cm;
	dns_dbnode_t *node = NULL;
	dns_clientinfo_t ci;
	isc_result_t result;

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

	/*
	 * Since we are looking for authoritative data, we do not set
	 * the GLUEOK flag.  Glue will be looked for later, but not
	 * necessarily in the same database.
	 */
	result = dns_db_findext(db, name, version, type,
				client->query.dboptions, client->now, &node,
				fname, &cm, &ci, rdataset, sigrdataset);
	if (result != ISC_R_SUCCESS) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}

		if (sigrdataset != NULL &&
		    dns_rdataset_isassociated(sigrdataset))
		{
			dns_rdataset_disassociate(sigrdataset);
		}

		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}

		return (result);
	}

	/*
	 * Do not return signatures if the zone is not fully signed.
	 */
	if (sigrdataset != NULL && !dns_db_issecure(db) &&
	    dns_rdataset_isassociated(sigrdataset))
	{
		dns_rdataset_disassociate(sigrdataset);
	}

	*nodep = node;

	return (ISC_R_SUCCESS);
}

/*
 * For query context 'qctx', try finding authoritative additional data for
 * given 'name' and 'type'. Called from query_additional_cb().
 *
 * If successful:
 *
 *   - store pointers to the database and node which contain the result in
 *     'dbp' and 'nodep', respectively,
 *
 *   - store the owner name of the returned node in 'fname',
 *
 *   - potentially bind 'rdataset' and 'sigrdataset', as explained in the
 *     comment for query_additionalauthfind().
 *
 * If unsuccessful:
 *
 *   - 'dbp' and 'nodep' will not be written to,
 *   - 'fname' may still be modified as it is passed to dns_db_findext(),
 *   - 'rdataset' and 'sigrdataset' will remain disassociated.
 */
static isc_result_t
query_additionalauth(query_ctx_t *qctx, const dns_name_t *name,
		     dns_rdatatype_t type, dns_db_t **dbp, dns_dbnode_t **nodep,
		     dns_name_t *fname, dns_rdataset_t *rdataset,
		     dns_rdataset_t *sigrdataset) {
	ns_client_t *client = qctx->client;
	ns_dbversion_t *dbversion = NULL;
	dns_dbversion_t *version = NULL;
	dns_dbnode_t *node = NULL;
	dns_zone_t *zone = NULL;
	dns_db_t *db = NULL;
	isc_result_t result;

	/*
	 * First, look within the same zone database for authoritative
	 * additional data.
	 */
	if (!client->query.authdbset || client->query.authdb == NULL) {
		return (ISC_R_NOTFOUND);
	}

	dbversion = ns_client_findversion(client, client->query.authdb);
	if (dbversion == NULL) {
		return (ISC_R_NOTFOUND);
	}

	dns_db_attach(client->query.authdb, &db);
	version = dbversion->version;

	CTRACE(ISC_LOG_DEBUG(3), "query_additionalauth: same zone");

	result = query_additionalauthfind(db, version, name, type, client,
					  &node, fname, rdataset, sigrdataset);
	if (result != ISC_R_SUCCESS &&
	    qctx->view->minimalresponses == dns_minimal_no &&
	    RECURSIONOK(client))
	{
		/*
		 * If we aren't doing response minimization and recursion is
		 * allowed, we can try and see if any other zone matches.
		 */
		version = NULL;
		dns_db_detach(&db);
		result = query_getzonedb(client, name, type, DNS_GETDB_NOLOG,
					 &zone, &db, &version);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		dns_zone_detach(&zone);

		CTRACE(ISC_LOG_DEBUG(3), "query_additionalauth: other zone");

		result = query_additionalauthfind(db, version, name, type,
						  client, &node, fname,
						  rdataset, sigrdataset);
	}

	if (result != ISC_R_SUCCESS) {
		dns_db_detach(&db);
	} else {
		*nodep = node;
		node = NULL;

		*dbp = db;
		db = NULL;
	}

	return (result);
}

static isc_result_t
query_additional_cb(void *arg, const dns_name_t *name, dns_rdatatype_t qtype) {
	query_ctx_t *qctx = arg;
	ns_client_t *client = qctx->client;
	isc_result_t result, eresult = ISC_R_SUCCESS;
	dns_dbnode_t *node = NULL;
	dns_db_t *db = NULL;
	dns_name_t *fname = NULL, *mname = NULL;
	dns_rdataset_t *rdataset = NULL, *sigrdataset = NULL;
	dns_rdataset_t *trdataset = NULL;
	isc_buffer_t *dbuf = NULL;
	isc_buffer_t b;
	ns_dbversion_t *dbversion = NULL;
	dns_dbversion_t *version = NULL;
	bool added_something = false, need_addname = false;
	dns_rdatatype_t type;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	dns_rdatasetadditional_t additionaltype =
		dns_rdatasetadditional_fromauth;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(qtype != dns_rdatatype_any);

	if (!WANTDNSSEC(client) && dns_rdatatype_isdnssec(qtype)) {
		return (ISC_R_SUCCESS);
	}

	CTRACE(ISC_LOG_DEBUG(3), "query_additional_cb");

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

	/*
	 * We treat type A additional section processing as if it
	 * were "any address type" additional section processing.
	 * To avoid multiple lookups, we do an 'any' database
	 * lookup and iterate over the node.
	 */
	if (qtype == dns_rdatatype_a) {
		type = dns_rdatatype_any;
	} else {
		type = qtype;
	}

	/*
	 * Get some resources.
	 */
	dbuf = ns_client_getnamebuf(client);
	if (dbuf == NULL) {
		goto cleanup;
	}
	fname = ns_client_newname(client, dbuf, &b);
	rdataset = ns_client_newrdataset(client);
	if (fname == NULL || rdataset == NULL) {
		goto cleanup;
	}
	if (WANTDNSSEC(client)) {
		sigrdataset = ns_client_newrdataset(client);
		if (sigrdataset == NULL) {
			goto cleanup;
		}
	}

	/*
	 * If we want only minimal responses and are here, then it must
	 * be for glue.
	 */
	if (qctx->view->minimalresponses == dns_minimal_yes &&
	    client->query.qtype != dns_rdatatype_ns)
	{
		goto try_glue;
	}

	/*
	 * First, look for authoritative additional data.
	 */
	result = query_additionalauth(qctx, name, type, &db, &node, fname,
				      rdataset, sigrdataset);
	if (result == ISC_R_SUCCESS) {
		goto found;
	}

	/*
	 * No authoritative data was found.  The cache is our next best bet.
	 */
	if (!qctx->view->recursion) {
		goto try_glue;
	}

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
		sigrdataset = ns_client_newrdataset(client);
		if (sigrdataset == NULL) {
			goto cleanup;
		}
	}

	version = NULL;
	result = dns_db_findext(db, name, version, type,
				client->query.dboptions | DNS_DBFIND_GLUEOK |
					DNS_DBFIND_ADDITIONALOK,
				client->now, &node, fname, &cm, &ci, rdataset,
				sigrdataset);

	dns_cache_updatestats(qctx->view->cache, result);
	if (!WANTDNSSEC(client)) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (result == ISC_R_SUCCESS) {
		goto found;
	}

	if (dns_rdataset_isassociated(rdataset)) {
		dns_rdataset_disassociate(rdataset);
	}
	if (sigrdataset != NULL && dns_rdataset_isassociated(sigrdataset)) {
		dns_rdataset_disassociate(sigrdataset);
	}
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
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

	if (client->query.gluedb == NULL) {
		goto cleanup;
	}

	/*
	 * Don't poison caches using the bailiwick protection model.
	 */
	if (!dns_name_issubdomain(name, dns_db_origin(client->query.gluedb))) {
		goto cleanup;
	}

	dbversion = ns_client_findversion(client, client->query.gluedb);
	if (dbversion == NULL) {
		goto cleanup;
	}

	dns_db_attach(client->query.gluedb, &db);
	version = dbversion->version;
	additionaltype = dns_rdatasetadditional_fromglue;
	result = dns_db_findext(db, name, version, type,
				client->query.dboptions | DNS_DBFIND_GLUEOK,
				client->now, &node, fname, &cm, &ci, rdataset,
				sigrdataset);
	if (result != ISC_R_SUCCESS && result != DNS_R_ZONECUT &&
	    result != DNS_R_GLUE)
	{
		goto cleanup;
	}

found:
	/*
	 * We have found a potential additional data rdataset, or
	 * at least a node to iterate over.
	 */
	ns_client_keepname(client, fname, dbuf);

	/*
	 * If we have an rdataset, add it to the additional data
	 * section.
	 */
	mname = NULL;
	if (dns_rdataset_isassociated(rdataset) &&
	    !query_isduplicate(client, fname, type, &mname))
	{
		if (mname != NULL) {
			INSIST(mname != fname);
			ns_client_releasename(client, &fname);
			fname = mname;
		} else {
			need_addname = true;
		}
		ISC_LIST_APPEND(fname->list, rdataset, link);
		trdataset = rdataset;
		rdataset = NULL;
		added_something = true;
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
		/*
		 * We now go looking for A and AAAA records, along with
		 * their signatures.
		 *
		 * XXXRTH  This code could be more efficient.
		 */
		if (rdataset != NULL) {
			if (dns_rdataset_isassociated(rdataset)) {
				dns_rdataset_disassociate(rdataset);
			}
		} else {
			rdataset = ns_client_newrdataset(client);
			if (rdataset == NULL) {
				goto addname;
			}
		}
		if (sigrdataset != NULL) {
			if (dns_rdataset_isassociated(sigrdataset)) {
				dns_rdataset_disassociate(sigrdataset);
			}
		} else if (WANTDNSSEC(client)) {
			sigrdataset = ns_client_newrdataset(client);
			if (sigrdataset == NULL) {
				goto addname;
			}
		}
		if (query_isduplicate(client, fname, dns_rdatatype_a, NULL)) {
			goto aaaa_lookup;
		}
		result = dns_db_findrdataset(db, node, version, dns_rdatatype_a,
					     0, client->now, rdataset,
					     sigrdataset);
		if (result == DNS_R_NCACHENXDOMAIN) {
			goto addname;
		} else if (result == DNS_R_NCACHENXRRSET) {
			dns_rdataset_disassociate(rdataset);
			if (sigrdataset != NULL &&
			    dns_rdataset_isassociated(sigrdataset))
			{
				dns_rdataset_disassociate(sigrdataset);
			}
		} else if (result == ISC_R_SUCCESS) {
			bool invalid = false;
			mname = NULL;
			if (additionaltype ==
				    dns_rdatasetadditional_fromcache &&
			    (DNS_TRUST_PENDING(rdataset->trust) ||
			     DNS_TRUST_GLUE(rdataset->trust)))
			{
				/* validate() may change rdataset->trust */
				invalid = !validate(client, db, fname, rdataset,
						    sigrdataset);
			}
			if (invalid && DNS_TRUST_PENDING(rdataset->trust)) {
				dns_rdataset_disassociate(rdataset);
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
				{
					dns_rdataset_disassociate(sigrdataset);
				}
			} else if (!query_isduplicate(client, fname,
						      dns_rdatatype_a, &mname))
			{
				if (mname != fname) {
					if (mname != NULL) {
						ns_client_releasename(client,
								      &fname);
						fname = mname;
					} else {
						need_addname = true;
					}
				}
				ISC_LIST_APPEND(fname->list, rdataset, link);
				added_something = true;
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
				{
					ISC_LIST_APPEND(fname->list,
							sigrdataset, link);
					sigrdataset =
						ns_client_newrdataset(client);
				}
				rdataset = ns_client_newrdataset(client);
				if (rdataset == NULL) {
					goto addname;
				}
				if (WANTDNSSEC(client) && sigrdataset == NULL) {
					goto addname;
				}
			} else {
				dns_rdataset_disassociate(rdataset);
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
				{
					dns_rdataset_disassociate(sigrdataset);
				}
			}
		}
	aaaa_lookup:
		if (query_isduplicate(client, fname, dns_rdatatype_aaaa, NULL))
		{
			goto addname;
		}
		result = dns_db_findrdataset(db, node, version,
					     dns_rdatatype_aaaa, 0, client->now,
					     rdataset, sigrdataset);
		if (result == DNS_R_NCACHENXDOMAIN) {
			goto addname;
		} else if (result == DNS_R_NCACHENXRRSET) {
			dns_rdataset_disassociate(rdataset);
			if (sigrdataset != NULL &&
			    dns_rdataset_isassociated(sigrdataset))
			{
				dns_rdataset_disassociate(sigrdataset);
			}
		} else if (result == ISC_R_SUCCESS) {
			bool invalid = false;
			mname = NULL;

			if (additionaltype ==
				    dns_rdatasetadditional_fromcache &&
			    (DNS_TRUST_PENDING(rdataset->trust) ||
			     DNS_TRUST_GLUE(rdataset->trust)))
			{
				/* validate() may change rdataset->trust */
				invalid = !validate(client, db, fname, rdataset,
						    sigrdataset);
			}

			if (invalid && DNS_TRUST_PENDING(rdataset->trust)) {
				dns_rdataset_disassociate(rdataset);
				if (sigrdataset != NULL &&
				    dns_rdataset_isassociated(sigrdataset))
				{
					dns_rdataset_disassociate(sigrdataset);
				}
			} else if (!query_isduplicate(client, fname,
						      dns_rdatatype_aaaa,
						      &mname))
			{
				if (mname != fname) {
					if (mname != NULL) {
						ns_client_releasename(client,
								      &fname);
						fname = mname;
					} else {
						need_addname = true;
					}
				}
				ISC_LIST_APPEND(fname->list, rdataset, link);
				added_something = true;
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
	CTRACE(ISC_LOG_DEBUG(3), "query_additional_cb: addname");
	/*
	 * If we haven't added anything, then we're done.
	 */
	if (!added_something) {
		goto cleanup;
	}

	/*
	 * We may have added our rdatasets to an existing name, if so, then
	 * need_addname will be false.  Whether we used an existing name
	 * or a new one, we must set fname to NULL to prevent cleanup.
	 */
	if (need_addname) {
		dns_message_addname(client->message, fname,
				    DNS_SECTION_ADDITIONAL);
	}
	fname = NULL;

	/*
	 * In some cases, a record that has been added as additional
	 * data may *also* trigger the addition of additional data.
	 * This cannot go more than MAX_RESTARTS levels deep.
	 */
	if (trdataset != NULL && dns_rdatatype_followadditional(type)) {
		eresult = dns_rdataset_additionaldata(
			trdataset, query_additional_cb, qctx);
	}

cleanup:
	CTRACE(ISC_LOG_DEBUG(3), "query_additional_cb: cleanup");
	ns_client_putrdataset(client, &rdataset);
	if (sigrdataset != NULL) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (fname != NULL) {
		ns_client_releasename(client, &fname);
	}
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
	if (db != NULL) {
		dns_db_detach(&db);
	}

	CTRACE(ISC_LOG_DEBUG(3), "query_additional_cb: done");
	return (eresult);
}

/*
 * Add 'rdataset' to 'name'.
 */
static void
query_addtoname(dns_name_t *name, dns_rdataset_t *rdataset) {
	ISC_LIST_APPEND(name->list, rdataset, link);
}

/*
 * Set the ordering for 'rdataset'.
 */
static void
query_setorder(query_ctx_t *qctx, dns_name_t *name, dns_rdataset_t *rdataset) {
	ns_client_t *client = qctx->client;
	dns_order_t *order = client->view->order;

	CTRACE(ISC_LOG_DEBUG(3), "query_setorder");

	UNUSED(client);

	if (order != NULL) {
		rdataset->attributes |= dns_order_find(
			order, name, rdataset->type, rdataset->rdclass);
	}
	rdataset->attributes |= DNS_RDATASETATTR_LOADORDER;
}

/*
 * Handle glue and fetch any other needed additional data for 'rdataset'.
 */
static void
query_additional(query_ctx_t *qctx, dns_rdataset_t *rdataset) {
	ns_client_t *client = qctx->client;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "query_additional");

	if (NOADDITIONAL(client)) {
		return;
	}

	/*
	 * Try to process glue directly.
	 */
	if (qctx->view->use_glue_cache &&
	    (rdataset->type == dns_rdatatype_ns) &&
	    (client->query.gluedb != NULL) &&
	    dns_db_iszone(client->query.gluedb))
	{
		ns_dbversion_t *dbversion;

		dbversion = ns_client_findversion(client, client->query.gluedb);
		if (dbversion == NULL) {
			goto regular;
		}

		result = dns_rdataset_addglue(rdataset, dbversion->version,
					      client->message);
		if (result == ISC_R_SUCCESS) {
			return;
		}
	}

regular:
	/*
	 * Add other additional data if needed.
	 * We don't care if dns_rdataset_additionaldata() fails.
	 */
	(void)dns_rdataset_additionaldata(rdataset, query_additional_cb, qctx);
	CTRACE(ISC_LOG_DEBUG(3), "query_additional: done");
}

static void
query_addrrset(query_ctx_t *qctx, dns_name_t **namep,
	       dns_rdataset_t **rdatasetp, dns_rdataset_t **sigrdatasetp,
	       isc_buffer_t *dbuf, dns_section_t section) {
	isc_result_t result;
	ns_client_t *client = qctx->client;
	dns_name_t *name = *namep, *mname = NULL;
	dns_rdataset_t *rdataset = *rdatasetp, *mrdataset = NULL;
	dns_rdataset_t *sigrdataset = NULL;

	CTRACE(ISC_LOG_DEBUG(3), "query_addrrset");

	REQUIRE(name != NULL);

	if (sigrdatasetp != NULL) {
		sigrdataset = *sigrdatasetp;
	}

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
	result = dns_message_findname(client->message, section, name,
				      rdataset->type, rdataset->covers, &mname,
				      &mrdataset);
	if (result == ISC_R_SUCCESS) {
		/*
		 * We've already got an RRset of the given name and type.
		 */
		CTRACE(ISC_LOG_DEBUG(3), "query_addrrset: dns_message_findname "
					 "succeeded: done");
		if (dbuf != NULL) {
			ns_client_releasename(client, namep);
		}
		if ((rdataset->attributes & DNS_RDATASETATTR_REQUIRED) != 0) {
			mrdataset->attributes |= DNS_RDATASETATTR_REQUIRED;
		}
		if ((rdataset->attributes & DNS_RDATASETATTR_STALE_ADDED) != 0)
		{
			mrdataset->attributes |= DNS_RDATASETATTR_STALE_ADDED;
		}
		return;
	} else if (result == DNS_R_NXDOMAIN) {
		/*
		 * The name doesn't exist.
		 */
		if (dbuf != NULL) {
			ns_client_keepname(client, name, dbuf);
		}
		dns_message_addname(client->message, name, section);
		*namep = NULL;
		mname = name;
	} else {
		RUNTIME_CHECK(result == DNS_R_NXRRSET);
		if (dbuf != NULL) {
			ns_client_releasename(client, namep);
		}
	}

	if (rdataset->trust != dns_trust_secure &&
	    (section == DNS_SECTION_ANSWER || section == DNS_SECTION_AUTHORITY))
	{
		client->query.attributes &= ~NS_QUERYATTR_SECURE;
	}

	/*
	 * Update message name, set rdataset order, and do additional
	 * section processing if needed.
	 */
	query_addtoname(mname, rdataset);
	query_setorder(qctx, mname, rdataset);
	query_additional(qctx, rdataset);

	/*
	 * Note: we only add SIGs if we've added the type they cover, so
	 * we do not need to check if the SIG rdataset is already in the
	 * response.
	 */
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
	    dns_rdataset_t *sigrdataset) {
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	isc_stdtime_t now;

	rdataset->trust = dns_trust_secure;
	sigrdataset->trust = dns_trust_secure;
	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

	/*
	 * Save the updated secure state.  Ignore failures.
	 */
	result = dns_db_findnodeext(db, name, true, &cm, &ci, &node);
	if (result != ISC_R_SUCCESS) {
		return;
	}

	isc_stdtime_get(&now);
	dns_rdataset_trimttl(rdataset, sigrdataset, rrsig, now,
			     client->view->acceptexpired);

	(void)dns_db_addrdataset(db, node, NULL, client->now, rdataset, 0,
				 NULL);
	(void)dns_db_addrdataset(db, node, NULL, client->now, sigrdataset, 0,
				 NULL);
	dns_db_detachnode(db, &node);
}

/*
 * Find the secure key that corresponds to rrsig.
 * Note: 'keyrdataset' maintains state between successive calls,
 * there may be multiple keys with the same keyid.
 * Return false if we have exhausted all the possible keys.
 */
static bool
get_key(ns_client_t *client, dns_db_t *db, dns_rdata_rrsig_t *rrsig,
	dns_rdataset_t *keyrdataset, dst_key_t **keyp) {
	isc_result_t result;
	dns_dbnode_t *node = NULL;
	bool secure = false;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

	if (!dns_rdataset_isassociated(keyrdataset)) {
		result = dns_db_findnodeext(db, &rrsig->signer, false, &cm, &ci,
					    &node);
		if (result != ISC_R_SUCCESS) {
			return (false);
		}

		result = dns_db_findrdataset(db, node, NULL,
					     dns_rdatatype_dnskey, 0,
					     client->now, keyrdataset, NULL);
		dns_db_detachnode(db, &node);
		if (result != ISC_R_SUCCESS) {
			return (false);
		}

		if (keyrdataset->trust != dns_trust_secure) {
			return (false);
		}

		result = dns_rdataset_first(keyrdataset);
	} else {
		result = dns_rdataset_next(keyrdataset);
	}

	for (; result == ISC_R_SUCCESS; result = dns_rdataset_next(keyrdataset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		isc_buffer_t b;

		dns_rdataset_current(keyrdataset, &rdata);
		isc_buffer_init(&b, rdata.data, rdata.length);
		isc_buffer_add(&b, rdata.length);
		result = dst_key_fromdns(&rrsig->signer, rdata.rdclass, &b,
					 client->mctx, keyp);
		if (result != ISC_R_SUCCESS) {
			continue;
		}
		if (rrsig->algorithm == (dns_secalg_t)dst_key_alg(*keyp) &&
		    rrsig->keyid == (dns_keytag_t)dst_key_id(*keyp) &&
		    dst_key_iszonekey(*keyp))
		{
			secure = true;
			break;
		}
		dst_key_free(keyp);
	}
	return (secure);
}

static bool
verify(dst_key_t *key, dns_name_t *name, dns_rdataset_t *rdataset,
       dns_rdata_t *rdata, ns_client_t *client) {
	isc_result_t result;
	dns_fixedname_t fixed;
	bool ignore = false;

	dns_fixedname_init(&fixed);

again:
	result = dns_dnssec_verify(name, rdataset, key, ignore,
				   client->view->maxbits, client->mctx, rdata,
				   NULL);
	if (result == DNS_R_SIGEXPIRED && client->view->acceptexpired) {
		ignore = true;
		goto again;
	}
	if (result == ISC_R_SUCCESS || result == DNS_R_FROMWILDCARD) {
		return (true);
	}
	return (false);
}

/*
 * Validate the rdataset if possible with available records.
 */
static bool
validate(ns_client_t *client, dns_db_t *db, dns_name_t *name,
	 dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_rrsig_t rrsig;
	dst_key_t *key = NULL;
	dns_rdataset_t keyrdataset;

	if (sigrdataset == NULL || !dns_rdataset_isassociated(sigrdataset)) {
		return (false);
	}

	for (result = dns_rdataset_first(sigrdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(sigrdataset))
	{
		dns_rdata_reset(&rdata);
		dns_rdataset_current(sigrdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &rrsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (!dns_resolver_algorithm_supported(client->view->resolver,
						      name, rrsig.algorithm))
		{
			continue;
		}
		if (!dns_name_issubdomain(name, &rrsig.signer)) {
			continue;
		}
		dns_rdataset_init(&keyrdataset);
		do {
			if (!get_key(client, db, &rrsig, &keyrdataset, &key)) {
				break;
			}
			if (verify(key, name, rdataset, &rdata, client)) {
				dst_key_free(&key);
				dns_rdataset_disassociate(&keyrdataset);
				mark_secure(client, db, name, &rrsig, rdataset,
					    sigrdataset);
				return (true);
			}
			dst_key_free(&key);
		} while (1);
		if (dns_rdataset_isassociated(&keyrdataset)) {
			dns_rdataset_disassociate(&keyrdataset);
		}
	}
	return (false);
}

static void
fixrdataset(ns_client_t *client, dns_rdataset_t **rdataset) {
	if (*rdataset == NULL) {
		*rdataset = ns_client_newrdataset(client);
	} else if (dns_rdataset_isassociated(*rdataset)) {
		dns_rdataset_disassociate(*rdataset);
	}
}

static void
fixfname(ns_client_t *client, dns_name_t **fname, isc_buffer_t **dbuf,
	 isc_buffer_t *nbuf) {
	if (*fname == NULL) {
		*dbuf = ns_client_getnamebuf(client);
		if (*dbuf == NULL) {
			return;
		}
		*fname = ns_client_newname(client, *dbuf, nbuf);
	}
}

static void
free_devent(ns_client_t *client, isc_event_t **eventp,
	    dns_fetchevent_t **deventp) {
	dns_fetchevent_t *devent = *deventp;

	REQUIRE((void *)(*eventp) == (void *)(*deventp));

	CTRACE(ISC_LOG_DEBUG(3), "free_devent");

	if (devent->fetch != NULL) {
		dns_resolver_destroyfetch(&devent->fetch);
	}
	if (devent->node != NULL) {
		dns_db_detachnode(devent->db, &devent->node);
	}
	if (devent->db != NULL) {
		dns_db_detach(&devent->db);
	}
	if (devent->rdataset != NULL) {
		ns_client_putrdataset(client, &devent->rdataset);
	}
	if (devent->sigrdataset != NULL) {
		ns_client_putrdataset(client, &devent->sigrdataset);
	}

	/*
	 * If the two pointers are the same then leave the setting of
	 * (*deventp) to NULL to isc_event_free.
	 */
	if ((void *)eventp != (void *)deventp) {
		(*deventp) = NULL;
	}
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

	CTRACE(ISC_LOG_DEBUG(3), "prefetch_done");

	LOCK(&client->query.fetchlock);
	if (client->query.prefetch != NULL) {
		INSIST(devent->fetch == client->query.prefetch);
		client->query.prefetch = NULL;
	}
	UNLOCK(&client->query.fetchlock);

	/*
	 * We're done prefetching, detach from quota.
	 */
	if (client->recursionquota != NULL) {
		isc_quota_detach(&client->recursionquota);
		ns_stats_decrement(client->sctx->nsstats,
				   ns_statscounter_recursclients);
	}

	free_devent(client, &event, &devent);
	isc_nmhandle_detach(&client->prefetchhandle);
}

static void
query_prefetch(ns_client_t *client, dns_name_t *qname,
	       dns_rdataset_t *rdataset) {
	isc_result_t result;
	isc_sockaddr_t *peeraddr;
	dns_rdataset_t *tmprdataset;
	unsigned int options;

	CTRACE(ISC_LOG_DEBUG(3), "query_prefetch");

	if (client->query.prefetch != NULL ||
	    client->view->prefetch_trigger == 0U ||
	    rdataset->ttl > client->view->prefetch_trigger ||
	    (rdataset->attributes & DNS_RDATASETATTR_PREFETCH) == 0)
	{
		return;
	}

	if (client->recursionquota == NULL) {
		result = isc_quota_attach(&client->sctx->recursionquota,
					  &client->recursionquota);
		switch (result) {
		case ISC_R_SUCCESS:
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_recursclients);
			break;
		case ISC_R_SOFTQUOTA:
			isc_quota_detach(&client->recursionquota);
			FALLTHROUGH;
		default:
			return;
		}
	}

	tmprdataset = ns_client_newrdataset(client);
	if (tmprdataset == NULL) {
		return;
	}

	if (!TCP(client)) {
		peeraddr = &client->peeraddr;
	} else {
		peeraddr = NULL;
	}

	isc_nmhandle_attach(client->handle, &client->prefetchhandle);
	options = client->query.fetchoptions | DNS_FETCHOPT_PREFETCH;
	result = dns_resolver_createfetch(
		client->view->resolver, qname, rdataset->type, NULL, NULL, NULL,
		peeraddr, client->message->id, options, 0, NULL, client->task,
		prefetch_done, client, tmprdataset, NULL,
		&client->query.prefetch);
	if (result != ISC_R_SUCCESS) {
		ns_client_putrdataset(client, &tmprdataset);
		isc_nmhandle_detach(&client->prefetchhandle);
	}

	dns_rdataset_clearprefetch(rdataset);
	ns_stats_increment(client->sctx->nsstats, ns_statscounter_prefetch);
}

static void
rpz_clean(dns_zone_t **zonep, dns_db_t **dbp, dns_dbnode_t **nodep,
	  dns_rdataset_t **rdatasetp) {
	if (nodep != NULL && *nodep != NULL) {
		REQUIRE(dbp != NULL && *dbp != NULL);
		dns_db_detachnode(*dbp, nodep);
	}
	if (dbp != NULL && *dbp != NULL) {
		dns_db_detach(dbp);
	}
	if (zonep != NULL && *zonep != NULL) {
		dns_zone_detach(zonep);
	}
	if (rdatasetp != NULL && *rdatasetp != NULL &&
	    dns_rdataset_isassociated(*rdatasetp))
	{
		dns_rdataset_disassociate(*rdatasetp);
	}
}

static void
rpz_match_clear(dns_rpz_st_t *st) {
	rpz_clean(&st->m.zone, &st->m.db, &st->m.node, &st->m.rdataset);
	st->m.version = NULL;
}

static isc_result_t
rpz_ready(ns_client_t *client, dns_rdataset_t **rdatasetp) {
	REQUIRE(rdatasetp != NULL);

	CTRACE(ISC_LOG_DEBUG(3), "rpz_ready");

	if (*rdatasetp == NULL) {
		*rdatasetp = ns_client_newrdataset(client);
		if (*rdatasetp == NULL) {
			CTRACE(ISC_LOG_ERROR, "rpz_ready: "
					      "ns_client_newrdataset failed");
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
		ns_client_putrdataset(client, &st->m.rdataset);
	}
	rpz_match_clear(st);

	rpz_clean(NULL, &st->r.db, NULL, NULL);
	if (st->r.ns_rdataset != NULL) {
		ns_client_putrdataset(client, &st->r.ns_rdataset);
	}
	if (st->r.r_rdataset != NULL) {
		ns_client_putrdataset(client, &st->r.r_rdataset);
	}

	rpz_clean(&st->q.zone, &st->q.db, &st->q.node, NULL);
	if (st->q.rdataset != NULL) {
		ns_client_putrdataset(client, &st->q.rdataset);
	}
	if (st->q.sigrdataset != NULL) {
		ns_client_putrdataset(client, &st->q.sigrdataset);
	}
	st->state = 0;
	st->m.type = DNS_RPZ_TYPE_BAD;
	st->m.policy = DNS_RPZ_POLICY_MISS;
	if (st->rpsdb != NULL) {
		dns_db_detach(&st->rpsdb);
	}
}

static dns_rpz_zbits_t
rpz_get_zbits(ns_client_t *client, dns_rdatatype_t ip_type,
	      dns_rpz_type_t rpz_type) {
	dns_rpz_st_t *st;
	dns_rpz_zbits_t zbits = 0;

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
#endif /* ifdef USE_DNSRPS */

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
		UNREACHABLE();
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
		} else {
			zbits &= DNS_RPZ_ZMASK(st->m.rpz->num) >> 1;
		}
	}

	/*
	 * If the client wants recursion, allow only compatible policies.
	 */
	if (!RECURSIONOK(client)) {
		zbits &= st->popt.no_rd_ok;
	}

	return (zbits);
}

static void
query_rpzfetch(ns_client_t *client, dns_name_t *qname, dns_rdatatype_t type) {
	isc_result_t result;
	isc_sockaddr_t *peeraddr;
	dns_rdataset_t *tmprdataset;
	unsigned int options;

	CTRACE(ISC_LOG_DEBUG(3), "query_rpzfetch");

	if (client->query.prefetch != NULL) {
		return;
	}

	if (client->recursionquota == NULL) {
		result = isc_quota_attach(&client->sctx->recursionquota,
					  &client->recursionquota);
		switch (result) {
		case ISC_R_SUCCESS:
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_recursclients);
			break;
		case ISC_R_SOFTQUOTA:
			isc_quota_detach(&client->recursionquota);
			FALLTHROUGH;
		default:
			return;
		}
	}

	tmprdataset = ns_client_newrdataset(client);
	if (tmprdataset == NULL) {
		return;
	}

	if (!TCP(client)) {
		peeraddr = &client->peeraddr;
	} else {
		peeraddr = NULL;
	}

	options = client->query.fetchoptions;
	isc_nmhandle_attach(client->handle, &client->prefetchhandle);
	result = dns_resolver_createfetch(
		client->view->resolver, qname, type, NULL, NULL, NULL, peeraddr,
		client->message->id, options, 0, NULL, client->task,
		prefetch_done, client, tmprdataset, NULL,
		&client->query.prefetch);
	if (result != ISC_R_SUCCESS) {
		ns_client_putrdataset(client, &tmprdataset);
		isc_nmhandle_detach(&client->prefetchhandle);
	}
}

/*
 * Get an NS, A, or AAAA rrset related to the response for the client
 * to check the contents of that rrset for hits by eligible policy zones.
 */
static isc_result_t
rpz_rrset_find(ns_client_t *client, dns_name_t *name, dns_rdatatype_t type,
	       unsigned int options, dns_rpz_type_t rpz_type, dns_db_t **dbp,
	       dns_dbversion_t *version, dns_rdataset_t **rdatasetp,
	       bool resuming) {
	dns_rpz_st_t *st;
	bool is_zone;
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
		if (*rdatasetp != NULL) {
			ns_client_putrdataset(client, rdatasetp);
		}
		RESTORE(*rdatasetp, st->r.r_rdataset);
		result = st->r.r_result;
		if (result == DNS_R_DELEGATION) {
			CTRACE(ISC_LOG_ERROR, "RPZ recursing");
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, name,
				     rpz_type, "rpz_rrset_find(1)", result);
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
		is_zone = false;
	} else {
		dns_zone_t *zone;

		version = NULL;
		zone = NULL;
		result = query_getdb(client, name, type, 0, &zone, dbp,
				     &version, &is_zone);
		if (result != ISC_R_SUCCESS) {
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, name,
				     rpz_type, "rpz_rrset_find(2)", result);
			st->m.policy = DNS_RPZ_POLICY_ERROR;
			if (zone != NULL) {
				dns_zone_detach(&zone);
			}
			return (result);
		}
		if (zone != NULL) {
			dns_zone_detach(&zone);
		}
	}

	node = NULL;
	found = dns_fixedname_initname(&fixed);
	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);
	result = dns_db_findext(*dbp, name, version, type, options, client->now,
				&node, found, &cm, &ci, *rdatasetp, NULL);
	if (result == DNS_R_DELEGATION && is_zone && USECACHE(client)) {
		/*
		 * Try the cache if we're authoritative for an
		 * ancestor but not the domain itself.
		 */
		rpz_clean(NULL, dbp, &node, rdatasetp);
		version = NULL;
		dns_db_attach(client->view->cachedb, dbp);
		result = dns_db_findext(*dbp, name, version, type, 0,
					client->now, &node, found, &cm, &ci,
					*rdatasetp, NULL);
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
			dns_name_copynf(name, st->r_name);
			result = ns_query_recurse(client, type, st->r_name,
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
rpz_get_p_name(ns_client_t *client, dns_name_t *p_name, dns_rpz_zone_t *rpz,
	       dns_rpz_type_t rpz_type, dns_name_t *trig_name) {
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
		UNREACHABLE();
	}

	/*
	 * Start with relative version of the full trigger name,
	 * and trim enough allow the addition of the suffix.
	 */
	dns_name_init(&prefix, prefix_offsets);
	labels = dns_name_countlabels(trig_name);
	first = 0;
	for (;;) {
		dns_name_getlabelsequence(trig_name, first, labels - first - 1,
					  &prefix);
		result = dns_name_concatenate(&prefix, suffix, p_name, NULL);
		if (result == ISC_R_SUCCESS) {
			break;
		}
		INSIST(result == DNS_R_NAMETOOLONG);
		/*
		 * Trim the trigger name until the combination is not too long.
		 */
		if (labels - first < 2) {
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, suffix,
				     rpz_type, "concatenate()", result);
			return (ISC_R_FAILURE);
		}
		/*
		 * Complain once about trimming the trigger name.
		 */
		if (first == 0) {
			rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL1, suffix,
				     rpz_type, "concatenate()", result);
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
	   dns_rpz_policy_t *policyp) {
	dns_fixedname_t foundf;
	dns_name_t *found;
	isc_result_t result;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	bool found_a = false;

	REQUIRE(nodep != NULL);

	CTRACE(ISC_LOG_DEBUG(3), "rpz_find_p");

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

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
	if (result != ISC_R_SUCCESS) {
		return (DNS_R_NXDOMAIN);
	}
	found = dns_fixedname_initname(&foundf);

	result = dns_db_findext(*dbp, p_name, *versionp, dns_rdatatype_any, 0,
				client->now, nodep, found, &cm, &ci, *rdatasetp,
				NULL);
	/*
	 * Choose the best rdataset if we found something.
	 */
	if (result == ISC_R_SUCCESS) {
		dns_rdatasetiter_t *rdsiter;

		rdsiter = NULL;
		result = dns_db_allrdatasets(*dbp, *nodep, *versionp, 0, 0,
					     &rdsiter);
		if (result != ISC_R_SUCCESS) {
			rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, p_name,
				     rpz_type, "allrdatasets()", result);
			CTRACE(ISC_LOG_ERROR,
			       "rpz_find_p: allrdatasets failed");
			return (DNS_R_SERVFAIL);
		}
		if (qtype == dns_rdatatype_aaaa &&
		    !ISC_LIST_EMPTY(client->view->dns64))
		{
			for (result = dns_rdatasetiter_first(rdsiter);
			     result == ISC_R_SUCCESS;
			     result = dns_rdatasetiter_next(rdsiter))
			{
				dns_rdatasetiter_current(rdsiter, *rdatasetp);
				if ((*rdatasetp)->type == dns_rdatatype_a) {
					found_a = true;
				}
				dns_rdataset_disassociate(*rdatasetp);
			}
		}
		for (result = dns_rdatasetiter_first(rdsiter);
		     result == ISC_R_SUCCESS;
		     result = dns_rdatasetiter_next(rdsiter))
		{
			dns_rdatasetiter_current(rdsiter, *rdatasetp);
			if ((*rdatasetp)->type == dns_rdatatype_cname ||
			    (*rdatasetp)->type == qtype)
			{
				break;
			}
			dns_rdataset_disassociate(*rdatasetp);
		}
		dns_rdatasetiter_destroy(&rdsiter);
		if (result != ISC_R_SUCCESS) {
			if (result != ISC_R_NOMORE) {
				rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL,
					     p_name, rpz_type, "rdatasetiter",
					     result);
				CTRACE(ISC_LOG_ERROR, "rpz_find_p: "
						      "rdatasetiter failed");
				return (DNS_R_SERVFAIL);
			}
			/*
			 * Ask again to get the right DNS_R_DNAME/NXRRSET/...
			 * result if there is neither a CNAME nor target type.
			 */
			if (dns_rdataset_isassociated(*rdatasetp)) {
				dns_rdataset_disassociate(*rdatasetp);
			}
			dns_db_detachnode(*dbp, nodep);

			if (qtype == dns_rdatatype_rrsig ||
			    qtype == dns_rdatatype_sig)
			{
				result = DNS_R_NXRRSET;
			} else {
				result = dns_db_findext(*dbp, p_name, *versionp,
							qtype, 0, client->now,
							nodep, found, &cm, &ci,
							*rdatasetp, NULL);
			}
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
			{
				return (DNS_R_CNAME);
			}
		}
		return (ISC_R_SUCCESS);
	case DNS_R_NXRRSET:
		if (found_a) {
			*policyp = DNS_RPZ_POLICY_DNS64;
		} else {
			*policyp = DNS_RPZ_POLICY_NODATA;
		}
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
		rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, p_name, rpz_type, "",
			     result);
		CTRACE(ISC_LOG_ERROR, "rpz_find_p: unexpected result");
		return (DNS_R_SERVFAIL);
	}
}

static void
rpz_save_p(dns_rpz_st_t *st, dns_rpz_zone_t *rpz, dns_rpz_type_t rpz_type,
	   dns_rpz_policy_t policy, dns_name_t *p_name, dns_rpz_prefix_t prefix,
	   isc_result_t result, dns_zone_t **zonep, dns_db_t **dbp,
	   dns_dbnode_t **nodep, dns_rdataset_t **rdatasetp,
	   dns_dbversion_t *version) {
	dns_rdataset_t *trdataset = NULL;

	rpz_match_clear(st);
	st->m.rpz = rpz;
	st->m.type = rpz_type;
	st->m.policy = policy;
	dns_name_copynf(p_name, st->p_name);
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
	  bool recursed) {
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
	rpz_log_rewrite(client, true, dns_dnsrps_2policy(rpsdb->result.zpolicy),
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
static bool
dnsrps_set_p(librpz_emsg_t *emsg, ns_client_t *client, dns_rpz_st_t *st,
	     dns_rdatatype_t qtype, dns_rdataset_t **p_rdatasetp,
	     bool recursed) {
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
		return (false);
	}

	if (rpsdb->result.policy == LIBRPZ_POLICY_UNDEFINED) {
		return (true);
	}

	/*
	 * Give the fake or shim DNSRPS database its new origin.
	 */
	if (!librpz->rsp_soa(emsg, NULL, NULL, &rpsdb->origin_buf,
			     &rpsdb->result, rpsdb->rsp))
	{
		return (false);
	}
	region.base = rpsdb->origin_buf.d;
	region.length = rpsdb->origin_buf.size;
	dns_name_fromregion(&rpsdb->common.origin, &region);

	if (!librpz->rsp_domain(emsg, &pname_buf, rpsdb->rsp)) {
		return (false);
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
			return (false);
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
		result = dns_db_find(p_db, st->p_name, NULL, searchtype, 0, 0,
				     &p_node, found, *p_rdatasetp, NULL);

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
			return (false);
		}
	}

	rpz_save_p(st, client->view->rpzs->zones[rpsdb->result.cznum],
		   dns_dnsrps_trig2type(rpsdb->result.trig), policy, st->p_name,
		   0, result, &p_zone, &p_db, &p_node, p_rdatasetp, NULL);

	rpz_clean(NULL, NULL, NULL, p_rdatasetp);

	return (true);
}

static isc_result_t
dnsrps_rewrite_ip(ns_client_t *client, const isc_netaddr_t *netaddr,
		  dns_rpz_type_t rpz_type, dns_rdataset_t **p_rdatasetp) {
	dns_rpz_st_t *st;
	rpsdb_t *rpsdb;
	librpz_trig_t trig = LIBRPZ_TRIG_CLIENT_IP;
	bool recursed = false;
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
		recursed = false;
		break;
	case DNS_RPZ_TYPE_IP:
		trig = LIBRPZ_TRIG_IP;
		recursed = true;
		break;
	case DNS_RPZ_TYPE_NSIP:
		trig = LIBRPZ_TRIG_NSIP;
		recursed = true;
		break;
	default:
		UNREACHABLE();
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
dnsrps_rewrite_name(ns_client_t *client, dns_name_t *trig_name, bool recursed,
		    dns_rpz_type_t rpz_type, dns_rdataset_t **p_rdatasetp) {
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
		UNREACHABLE();
	}

	dns_name_toregion(trig_name, &r);
	do {
		if (!librpz->rsp_push(&emsg, rpsdb->rsp) ||
		    !librpz->ck_domain(&emsg, r.base, r.length, trig,
				       ++rpsdb->hit_id, recursed, rpsdb->rsp) ||
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
	       dns_rpz_zbits_t zbits, dns_rdataset_t **p_rdatasetp) {
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
#endif /* ifdef USE_DNSRPS */

	ip_name = dns_fixedname_initname(&ip_namef);

	p_zone = NULL;
	p_db = NULL;
	p_node = NULL;

	while (zbits != 0) {
		rpz_num = dns_rpz_find_ip(rpzs, rpz_type, zbits, netaddr,
					  ip_name, &prefix);
		if (rpz_num == DNS_RPZ_INVALID_NUM) {
			break;
		}
		zbits &= (DNS_RPZ_ZMASK(rpz_num) >> 1);

		/*
		 * Do not try applying policy zones that cannot replace a
		 * previously found policy zone.
		 * Stop looking if the next best choice cannot
		 * replace what we already have.
		 */
		rpz = rpzs->zones[rpz_num];
		if (st->m.policy != DNS_RPZ_POLICY_MISS) {
			if (st->m.rpz->num < rpz->num) {
				break;
			}
			if (st->m.rpz->num == rpz->num &&
			    (st->m.type < rpz_type || st->m.prefix > prefix))
			{
				break;
			}
		}

		/*
		 * Get the policy for a prefix at least as long
		 * as the prefix of the entry we had before.
		 */
		p_name = dns_fixedname_initname(&p_namef);
		result = rpz_get_p_name(client, p_name, rpz, rpz_type, ip_name);
		if (result != ISC_R_SUCCESS) {
			continue;
		}
		result = rpz_find_p(client, ip_name, qtype, p_name, rpz,
				    rpz_type, &p_zone, &p_db, &p_version,
				    &p_node, p_rdatasetp, &policy);
		switch (result) {
		case DNS_R_NXDOMAIN:
			/*
			 * Continue after a policy record that is missing
			 * contrary to the summary data.  The summary
			 * data can out of date during races with and among
			 * policy zone updates.
			 */
			CTRACE(ISC_LOG_ERROR, "rpz_rewrite_ip: mismatched "
					      "summary data; "
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
			    (st->m.type == rpz_type && st->m.prefix == prefix &&
			     0 > dns_name_rdatacompare(st->p_name, p_name)))
			{
				break;
			}

			/*
			 * Stop checking after saving an enabled hit in this
			 * policy zone.  The radix tree in the policy zone
			 * ensures that we found the longest match.
			 */
			if (rpz->policy != DNS_RPZ_POLICY_DISABLED) {
				CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_ip: "
							 "rpz_save_p");
				rpz_save_p(st, rpz, rpz_type, policy, p_name,
					   prefix, result, &p_zone, &p_db,
					   &p_node, p_rdatasetp, p_version);
				break;
			}

			/*
			 * Log DNS_RPZ_POLICY_DISABLED zones
			 * and try the next eligible policy zone.
			 */
			rpz_log_rewrite(client, true, policy, rpz_type, p_zone,
					p_name, NULL, rpz_num);
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
rpz_rewrite_ip_rrset(ns_client_t *client, dns_name_t *name,
		     dns_rdatatype_t qtype, dns_rpz_type_t rpz_type,
		     dns_rdatatype_t ip_type, dns_db_t **ip_dbp,
		     dns_dbversion_t *ip_version, dns_rdataset_t **ip_rdatasetp,
		     dns_rdataset_t **p_rdatasetp, bool resuming) {
	dns_rpz_zbits_t zbits;
	isc_netaddr_t netaddr;
	struct in_addr ina;
	struct in6_addr in6a;
	isc_result_t result;
	unsigned int options = client->query.dboptions | DNS_DBFIND_GLUEOK;
	bool done = false;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_ip_rrset");

	do {
		zbits = rpz_get_zbits(client, ip_type, rpz_type);
		if (zbits == 0) {
			return (ISC_R_SUCCESS);
		}

		/*
		 * Get the A or AAAA rdataset.
		 */
		result = rpz_rrset_find(client, name, ip_type, options,
					rpz_type, ip_dbp, ip_version,
					ip_rdatasetp, resuming);
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
			rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL1, name,
				     rpz_type, "NS address rewrite rrset",
				     result);
			return (ISC_R_SUCCESS);
		default:
			if (client->query.rpz_st->m.policy !=
			    DNS_RPZ_POLICY_ERROR)
			{
				client->query.rpz_st->m.policy =
					DNS_RPZ_POLICY_ERROR;
				rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, name,
					     rpz_type,
					     "NS address rewrite rrset",
					     result);
			}
			CTRACE(ISC_LOG_ERROR,
			       "rpz_rewrite_ip_rrset: unexpected "
			       "result");
			return (DNS_R_SERVFAIL);
		}

		/*
		 * If we are processing glue setup for the next loop
		 * otherwise we are done.
		 */
		if (result == DNS_R_GLUE) {
			options = client->query.dboptions;
		} else {
			options = client->query.dboptions | DNS_DBFIND_GLUEOK;
			done = true;
		}

		/*
		 * Check all of the IP addresses in the rdataset.
		 */
		for (result = dns_rdataset_first(*ip_rdatasetp);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(*ip_rdatasetp))
		{
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

			result = rpz_rewrite_ip(client, &netaddr, qtype,
						rpz_type, zbits, p_rdatasetp);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}
		}
	} while (!done &&
		 client->query.rpz_st->m.policy == DNS_RPZ_POLICY_MISS);

	return (ISC_R_SUCCESS);
}

/*
 * Look for IP addresses in A and AAAA rdatasets
 * that trigger all eligible IP or NSIP policy rules.
 */
static isc_result_t
rpz_rewrite_ip_rrsets(ns_client_t *client, dns_name_t *name,
		      dns_rdatatype_t qtype, dns_rpz_type_t rpz_type,
		      dns_rdataset_t **ip_rdatasetp, bool resuming) {
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
	    (qtype == dns_rdatatype_a || qtype == dns_rdatatype_any ||
	     rpz_type == DNS_RPZ_TYPE_NSIP))
	{
		/*
		 * Rewrite based on an IPv4 address that will appear
		 * in the ANSWER section or if we are checking IP addresses.
		 */
		result = rpz_rewrite_ip_rrset(
			client, name, qtype, rpz_type, dns_rdatatype_a, &ip_db,
			ip_version, ip_rdatasetp, &p_rdataset, resuming);
		if (result == ISC_R_SUCCESS) {
			st->state |= DNS_RPZ_DONE_IPv4;
		}
	} else {
		result = ISC_R_SUCCESS;
	}
	if (result == ISC_R_SUCCESS &&
	    (qtype == dns_rdatatype_aaaa || qtype == dns_rdatatype_any ||
	     rpz_type == DNS_RPZ_TYPE_NSIP))
	{
		/*
		 * Rewrite based on IPv6 addresses that will appear
		 * in the ANSWER section or if we are checking IP addresses.
		 */
		result = rpz_rewrite_ip_rrset(client, name, qtype, rpz_type,
					      dns_rdatatype_aaaa, &ip_db,
					      ip_version, ip_rdatasetp,
					      &p_rdataset, resuming);
	}
	if (ip_db != NULL) {
		dns_db_detach(&ip_db);
	}
	ns_client_putrdataset(client, &p_rdataset);
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
		 dns_rpz_zbits_t allowed_zbits, bool recursed,
		 dns_rdataset_t **rdatasetp) {
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
#endif /* ifndef USE_DNSRPS */

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_name");

	rpzs = client->view->rpzs;
	st = client->query.rpz_st;

#ifdef USE_DNSRPS
	if (st->popt.dnsrps_enabled) {
		return (dnsrps_rewrite_name(client, trig_name, recursed,
					    rpz_type, rdatasetp));
	}
#endif /* ifdef USE_DNSRPS */

	zbits = rpz_get_zbits(client, qtype, rpz_type);
	zbits &= allowed_zbits;
	if (zbits == 0) {
		return (ISC_R_SUCCESS);
	}

	/*
	 * Use the summary database to find the bit mask of policy zones
	 * with policies for this trigger name. We do this even if there
	 * is only one eligible policy zone so that wildcard triggers
	 * are matched correctly, and not into their parent.
	 */
	zbits = dns_rpz_find_name(rpzs, rpz_type, zbits, trig_name);
	if (zbits == 0) {
		return (ISC_R_SUCCESS);
	}

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
		if ((zbits & 1) == 0) {
			continue;
		}

		/*
		 * Do not check policy zones that cannot replace a previously
		 * found policy.
		 */
		rpz = rpzs->zones[rpz_num];
		if (st->m.policy != DNS_RPZ_POLICY_MISS) {
			if (st->m.rpz->num < rpz->num) {
				break;
			}
			if (st->m.rpz->num == rpz->num && st->m.type < rpz_type)
			{
				break;
			}
		}

		/*
		 * Get the next policy zone's record for this trigger name.
		 */
		result = rpz_get_p_name(client, p_name, rpz, rpz_type,
					trig_name);
		if (result != ISC_R_SUCCESS) {
			continue;
		}
		result = rpz_find_p(client, trig_name, qtype, p_name, rpz,
				    rpz_type, &p_zone, &p_db, &p_version,
				    &p_node, rdatasetp, &policy);
		switch (result) {
		case DNS_R_NXDOMAIN:
			/*
			 * Continue after a missing policy record
			 * contrary to the summary data.  The summary
			 * data can out of date during races with and among
			 * policy zone updates.
			 */
			CTRACE(ISC_LOG_ERROR, "rpz_rewrite_name: mismatched "
					      "summary data; "
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
			{
				continue;
			}

			if (rpz->policy != DNS_RPZ_POLICY_DISABLED) {
				CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_name: "
							 "rpz_save_p");
				rpz_save_p(st, rpz, rpz_type, policy, p_name, 0,
					   result, &p_zone, &p_db, &p_node,
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
			rpz_log_rewrite(client, true, policy, rpz_type, p_zone,
					p_name, NULL, rpz_num);
			break;
		}
	}

	rpz_clean(&p_zone, &p_db, &p_node, rdatasetp);
	return (ISC_R_SUCCESS);
}

static void
rpz_rewrite_ns_skip(ns_client_t *client, dns_name_t *nsname,
		    isc_result_t result, int level, const char *str) {
	dns_rpz_st_t *st;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite_ns_skip");

	st = client->query.rpz_st;

	if (str != NULL) {
		rpz_log_fail_helper(client, level, nsname, DNS_RPZ_TYPE_NSIP,
				    DNS_RPZ_TYPE_NSDNAME, str, result);
	}
	if (st->r.ns_rdataset != NULL &&
	    dns_rdataset_isassociated(st->r.ns_rdataset))
	{
		dns_rdataset_disassociate(st->r.ns_rdataset);
	}

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
rpz_rewrite(ns_client_t *client, dns_rdatatype_t qtype, isc_result_t qresult,
	    bool resuming, dns_rdataset_t *ordataset, dns_rdataset_t *osigset) {
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
	unsigned int options;
#ifdef USE_DNSRPS
	librpz_emsg_t emsg;
#endif /* ifdef USE_DNSRPS */

	CTRACE(ISC_LOG_DEBUG(3), "rpz_rewrite");

	rpzs = client->view->rpzs;
	st = client->query.rpz_st;

	if (rpzs == NULL) {
		return (ISC_R_NOTFOUND);
	}
	if (st != NULL && (st->state & DNS_RPZ_REWRITTEN) != 0) {
		return (DNS_R_DISALLOWED);
	}
	if (RECURSING(client)) {
		return (DNS_R_DISALLOWED);
	}

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
#endif /* ifndef USE_DNSRPS */

	if (st == NULL) {
		st = isc_mem_get(client->mctx, sizeof(*st));
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
			result = dns_dnsrps_rewrite_init(
				&emsg, st, rpzs, client->query.qname,
				client->mctx, RECURSIONOK(client));
			if (result != ISC_R_SUCCESS) {
				rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, NULL,
					     DNS_RPZ_TYPE_QNAME, emsg.c,
					     result);
				st->m.policy = DNS_RPZ_POLICY_ERROR;
				return (ISC_R_SUCCESS);
			}
		}
#endif /* ifdef USE_DNSRPS */
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
	case DNS_R_COVERINGNSEC:
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
		if (RECURSIONOK(client)) {
			qresult_type = qresult_type_recurse;
		} else {
			qresult_type = qresult_type_restart;
		}
		break;
	case ISC_R_FAILURE:
	case ISC_R_TIMEDOUT:
	case DNS_R_BROKENCHAIN:
		rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL3, NULL,
			     DNS_RPZ_TYPE_QNAME,
			     "stop on qresult in rpz_rewrite()", qresult);
		return (ISC_R_SUCCESS);
	default:
		rpz_log_fail(client, DNS_RPZ_DEBUG_LEVEL1, NULL,
			     DNS_RPZ_TYPE_QNAME,
			     "stop on unrecognized qresult in rpz_rewrite()",
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
				if (result != ISC_R_SUCCESS) {
					goto cleanup;
				}
			}
		}

		/*
		 * Check triggers for the query name if this is the first time
		 * for the current qname.
		 * There is a first time for each name in a CNAME chain
		 */
		if ((st->state & DNS_RPZ_DONE_QNAME) == 0) {
			bool norec = (qresult_type != qresult_type_recurse);
			result = rpz_rewrite_name(client, client->query.qname,
						  qtype, DNS_RPZ_TYPE_QNAME,
						  allowed, norec, &rdataset);
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}

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
		 * because recursion is so slow.
		 */
		if (qresult_type == qresult_type_recurse) {
			goto cleanup;
		}

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
	    rpz_get_zbits(client, qtype, DNS_RPZ_TYPE_IP) != 0)
	{
		result = rpz_rewrite_ip_rrsets(client, client->query.qname,
					       qtype, DNS_RPZ_TYPE_IP,
					       &rdataset, resuming);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		/*
		 * We are finished checking the IP addresses for the qname.
		 * Start with IPv4 if we will check NS IP addresses.
		 */
		st->state |= DNS_RPZ_DONE_QNAME_IP;
		st->state &= ~DNS_RPZ_DONE_IPv4;
	}

	/*
	 * Stop looking for rules if there are none of the other kinds
	 * that could override what we already have.
	 */
	if (rpz_get_zbits(client, dns_rdatatype_any, DNS_RPZ_TYPE_NSDNAME) ==
		    0 &&
	    rpz_get_zbits(client, dns_rdatatype_any, DNS_RPZ_TYPE_NSIP) == 0)
	{
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	dns_fixedname_init(&nsnamef);
	dns_name_clone(client->query.qname, dns_fixedname_name(&nsnamef));
	options = client->query.dboptions | DNS_DBFIND_GLUEOK;
	while (st->r.label > st->popt.min_ns_labels) {
		bool was_glue = false;
		/*
		 * Get NS rrset for each domain in the current qname.
		 */
		if (st->r.label == dns_name_countlabels(client->query.qname)) {
			nsname = client->query.qname;
		} else {
			nsname = dns_fixedname_name(&nsnamef);
			dns_name_split(client->query.qname, st->r.label, NULL,
				       nsname);
		}
		if (st->r.ns_rdataset == NULL ||
		    !dns_rdataset_isassociated(st->r.ns_rdataset))
		{
			dns_db_t *db = NULL;
			result = rpz_rrset_find(client, nsname,
						dns_rdatatype_ns, options,
						DNS_RPZ_TYPE_NSDNAME, &db, NULL,
						&st->r.ns_rdataset, resuming);
			if (db != NULL) {
				dns_db_detach(&db);
			}
			if (st->m.policy == DNS_RPZ_POLICY_ERROR) {
				goto cleanup;
			}
			switch (result) {
			case DNS_R_GLUE:
				was_glue = true;
				FALLTHROUGH;
			case ISC_R_SUCCESS:
				result = dns_rdataset_first(st->r.ns_rdataset);
				if (result != ISC_R_SUCCESS) {
					goto cleanup;
				}
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
				rpz_rewrite_ns_skip(client, nsname, result, 0,
						    NULL);
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
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			dns_rdata_reset(&nsrdata);

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
				result = rpz_rewrite_name(
					client, &ns.name, qtype,
					DNS_RPZ_TYPE_NSDNAME, DNS_RPZ_ALL_ZBITS,
					true, &rdataset);
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
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
			st->state &= ~(DNS_RPZ_DONE_NSDNAME |
				       DNS_RPZ_DONE_IPv4);
			result = dns_rdataset_next(st->r.ns_rdataset);
		} while (result == ISC_R_SUCCESS);
		dns_rdataset_disassociate(st->r.ns_rdataset);

		/*
		 * If we just checked a glue NS RRset retry without allowing
		 * glue responses, otherwise setup for the next name.
		 */
		if (was_glue) {
			options = client->query.dboptions;
		} else {
			options = client->query.dboptions | DNS_DBFIND_GLUEOK;
			st->r.label--;
		}

		if (rpz_get_zbits(client, dns_rdatatype_any,
				  DNS_RPZ_TYPE_NSDNAME) == 0 &&
		    rpz_get_zbits(client, dns_rdatatype_any,
				  DNS_RPZ_TYPE_NSIP) == 0)
		{
			break;
		}
	}

	/*
	 * Use the best hit, if any.
	 */
	result = ISC_R_SUCCESS;

cleanup:
#ifdef USE_DNSRPS
	if (st->popt.dnsrps_enabled && st->m.policy != DNS_RPZ_POLICY_ERROR &&
	    !dnsrps_set_p(&emsg, client, st, qtype, &rdataset,
			  (qresult_type != qresult_type_recurse)))
	{
		rpz_log_fail(client, DNS_RPZ_ERROR_LEVEL, NULL,
			     DNS_RPZ_TYPE_BAD, emsg.c, DNS_R_SERVFAIL);
		st->m.policy = DNS_RPZ_POLICY_ERROR;
	}
#endif /* ifdef USE_DNSRPS */
	if (st->m.policy != DNS_RPZ_POLICY_MISS &&
	    st->m.policy != DNS_RPZ_POLICY_ERROR &&
	    st->m.rpz->policy != DNS_RPZ_POLICY_GIVEN)
	{
		st->m.policy = st->m.rpz->policy;
	}
	if (st->m.policy == DNS_RPZ_POLICY_MISS ||
	    st->m.policy == DNS_RPZ_POLICY_PASSTHRU ||
	    st->m.policy == DNS_RPZ_POLICY_ERROR)
	{
		if (st->m.policy == DNS_RPZ_POLICY_PASSTHRU &&
		    result != DNS_R_DELEGATION)
		{
			rpz_log_rewrite(client, false, st->m.policy, st->m.type,
					st->m.zone, st->p_name, NULL,
					st->m.rpz->num);
		}
		rpz_match_clear(st);
	}
	if (st->m.policy == DNS_RPZ_POLICY_ERROR) {
		CTRACE(ISC_LOG_ERROR, "SERVFAIL due to RPZ policy");
		st->m.type = DNS_RPZ_TYPE_BAD;
		result = DNS_R_SERVFAIL;
	}
	ns_client_putrdataset(client, &rdataset);
	if ((st->state & DNS_RPZ_RECURSING) == 0) {
		rpz_clean(NULL, &st->r.db, NULL, &st->r.ns_rdataset);
	}

	return (result);
}

/*
 * See if response policy zone rewriting is allowed by a lack of interest
 * by the client in DNSSEC or a lack of signatures.
 */
static bool
rpz_ck_dnssec(ns_client_t *client, isc_result_t qresult,
	      dns_rdataset_t *rdataset, dns_rdataset_t *sigrdataset) {
	dns_fixedname_t fixed;
	dns_name_t *found;
	dns_rdataset_t trdataset;
	dns_rdatatype_t type;
	isc_result_t result;

	CTRACE(ISC_LOG_DEBUG(3), "rpz_ck_dnssec");

	if (client->view->rpzs->p.break_dnssec || !WANTDNSSEC(client)) {
		return (true);
	}

	/*
	 * We do not know if there are signatures if we have not recursed
	 * for them.
	 */
	if (qresult == DNS_R_DELEGATION || qresult == ISC_R_NOTFOUND) {
		return (false);
	}

	if (sigrdataset == NULL) {
		return (true);
	}
	if (dns_rdataset_isassociated(sigrdataset)) {
		return (false);
	}

	/*
	 * We are happy to rewrite nothing.
	 */
	if (rdataset == NULL || !dns_rdataset_isassociated(rdataset)) {
		return (true);
	}
	/*
	 * Do not rewrite if there is any sign of signatures.
	 */
	if (rdataset->type == dns_rdatatype_nsec ||
	    rdataset->type == dns_rdatatype_nsec3 ||
	    rdataset->type == dns_rdatatype_rrsig)
	{
		return (false);
	}

	/*
	 * Look for a signature in a negative cache rdataset.
	 */
	if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) == 0) {
		return (true);
	}
	found = dns_fixedname_initname(&fixed);
	dns_rdataset_init(&trdataset);
	for (result = dns_rdataset_first(rdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(rdataset))
	{
		dns_ncache_current(rdataset, found, &trdataset);
		type = trdataset.type;
		dns_rdataset_disassociate(&trdataset);
		if (type == dns_rdatatype_nsec || type == dns_rdatatype_nsec3 ||
		    type == dns_rdatatype_rrsig)
		{
			return (false);
		}
	}
	return (true);
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
static unsigned char hostmaster_data[] = "\012hostmaster\014root-"
					 "servers\003org";

static unsigned char prisoner_offsets[] = { 0, 9, 14, 18 };
static unsigned char hostmaster_offsets[] = { 0, 11, 24, 28 };

static dns_name_t const prisoner = DNS_NAME_INITABSOLUTE(prisoner_data,
							 prisoner_offsets);
static dns_name_t const hostmaster = DNS_NAME_INITABSOLUTE(hostmaster_data,
							   hostmaster_offsets);

static void
warn_rfc1918(ns_client_t *client, dns_name_t *fname, dns_rdataset_t *rdataset) {
	unsigned int i;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_soa_t soa;
	dns_rdataset_t found;
	isc_result_t result;

	for (i = 0; i < (sizeof(rfc1918names) / sizeof(*rfc1918names)); i++) {
		if (dns_name_issubdomain(fname, &rfc1918names[i])) {
			dns_rdataset_init(&found);
			result = dns_ncache_getrdataset(
				rdataset, &rfc1918names[i], dns_rdatatype_soa,
				&found);
			if (result != ISC_R_SUCCESS) {
				return;
			}

			result = dns_rdataset_first(&found);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			dns_rdataset_current(&found, &rdata);
			result = dns_rdata_tostruct(&rdata, &soa, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			if (dns_name_equal(&soa.origin, &prisoner) &&
			    dns_name_equal(&soa.contact, &hostmaster))
			{
				char buf[DNS_NAME_FORMATSIZE];
				dns_name_format(fname, buf, sizeof(buf));
				ns_client_log(client, DNS_LOGCATEGORY_SECURITY,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_WARNING,
					      "RFC 1918 response from "
					      "Internet for %s",
					      buf);
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
		       dns_name_t *fname, bool exact, dns_name_t *found) {
	unsigned char salt[256];
	size_t salt_length;
	uint16_t iterations;
	isc_result_t result;
	unsigned int dboptions;
	dns_fixedname_t fixed;
	dns_hash_t hash;
	dns_name_t name;
	unsigned int skip = 0, labels;
	dns_rdata_nsec3_t nsec3;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	bool optout;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	salt_length = sizeof(salt);
	result = dns_db_getnsec3parameters(db, version, &hash, NULL,
					   &iterations, salt, &salt_length);
	if (result != ISC_R_SUCCESS) {
		return;
	}

	dns_name_init(&name, NULL);
	dns_name_clone(qname, &name);
	labels = dns_name_countlabels(&name);
	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

	/*
	 * Map unknown algorithm to known value.
	 */
	if (hash == DNS_NSEC3_UNKNOWNALG) {
		hash = 1;
	}

again:
	dns_fixedname_init(&fixed);
	result = dns_nsec3_hashname(&fixed, NULL, NULL, &name,
				    dns_db_origin(db), hash, iterations, salt,
				    salt_length);
	if (result != ISC_R_SUCCESS) {
		return;
	}

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
		result = dns_rdata_tostruct(&rdata, &nsec3, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		dns_rdata_reset(&rdata);
		optout = ((nsec3.flags & DNS_NSEC3FLAG_OPTOUT) != 0);
		if (found != NULL && optout &&
		    dns_name_issubdomain(&name, dns_db_origin(db)))
		{
			dns_rdataset_disassociate(rdataset);
			if (dns_rdataset_isassociated(sigrdataset)) {
				dns_rdataset_disassociate(sigrdataset);
			}
			skip++;
			dns_name_getlabelsequence(qname, skip, labels - skip,
						  &name);
			ns_client_log(client, DNS_LOGCATEGORY_DNSSEC,
				      NS_LOGMODULE_QUERY, ISC_LOG_DEBUG(3),
				      "looking for closest provable encloser");
			goto again;
		}
		if (exact) {
			ns_client_log(client, DNS_LOGCATEGORY_DNSSEC,
				      NS_LOGMODULE_QUERY, ISC_LOG_WARNING,
				      "expected a exact match NSEC3, got "
				      "a covering record");
		}
	} else if (result != ISC_R_SUCCESS) {
		return;
	} else if (!exact) {
		ns_client_log(client, DNS_LOGCATEGORY_DNSSEC,
			      NS_LOGMODULE_QUERY, ISC_LOG_WARNING,
			      "expected covering NSEC3, got an exact match");
	}
	if (found == qname) {
		if (skip != 0U) {
			dns_name_getlabelsequence(qname, skip, labels - skip,
						  found);
		}
	} else if (found != NULL) {
		dns_name_copynf(&name, found);
	}
	return;
}

static uint32_t
dns64_ttl(dns_db_t *db, dns_dbversion_t *version) {
	dns_dbnode_t *node = NULL;
	dns_rdata_soa_t soa;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdataset_t rdataset;
	isc_result_t result;
	uint32_t ttl = UINT32_MAX;

	dns_rdataset_init(&rdataset);

	result = dns_db_getoriginnode(db, &node);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = dns_db_findrdataset(db, node, version, dns_rdatatype_soa, 0, 0,
				     &rdataset, NULL);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_rdataset_first(&rdataset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	dns_rdataset_current(&rdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &soa, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	ttl = ISC_MIN(rdataset.ttl, soa.minimum);

cleanup:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}
	if (node != NULL) {
		dns_db_detachnode(db, &node);
	}
	return (ttl);
}

static bool
dns64_aaaaok(ns_client_t *client, dns_rdataset_t *rdataset,
	     dns_rdataset_t *sigrdataset) {
	isc_netaddr_t netaddr;
	dns_aclenv_t *env =
		ns_interfacemgr_getaclenv(client->manager->interface->mgr);
	dns_dns64_t *dns64 = ISC_LIST_HEAD(client->view->dns64);
	unsigned int flags = 0;
	unsigned int i, count;
	bool *aaaaok;

	INSIST(client->query.dns64_aaaaok == NULL);
	INSIST(client->query.dns64_aaaaoklen == 0);
	INSIST(client->query.dns64_aaaa == NULL);
	INSIST(client->query.dns64_sigaaaa == NULL);

	if (dns64 == NULL) {
		return (true);
	}

	if (RECURSIONOK(client)) {
		flags |= DNS_DNS64_RECURSIVE;
	}

	if (WANTDNSSEC(client) && sigrdataset != NULL &&
	    dns_rdataset_isassociated(sigrdataset))
	{
		flags |= DNS_DNS64_DNSSEC;
	}

	count = dns_rdataset_count(rdataset);
	aaaaok = isc_mem_get(client->mctx, sizeof(bool) * count);

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	if (dns_dns64_aaaaok(dns64, &netaddr, client->signer, env, flags,
			     rdataset, aaaaok, count))
	{
		for (i = 0; i < count; i++) {
			if (aaaaok != NULL && !aaaaok[i]) {
				SAVE(client->query.dns64_aaaaok, aaaaok);
				client->query.dns64_aaaaoklen = count;
				break;
			}
		}
		if (aaaaok != NULL) {
			isc_mem_put(client->mctx, aaaaok, sizeof(bool) * count);
		}
		return (true);
	}
	if (aaaaok != NULL) {
		isc_mem_put(client->mctx, aaaaok, sizeof(bool) * count);
	}
	return (false);
}

/*
 * Look for the name and type in the redirection zone.  If found update
 * the arguments as appropriate.  Return true if a update was
 * performed.
 *
 * Only perform the update if the client is in the allow query acl and
 * returning the update would not cause a DNSSEC validation failure.
 */
static isc_result_t
redirect(ns_client_t *client, dns_name_t *name, dns_rdataset_t *rdataset,
	 dns_dbnode_t **nodep, dns_db_t **dbp, dns_dbversion_t **versionp,
	 dns_rdatatype_t qtype) {
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

	if (client->view->redirect == NULL) {
		return (ISC_R_NOTFOUND);
	}

	found = dns_fixedname_initname(&fixed);
	dns_rdataset_init(&trdataset);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, &client->ecs, NULL);

	if (WANTDNSSEC(client) && dns_db_iszone(*dbp) && dns_db_issecure(*dbp))
	{
		return (ISC_R_NOTFOUND);
	}

	if (WANTDNSSEC(client) && dns_rdataset_isassociated(rdataset)) {
		if (rdataset->trust == dns_trust_secure) {
			return (ISC_R_NOTFOUND);
		}
		if (rdataset->trust == dns_trust_ultimate &&
		    (rdataset->type == dns_rdatatype_nsec ||
		     rdataset->type == dns_rdatatype_nsec3))
		{
			return (ISC_R_NOTFOUND);
		}
		if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) != 0) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset))
			{
				dns_ncache_current(rdataset, found, &trdataset);
				type = trdataset.type;
				dns_rdataset_disassociate(&trdataset);
				if (type == dns_rdatatype_nsec ||
				    type == dns_rdatatype_nsec3 ||
				    type == dns_rdatatype_rrsig)
				{
					return (ISC_R_NOTFOUND);
				}
			}
		}
	}

	result = ns_client_checkaclsilent(
		client, NULL, dns_zone_getqueryacl(client->view->redirect),
		true);
	if (result != ISC_R_SUCCESS) {
		return (ISC_R_NOTFOUND);
	}

	result = dns_zone_getdb(client->view->redirect, &db);
	if (result != ISC_R_SUCCESS) {
		return (ISC_R_NOTFOUND);
	}

	dbversion = ns_client_findversion(client, db);
	if (dbversion == NULL) {
		dns_db_detach(&db);
		return (ISC_R_NOTFOUND);
	}

	/*
	 * Lookup the requested data in the redirect zone.
	 */
	result = dns_db_findext(db, client->query.qname, dbversion->version,
				qtype, DNS_DBFIND_NOZONECUT, client->now, &node,
				found, &cm, &ci, &trdataset, NULL);
	if (result == DNS_R_NXRRSET || result == DNS_R_NCACHENXRRSET) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		if (dns_rdataset_isassociated(&trdataset)) {
			dns_rdataset_disassociate(&trdataset);
		}
		goto nxrrset;
	} else if (result != ISC_R_SUCCESS) {
		if (dns_rdataset_isassociated(&trdataset)) {
			dns_rdataset_disassociate(&trdataset);
		}
		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}
		dns_db_detach(&db);
		return (ISC_R_NOTFOUND);
	}

	CTRACE(ISC_LOG_DEBUG(3), "redirect: found data: done");
	dns_name_copynf(found, name);
	if (dns_rdataset_isassociated(rdataset)) {
		dns_rdataset_disassociate(rdataset);
	}
	if (dns_rdataset_isassociated(&trdataset)) {
		dns_rdataset_clone(&trdataset, rdataset);
		dns_rdataset_disassociate(&trdataset);
	}
nxrrset:
	if (*nodep != NULL) {
		dns_db_detachnode(*dbp, nodep);
	}
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
	  dns_rdatatype_t qtype, bool *is_zonep) {
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
	bool is_zone;
	unsigned int labels;
	unsigned int options;

	CTRACE(ISC_LOG_DEBUG(3), "redirect2");

	if (client->view->redirectzone == NULL) {
		return (ISC_R_NOTFOUND);
	}

	if (dns_name_issubdomain(name, client->view->redirectzone)) {
		return (ISC_R_NOTFOUND);
	}

	found = dns_fixedname_initname(&fixed);
	dns_rdataset_init(&trdataset);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, &client->ecs, NULL);

	if (WANTDNSSEC(client) && dns_db_iszone(*dbp) && dns_db_issecure(*dbp))
	{
		return (ISC_R_NOTFOUND);
	}

	if (WANTDNSSEC(client) && dns_rdataset_isassociated(rdataset)) {
		if (rdataset->trust == dns_trust_secure) {
			return (ISC_R_NOTFOUND);
		}
		if (rdataset->trust == dns_trust_ultimate &&
		    (rdataset->type == dns_rdatatype_nsec ||
		     rdataset->type == dns_rdatatype_nsec3))
		{
			return (ISC_R_NOTFOUND);
		}
		if ((rdataset->attributes & DNS_RDATASETATTR_NEGATIVE) != 0) {
			for (result = dns_rdataset_first(rdataset);
			     result == ISC_R_SUCCESS;
			     result = dns_rdataset_next(rdataset))
			{
				dns_ncache_current(rdataset, found, &trdataset);
				type = trdataset.type;
				dns_rdataset_disassociate(&trdataset);
				if (type == dns_rdatatype_nsec ||
				    type == dns_rdatatype_nsec3 ||
				    type == dns_rdatatype_rrsig)
				{
					return (ISC_R_NOTFOUND);
				}
			}
		}
	}

	redirectname = dns_fixedname_initname(&fixedredirect);
	labels = dns_name_countlabels(client->query.qname);
	if (labels > 1U) {
		dns_name_t prefix;

		dns_name_init(&prefix, NULL);
		dns_name_getlabelsequence(client->query.qname, 0, labels - 1,
					  &prefix);
		result = dns_name_concatenate(&prefix,
					      client->view->redirectzone,
					      redirectname, NULL);
		if (result != ISC_R_SUCCESS) {
			return (ISC_R_NOTFOUND);
		}
	} else {
		dns_name_copynf(redirectname, client->view->redirectzone);
	}

	options = 0;
	result = query_getdb(client, redirectname, qtype, options, &zone, &db,
			     &version, &is_zone);
	if (result != ISC_R_SUCCESS) {
		return (ISC_R_NOTFOUND);
	}
	if (zone != NULL) {
		dns_zone_detach(&zone);
	}

	/*
	 * Lookup the requested data in the redirect zone.
	 */
	result = dns_db_findext(db, redirectname, version, qtype, 0,
				client->now, &node, found, &cm, &ci, &trdataset,
				NULL);
	if (result == DNS_R_NXRRSET || result == DNS_R_NCACHENXRRSET) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		if (dns_rdataset_isassociated(&trdataset)) {
			dns_rdataset_disassociate(&trdataset);
		}
		goto nxrrset;
	} else if (result == ISC_R_NOTFOUND || result == DNS_R_DELEGATION) {
		/*
		 * Cleanup.
		 */
		if (dns_rdataset_isassociated(&trdataset)) {
			dns_rdataset_disassociate(&trdataset);
		}
		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}
		dns_db_detach(&db);
		/*
		 * Don't loop forever if the lookup failed last time.
		 */
		if (!REDIRECT(client)) {
			result = ns_query_recurse(client, qtype, redirectname,
						  NULL, NULL, true);
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
		if (dns_rdataset_isassociated(&trdataset)) {
			dns_rdataset_disassociate(&trdataset);
		}
		if (node != NULL) {
			dns_db_detachnode(db, &node);
		}
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

	dns_name_copynf(found, name);
	if (dns_rdataset_isassociated(rdataset)) {
		dns_rdataset_disassociate(rdataset);
	}
	if (dns_rdataset_isassociated(&trdataset)) {
		dns_rdataset_clone(&trdataset, rdataset);
		dns_rdataset_disassociate(&trdataset);
	}
nxrrset:
	if (*nodep != NULL) {
		dns_db_detachnode(*dbp, nodep);
	}
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
 *
 * Whenever this function is called, qctx_destroy() must be called
 * when leaving the scope or freeing the qctx.
 */
static void
qctx_init(ns_client_t *client, dns_fetchevent_t **eventp, dns_rdatatype_t qtype,
	  query_ctx_t *qctx) {
	REQUIRE(qctx != NULL);
	REQUIRE(client != NULL);

	memset(qctx, 0, sizeof(*qctx));

	/* Set this first so CCTRACE will work */
	qctx->client = client;

	dns_view_attach(client->view, &qctx->view);

	CCTRACE(ISC_LOG_DEBUG(3), "qctx_init");

	if (eventp != NULL) {
		qctx->event = *eventp;
		*eventp = NULL;
	} else {
		qctx->event = NULL;
	}
	qctx->qtype = qctx->type = qtype;
	qctx->result = ISC_R_SUCCESS;
	qctx->findcoveringnsec = qctx->view->synthfromdnssec;

	/*
	 * If it's an RRSIG or SIG query, we'll iterate the node.
	 */
	if (qctx->qtype == dns_rdatatype_rrsig ||
	    qctx->qtype == dns_rdatatype_sig)
	{
		qctx->type = dns_rdatatype_any;
	}

	CALL_HOOK_NORETURN(NS_QUERY_QCTX_INITIALIZED, qctx);
}

/*
 * Make 'dst' and exact copy of 'src', with exception of the
 * option field, which is reset to zero.
 * This function also attaches dst's view and db to the src's
 * view and cachedb.
 */
static void
qctx_copy(const query_ctx_t *qctx, query_ctx_t *dst) {
	REQUIRE(qctx != NULL);
	REQUIRE(dst != NULL);

	memmove(dst, qctx, sizeof(*dst));
	dst->view = NULL;
	dst->db = NULL;
	dst->options = 0;
	dns_view_attach(qctx->view, &dst->view);
	dns_db_attach(qctx->view->cachedb, &dst->db);
	CCTRACE(ISC_LOG_DEBUG(3), "qctx_copy");
}

/*%
 * Clean up and disassociate the rdataset and node pointers in qctx.
 */
static void
qctx_clean(query_ctx_t *qctx) {
	if (qctx->rdataset != NULL && dns_rdataset_isassociated(qctx->rdataset))
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
		ns_client_putrdataset(qctx->client, &qctx->rdataset);
	}

	if (qctx->sigrdataset != NULL) {
		ns_client_putrdataset(qctx->client, &qctx->sigrdataset);
	}

	if (qctx->fname != NULL) {
		ns_client_releasename(qctx->client, &qctx->fname);
	}

	if (qctx->db != NULL) {
		INSIST(qctx->node == NULL);
		dns_db_detach(&qctx->db);
	}

	if (qctx->zone != NULL) {
		dns_zone_detach(&qctx->zone);
	}

	if (qctx->zdb != NULL) {
		ns_client_putrdataset(qctx->client, &qctx->zsigrdataset);
		ns_client_putrdataset(qctx->client, &qctx->zrdataset);
		ns_client_releasename(qctx->client, &qctx->zfname);
		dns_db_detachnode(qctx->zdb, &qctx->znode);
		dns_db_detach(&qctx->zdb);
	}

	if (qctx->event != NULL && !qctx->client->nodetach) {
		free_devent(qctx->client, ISC_EVENT_PTR(&qctx->event),
			    &qctx->event);
	}
}

static void
qctx_destroy(query_ctx_t *qctx) {
	CALL_HOOK_NORETURN(NS_QUERY_QCTX_DESTROYED, qctx);

	dns_view_detach(&qctx->view);
}

/*%
 * Log detailed information about the query immediately after
 * the client request or a return from recursion.
 */
static void
query_trace(query_ctx_t *qctx) {
#ifdef WANT_QUERYTRACE
	char mbuf[2 * DNS_NAME_FORMATSIZE];
	char qbuf[DNS_NAME_FORMATSIZE];

	if (qctx->client->query.origqname != NULL) {
		dns_name_format(qctx->client->query.origqname, qbuf,
				sizeof(qbuf));
	} else {
		snprintf(qbuf, sizeof(qbuf), "<unset>");
	}

	snprintf(mbuf, sizeof(mbuf) - 1,
		 "client attr:0x%x, query attr:0x%X, restarts:%u, "
		 "origqname:%s, timer:%d, authdb:%d, referral:%d",
		 qctx->client->attributes, qctx->client->query.attributes,
		 qctx->client->query.restarts, qbuf,
		 (int)qctx->client->query.timerset,
		 (int)qctx->client->query.authdbset,
		 (int)qctx->client->query.isreferral);
	CCTRACE(ISC_LOG_DEBUG(3), mbuf);
#else  /* ifdef WANT_QUERYTRACE */
	UNUSED(qctx);
#endif /* ifdef WANT_QUERYTRACE */
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

	CALL_HOOK(NS_QUERY_SETUP, &qctx);

	/*
	 * Check SERVFAIL cache
	 */
	result = ns__query_sfcache(&qctx);
	if (result != ISC_R_COMPLETE) {
		qctx_destroy(&qctx);
		return (result);
	}

	result = ns__query_start(&qctx);

cleanup:
	qctx_destroy(&qctx);
	return (result);
}

static bool
get_root_key_sentinel_id(query_ctx_t *qctx, const char *ndata) {
	unsigned int v = 0;
	int i;

	for (i = 0; i < 5; i++) {
		if (!isdigit((unsigned char)ndata[i])) {
			return (false);
		}
		v *= 10;
		v += ndata[i] - '0';
	}
	if (v > 65535U) {
		return (false);
	}
	qctx->client->query.root_key_sentinel_keyid = v;
	return (true);
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
		qctx->client->query.root_key_sentinel_is_ta = true;
		/*
		 * Simplify processing by disabling aggressive
		 * negative caching.
		 */
		qctx->findcoveringnsec = false;
		ns_client_log(qctx->client, NS_LOGCATEGORY_TAT,
			      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
			      "root-key-sentinel-is-ta query label found");
	} else if (qctx->client->query.qname->length > 31 && ndata[0] == 30 &&
		   strncasecmp(ndata + 1, "root-key-sentinel-not-ta-", 25) == 0)
	{
		if (!get_root_key_sentinel_id(qctx, ndata + 26)) {
			return;
		}
		qctx->client->query.root_key_sentinel_not_ta = true;
		/*
		 * Simplify processing by disabling aggressive
		 * negative caching.
		 */
		qctx->findcoveringnsec = false;
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
	qctx->want_restart = false;
	qctx->authoritative = false;
	qctx->version = NULL;
	qctx->zversion = NULL;
	qctx->need_wildcardproof = false;
	qctx->rpz = false;

	CALL_HOOK(NS_QUERY_START_BEGIN, qctx);

	/*
	 * If we require a server cookie then send back BADCOOKIE
	 * before we have done too much work.
	 */
	if (!TCP(qctx->client) && qctx->view->requireservercookie &&
	    WANTCOOKIE(qctx->client) && !HAVECOOKIE(qctx->client))
	{
		qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AA;
		qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AD;
		qctx->client->message->rcode = dns_rcode_badcookie;
		return (ns_query_done(qctx));
	}

	if (qctx->view->checknames &&
	    !dns_rdata_checkowner(qctx->client->query.qname,
				  qctx->client->message->rdclass, qctx->qtype,
				  false))
	{
		char namebuf[DNS_NAME_FORMATSIZE];
		char typebuf[DNS_RDATATYPE_FORMATSIZE];
		char classbuf[DNS_RDATACLASS_FORMATSIZE];

		dns_name_format(qctx->client->query.qname, namebuf,
				sizeof(namebuf));
		dns_rdatatype_format(qctx->qtype, typebuf, sizeof(typebuf));
		dns_rdataclass_format(qctx->client->message->rdclass, classbuf,
				      sizeof(classbuf));
		ns_client_log(qctx->client, DNS_LOGCATEGORY_SECURITY,
			      NS_LOGMODULE_QUERY, ISC_LOG_ERROR,
			      "check-names failure %s/%s/%s", namebuf, typebuf,
			      classbuf);
		QUERY_ERROR(qctx, DNS_R_REFUSED);
		return (ns_query_done(qctx));
	}

	/*
	 * Setup for root key sentinel processing.
	 */
	if (qctx->view->root_key_sentinel &&
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
			     qctx->qtype, qctx->options, &qctx->zone, &qctx->db,
			     &qctx->version, &qctx->is_zone);
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

		tresult = query_getzonedb(
			qctx->client, qctx->client->query.qname, qctx->qtype,
			DNS_GETDB_PARTIAL, &tzone, &tdb, &tversion);
		if (tresult == ISC_R_SUCCESS) {
			/*
			 * We are authoritative for QNAME.  Attach the relevant
			 * zone to query context, set result to ISC_R_SUCCESS.
			 */
			qctx->options &= ~DNS_GETDB_NOEXACT;
			ns_client_putrdataset(qctx->client, &qctx->rdataset);
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
			qctx->is_zone = true;
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
			CCTRACE(ISC_LOG_ERROR, "ns__query_start: query_getdb "
					       "failed");
			QUERY_ERROR(qctx, result);
		}
		return (ns_query_done(qctx));
	}

	/*
	 * We found a database from which we can answer the query.  Update
	 * relevant query context flags if the answer is to be prepared using
	 * authoritative data.
	 */
	qctx->is_staticstub_zone = false;
	if (qctx->is_zone) {
		qctx->authoritative = true;
		if (qctx->zone != NULL) {
			if (dns_zone_gettype(qctx->zone) == dns_zone_mirror) {
				qctx->authoritative = false;
			}
			if (dns_zone_gettype(qctx->zone) == dns_zone_staticstub)
			{
				qctx->is_staticstub_zone = true;
			}
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
		qctx->client->query.authdbset = true;

		/* Track TCP vs UDP stats per zone */
		if (TCP(qctx->client)) {
			inc_stats(qctx->client, ns_statscounter_tcp);
		} else {
			inc_stats(qctx->client, ns_statscounter_udp);
		}
	}

	if (!qctx->is_zone && (qctx->view->staleanswerclienttimeout == 0) &&
	    dns_view_staleanswerenabled(qctx->view))
	{
		/*
		 * If stale answers are enabled and
		 * stale-answer-client-timeout is zero, then we can promptly
		 * answer with a stale RRset if one is available in cache.
		 */
		qctx->options |= DNS_GETDB_STALEFIRST;
	}

	result = query_lookup(qctx);

	/*
	 * Clear "look-also-for-stale-data" flag.
	 * If a fetch is created to resolve this query, then,
	 * when it completes, this option is not expected to be set.
	 */
	qctx->options &= ~DNS_GETDB_STALEFIRST;

cleanup:
	return (result);
}

/*
 * Allocate buffers in 'qctx' used to store query results.
 *
 * 'buffer' must be a pointer to an object whose lifetime
 * doesn't expire while 'qctx' is in use.
 */
static isc_result_t
qctx_prepare_buffers(query_ctx_t *qctx, isc_buffer_t *buffer) {
	REQUIRE(qctx != NULL);
	REQUIRE(qctx->client != NULL);
	REQUIRE(buffer != NULL);

	qctx->dbuf = ns_client_getnamebuf(qctx->client);
	if (ISC_UNLIKELY(qctx->dbuf == NULL)) {
		CCTRACE(ISC_LOG_ERROR,
			"qctx_prepare_buffers: ns_client_getnamebuf "
			"failed");
		return (ISC_R_NOMEMORY);
	}

	qctx->fname = ns_client_newname(qctx->client, qctx->dbuf, buffer);
	if (ISC_UNLIKELY(qctx->fname == NULL)) {
		CCTRACE(ISC_LOG_ERROR,
			"qctx_prepare_buffers: ns_client_newname failed");

		return (ISC_R_NOMEMORY);
	}

	qctx->rdataset = ns_client_newrdataset(qctx->client);
	if (ISC_UNLIKELY(qctx->rdataset == NULL)) {
		CCTRACE(ISC_LOG_ERROR,
			"qctx_prepare_buffers: ns_client_newrdataset failed");
		goto error;
	}

	if ((WANTDNSSEC(qctx->client) || qctx->findcoveringnsec) &&
	    (!qctx->is_zone || dns_db_issecure(qctx->db)))
	{
		qctx->sigrdataset = ns_client_newrdataset(qctx->client);
		if (qctx->sigrdataset == NULL) {
			CCTRACE(ISC_LOG_ERROR,
				"qctx_prepare_buffers: "
				"ns_client_newrdataset failed (2)");
			goto error;
		}
	}

	return (ISC_R_SUCCESS);

error:
	if (qctx->fname != NULL) {
		ns_client_releasename(qctx->client, &qctx->fname);
	}
	if (qctx->rdataset != NULL) {
		ns_client_putrdataset(qctx->client, &qctx->rdataset);
	}

	return (ISC_R_NOMEMORY);
}

/*
 * Setup a new query context for resolving a query.
 *
 * This function is only called if both these conditions are met:
 *    1. BIND is configured with stale-answer-client-timeout 0.
 *    2. A stale RRset is found in cache during initial query
 *       database lookup.
 *
 * We continue with this function for refreshing/resolving an RRset
 * after answering a client with stale data.
 */
static void
query_refresh_rrset(query_ctx_t *orig_qctx) {
	isc_buffer_t buffer;
	query_ctx_t qctx;

	REQUIRE(orig_qctx != NULL);
	REQUIRE(orig_qctx->client != NULL);

	qctx_copy(orig_qctx, &qctx);
	qctx.client->query.dboptions &= ~(DNS_DBFIND_STALETIMEOUT |
					  DNS_DBFIND_STALEOK |
					  DNS_DBFIND_STALEENABLED);

	/*
	 * We'll need some resources...
	 */
	if (qctx_prepare_buffers(&qctx, &buffer) != ISC_R_SUCCESS) {
		dns_db_detach(&qctx.db);
		qctx_destroy(&qctx);
		return;
	}

	/*
	 * Pretend we didn't find anything in cache.
	 */
	(void)query_gotanswer(&qctx, ISC_R_NOTFOUND);

	if (qctx.fname != NULL) {
		ns_client_releasename(qctx.client, &qctx.fname);
	}
	if (qctx.rdataset != NULL) {
		ns_client_putrdataset(qctx.client, &qctx.rdataset);
	}

	qctx_destroy(&qctx);
}

/*%
 * Perform a local database lookup, in either an authoritative or
 * cache database. If unable to answer, call ns_query_done(); otherwise
 * hand off processing to query_gotanswer().
 */
static isc_result_t
query_lookup(query_ctx_t *qctx) {
	isc_buffer_t buffer;
	isc_result_t result = ISC_R_UNSET;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;
	dns_name_t *rpzqname = NULL;
	char namebuf[DNS_NAME_FORMATSIZE];
	unsigned int dboptions;
	dns_ttl_t stale_refresh = 0;
	bool dbfind_stale = false;
	bool stale_timeout = false;
	bool answer_found = false;
	bool stale_found = false;
	bool stale_refresh_window = false;

	CCTRACE(ISC_LOG_DEBUG(3), "query_lookup");

	CALL_HOOK(NS_QUERY_LOOKUP_BEGIN, qctx);

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, qctx->client,
			    HAVEECS(qctx->client) ? &qctx->client->ecs : NULL,
			    NULL);

	/*
	 * We'll need some resources...
	 */
	result = qctx_prepare_buffers(qctx, &buffer);
	if (result != ISC_R_SUCCESS) {
		QUERY_ERROR(qctx, result);
		return (ns_query_done(qctx));
	}

	/*
	 * Now look for an answer in the database.
	 */
	if (qctx->dns64 && qctx->rpz) {
		rpzqname = qctx->client->query.rpz_st->p_name;
	} else {
		rpzqname = qctx->client->query.qname;
	}

	if ((qctx->options & DNS_GETDB_STALEFIRST) != 0) {
		/*
		 * If DNS_GETDB_STALEFIRST is set, it means that a stale
		 * RRset may be returned as part of this lookup. An attempt
		 * to refresh the RRset will still take place if an
		 * active RRset is not available.
		 */
		qctx->client->query.dboptions |= DNS_DBFIND_STALETIMEOUT;
	}

	dboptions = qctx->client->query.dboptions;
	if (!qctx->is_zone && qctx->findcoveringnsec &&
	    (qctx->type != dns_rdatatype_null || !dns_name_istat(rpzqname)))
	{
		dboptions |= DNS_DBFIND_COVERINGNSEC;
	}

	(void)dns_db_getservestalerefresh(qctx->client->view->cachedb,
					  &stale_refresh);
	if (stale_refresh > 0 &&
	    dns_view_staleanswerenabled(qctx->client->view))
	{
		dboptions |= DNS_DBFIND_STALEENABLED;
	}

	result = dns_db_findext(qctx->db, rpzqname, qctx->version, qctx->type,
				dboptions, qctx->client->now, &qctx->node,
				qctx->fname, &cm, &ci, qctx->rdataset,
				qctx->sigrdataset);

	/*
	 * Fixup fname and sigrdataset.
	 */
	if (qctx->dns64 && qctx->rpz) {
		dns_name_copynf(qctx->client->query.qname, qctx->fname);
		if (qctx->sigrdataset != NULL &&
		    dns_rdataset_isassociated(qctx->sigrdataset))
		{
			dns_rdataset_disassociate(qctx->sigrdataset);
		}
	}

	if (!qctx->is_zone) {
		dns_cache_updatestats(qctx->view->cache, result);
	}

	/*
	 * If DNS_DBFIND_STALEOK is set this means we are dealing with a
	 * lookup following a failed lookup and it is okay to serve a stale
	 * answer. This will (re)start the 'stale-refresh-time' window in
	 * rbtdb, tracking the last time the RRset lookup failed.
	 */
	dbfind_stale = ((dboptions & DNS_DBFIND_STALEOK) != 0);

	/*
	 * If DNS_DBFIND_STALEENABLED is set, this may be a normal lookup, but
	 * we are allowed to immediately respond with a stale answer if the
	 * request is within the 'stale-refresh-time' window.
	 */
	stale_refresh_window = (STALE_WINDOW(qctx->rdataset) &&
				(dboptions & DNS_DBFIND_STALEENABLED) != 0);

	/*
	 * If DNS_DBFIND_STALETIMEOUT is set, a stale answer is requested.
	 * This can happen if 'stale-answer-client-timeout' is enabled.
	 *
	 * If 'stale-answer-client-timeout' is set to 0, and a stale
	 * answer is found, send it to the client, and try to refresh the
	 * RRset.
	 *
	 * If 'stale-answer-client-timeout' is non-zero, and a stale
	 * answer is found, send it to the client. Don't try to refresh the
	 * RRset because a fetch is already in progress.
	 */
	stale_timeout = ((dboptions & DNS_DBFIND_STALETIMEOUT) != 0);

	if (dns_rdataset_isassociated(qctx->rdataset) &&
	    dns_rdataset_count(qctx->rdataset) > 0 && !STALE(qctx->rdataset))
	{
		/* Found non-stale usable rdataset. */
		answer_found = true;
		goto gotanswer;
	}

	if (dbfind_stale || stale_refresh_window || stale_timeout) {
		dns_name_format(qctx->client->query.qname, namebuf,
				sizeof(namebuf));

		inc_stats(qctx->client, ns_statscounter_trystale);

		if (dns_rdataset_isassociated(qctx->rdataset) &&
		    dns_rdataset_count(qctx->rdataset) > 0 &&
		    STALE(qctx->rdataset))
		{
			qctx->rdataset->ttl = qctx->view->staleanswerttl;
			stale_found = true;
			inc_stats(qctx->client, ns_statscounter_usedstale);
		} else {
			stale_found = false;
		}
	}

	if (dbfind_stale) {
		isc_log_write(ns_lctx, NS_LOGCATEGORY_SERVE_STALE,
			      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
			      "%s resolver failure, stale answer %s", namebuf,
			      stale_found ? "used" : "unavailable");
		if (!stale_found) {
			/*
			 * Resolver failure, no stale data, nothing more we
			 * can do, return SERVFAIL.
			 */
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
			return (ns_query_done(qctx));
		}
	} else if (stale_refresh_window) {
		/*
		 * A recent lookup failed, so during this time window we are
		 * allowed to return stale data immediately.
		 */
		isc_log_write(ns_lctx, NS_LOGCATEGORY_SERVE_STALE,
			      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
			      "%s query within stale refresh time, stale "
			      "answer %s",
			      namebuf, stale_found ? "used" : "unavailable");

		if (!stale_found) {
			/*
			 * During the stale refresh window explicitly do not try
			 * to refresh the data, because a recent lookup failed.
			 */
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
			return (ns_query_done(qctx));
		}
	} else if (stale_timeout) {
		if ((qctx->options & DNS_GETDB_STALEFIRST) != 0) {
			if (!stale_found) {
				/*
				 * We have nothing useful in cache to return
				 * immediately.
				 */
				qctx_clean(qctx);
				qctx_freedata(qctx);
				dns_db_attach(qctx->client->view->cachedb,
					      &qctx->db);
				qctx->client->query.dboptions &=
					~DNS_DBFIND_STALETIMEOUT;
				qctx->options &= ~DNS_GETDB_STALEFIRST;
				if (qctx->client->query.fetch != NULL) {
					dns_resolver_destroyfetch(
						&qctx->client->query.fetch);
				}
				return (query_lookup(qctx));
			} else {
				/*
				 * Immediately return the stale answer, start a
				 * resolver fetch to refresh the data in cache.
				 */
				isc_log_write(
					ns_lctx, NS_LOGCATEGORY_SERVE_STALE,
					NS_LOGMODULE_QUERY, ISC_LOG_INFO,
					"%s stale answer used, an attempt to "
					"refresh the RRset will still be made",
					namebuf);
				qctx->refresh_rrset = STALE(qctx->rdataset);
			}
		} else {
			/*
			 * The 'stale-answer-client-timeout' triggered, return
			 * the stale answer if available, otherwise wait until
			 * the resolver finishes.
			 */
			isc_log_write(ns_lctx, NS_LOGCATEGORY_SERVE_STALE,
				      NS_LOGMODULE_QUERY, ISC_LOG_INFO,
				      "%s client timeout, stale answer %s",
				      namebuf,
				      stale_found ? "used" : "unavailable");
			if (!stale_found) {
				return (result);
			}

			/*
			 * There still might be real answer later. Mark the
			 * query so we'll know we can skip answering.
			 */
			qctx->client->query.attributes |=
				NS_QUERYATTR_STALEPENDING;
		}
	}

gotanswer:
	if (stale_timeout && (answer_found || stale_found)) {
		/*
		 * Mark RRsets that we are adding to the client message on a
		 * lookup during 'stale-answer-client-timeout', so we can
		 * clean it up if needed when we resume from recursion.
		 */
		qctx->client->query.attributes |= NS_QUERYATTR_STALEOK;
		qctx->rdataset->attributes |= DNS_RDATASETATTR_STALE_ADDED;
	}

	result = query_gotanswer(qctx, result);

cleanup:
	return (result);
}

/*
 * Clear all rdatasets from the message that are in the given section and
 * that have the 'attr' attribute set.
 */
static void
message_clearrdataset(dns_message_t *msg, unsigned int attr) {
	unsigned int i;
	dns_name_t *name, *next_name;
	dns_rdataset_t *rds, *next_rds;

	/*
	 * Clean up name lists by calling the rdataset disassociate function.
	 */
	for (i = DNS_SECTION_ANSWER; i < DNS_SECTION_MAX; i++) {
		name = ISC_LIST_HEAD(msg->sections[i]);
		while (name != NULL) {
			next_name = ISC_LIST_NEXT(name, link);

			rds = ISC_LIST_HEAD(name->list);
			while (rds != NULL) {
				next_rds = ISC_LIST_NEXT(rds, link);
				if ((rds->attributes & attr) != attr) {
					rds = next_rds;
					continue;
				}
				ISC_LIST_UNLINK(name->list, rds, link);
				INSIST(dns_rdataset_isassociated(rds));
				dns_rdataset_disassociate(rds);
				isc_mempool_put(msg->rdspool, rds);
				rds = next_rds;
			}

			if (ISC_LIST_EMPTY(name->list)) {
				ISC_LIST_UNLINK(msg->sections[i], name, link);
				if (dns_name_dynamic(name)) {
					dns_name_free(name, msg->mctx);
				}
				isc_mempool_put(msg->namepool, name);
			}

			name = next_name;
		}
	}
}

/*
 * Clear any rdatasets from the client's message that were added on a lookup
 * due to a client timeout.
 */
static void
query_clear_stale(ns_client_t *client) {
	message_clearrdataset(client->message, DNS_RDATASETATTR_STALE_ADDED);
}

/*
 * Create a new query context with the sole intent of looking up for a stale
 * RRset in cache. If an entry is found, we mark the original query as
 * answered, in order to avoid answering the query twice, when the original
 * fetch finishes.
 */
static void
query_lookup_stale(ns_client_t *client) {
	query_ctx_t qctx;

	qctx_init(client, NULL, client->query.qtype, &qctx);
	dns_db_attach(client->view->cachedb, &qctx.db);
	client->query.attributes &= ~NS_QUERYATTR_RECURSIONOK;
	client->query.dboptions |= DNS_DBFIND_STALETIMEOUT;
	client->nodetach = true;
	(void)query_lookup(&qctx);
	if (qctx.node != NULL) {
		dns_db_detachnode(qctx.db, &qctx.node);
	}
	qctx_freedata(&qctx);
	qctx_destroy(&qctx);
}

/*
 * Event handler to resume processing a query after recursion, or when a
 * client timeout is triggered. If the query has timed out or been cancelled
 * or the system is shutting down, clean up and exit. If a client timeout is
 * triggered, see if we can respond with a stale answer from cache. Otherwise,
 * call query_resume() to continue the ongoing work.
 */
static void
fetch_callback(isc_task_t *task, isc_event_t *event) {
	dns_fetchevent_t *devent = (dns_fetchevent_t *)event;
	dns_fetch_t *fetch = NULL;
	ns_client_t *client = NULL;
	bool fetch_canceled = false;
	bool fetch_answered = false;
	bool client_shuttingdown = false;
	isc_logcategory_t *logcategory = NS_LOGCATEGORY_QUERY_ERRORS;
	isc_result_t result;
	int errorloglevel;
	query_ctx_t qctx;

	UNUSED(task);

	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE ||
		event->ev_type == DNS_EVENT_TRYSTALE);

	client = devent->ev_arg;

	REQUIRE(NS_CLIENT_VALID(client));
	REQUIRE(task == client->task);
	REQUIRE(RECURSING(client));

	CTRACE(ISC_LOG_DEBUG(3), "fetch_callback");

	if (event->ev_type == DNS_EVENT_TRYSTALE) {
		if (devent->result != ISC_R_CANCELED) {
			query_lookup_stale(client);
		}
		isc_event_free(ISC_EVENT_PTR(&event));
		return;
	}
	/*
	 * We are resuming from recursion. Reset any attributes, options
	 * that a lookup due to stale-answer-client-timeout may have set.
	 */
	if (client->view->cachedb != NULL && client->view->recursion) {
		client->query.attributes |= NS_QUERYATTR_RECURSIONOK;
	}
	client->query.fetchoptions &= ~DNS_FETCHOPT_TRYSTALE_ONTIMEOUT;
	client->query.dboptions &= ~DNS_DBFIND_STALETIMEOUT;
	client->nodetach = false;

	LOCK(&client->query.fetchlock);
	INSIST(client->query.fetch == devent->fetch ||
	       client->query.fetch == NULL);
	if (QUERY_STALEPENDING(&client->query)) {
		/*
		 * We've gotten an authoritative answer to a query that
		 * was left pending after a stale timeout. We don't need
		 * to do anything with it; free all the data and go home.
		 */
		client->query.fetch = NULL;
		fetch_answered = true;
	} else if (client->query.fetch != NULL) {
		/*
		 * This is the fetch we've been waiting for.
		 */
		INSIST(devent->fetch == client->query.fetch);
		client->query.fetch = NULL;

		/*
		 * Update client->now.
		 */
		isc_stdtime_get(&client->now);
	} else {
		/*
		 * This is a fetch completion event for a canceled fetch.
		 * Clean up and don't resume the find.
		 */
		fetch_canceled = true;
	}
	UNLOCK(&client->query.fetchlock);

	SAVE(fetch, devent->fetch);

	/*
	 * We're done recursing, detach from quota and unlink from
	 * the manager's recursing-clients list.
	 */

	if (client->recursionquota != NULL) {
		isc_quota_detach(&client->recursionquota);
		ns_stats_decrement(client->sctx->nsstats,
				   ns_statscounter_recursclients);
	}

	LOCK(&client->manager->reclock);
	if (ISC_LINK_LINKED(client, rlink)) {
		ISC_LIST_UNLINK(client->manager->recursing, client, rlink);
	}
	UNLOCK(&client->manager->reclock);

	isc_nmhandle_detach(&client->fetchhandle);

	client->query.attributes &= ~NS_QUERYATTR_RECURSING;
	client->state = NS_CLIENTSTATE_WORKING;

	/*
	 * Initialize a new qctx and use it to either resume from
	 * recursion or clean up after cancelation.  Transfer
	 * ownership of devent to the new qctx in the process.
	 */
	qctx_init(client, &devent, 0, &qctx);

	client_shuttingdown = ns_client_shuttingdown(client);
	if (fetch_canceled || fetch_answered || client_shuttingdown) {
		/*
		 * We've timed out or are shutting down. We can now
		 * free the event and other resources held by qctx, but
		 * don't call qctx_destroy() yet: it might destroy the
		 * client, which we still need for a moment.
		 */
		qctx_freedata(&qctx);

		/*
		 * Return an error to the client, or just drop.
		 */
		if (fetch_canceled) {
			CTRACE(ISC_LOG_ERROR, "fetch cancelled");
			query_error(client, DNS_R_SERVFAIL, __LINE__);
		} else {
			query_next(client, ISC_R_CANCELED);
		}

		/*
		 * Free any persistent plugin data that was allocated to
		 * service the client, then detach the client object.
		 */
		qctx.detach_client = true;
		qctx_destroy(&qctx);
	} else {
		/*
		 * Resume the find process.
		 */
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
						      errorloglevel, false);
			}
		}

		qctx_destroy(&qctx);
	}

	dns_resolver_destroyfetch(&fetch);
}

/*%
 * Check whether the recursion parameters in 'param' match the current query's
 * recursion parameters provided in 'qtype', 'qname', and 'qdomain'.
 */
static bool
recparam_match(const ns_query_recparam_t *param, dns_rdatatype_t qtype,
	       const dns_name_t *qname, const dns_name_t *qdomain) {
	REQUIRE(param != NULL);

	return (param->qtype == qtype && param->qname != NULL &&
		qname != NULL && param->qdomain != NULL && qdomain != NULL &&
		dns_name_equal(param->qname, qname) &&
		dns_name_equal(param->qdomain, qdomain));
}

/*%
 * Update 'param' with current query's recursion parameters provided in
 * 'qtype', 'qname', and 'qdomain'.
 */
static void
recparam_update(ns_query_recparam_t *param, dns_rdatatype_t qtype,
		const dns_name_t *qname, const dns_name_t *qdomain) {
	REQUIRE(param != NULL);

	param->qtype = qtype;

	if (qname == NULL) {
		param->qname = NULL;
	} else {
		param->qname = dns_fixedname_initname(&param->fqname);
		dns_name_copynf(qname, param->qname);
	}

	if (qdomain == NULL) {
		param->qdomain = NULL;
	} else {
		param->qdomain = dns_fixedname_initname(&param->fqdomain);
		dns_name_copynf(qdomain, param->qdomain);
	}
}
static atomic_uint_fast32_t last_soft, last_hard;

isc_result_t
ns_query_recurse(ns_client_t *client, dns_rdatatype_t qtype, dns_name_t *qname,
		 dns_name_t *qdomain, dns_rdataset_t *nameservers,
		 bool resuming) {
	isc_result_t result;
	dns_rdataset_t *rdataset, *sigrdataset;
	isc_sockaddr_t *peeraddr = NULL;

	CTRACE(ISC_LOG_DEBUG(3), "ns_query_recurse");

	/*
	 * Check recursion parameters from the previous query to see if they
	 * match.  If not, update recursion parameters and proceed.
	 */
	if (recparam_match(&client->query.recparam, qtype, qname, qdomain)) {
		ns_client_log(client, NS_LOGCATEGORY_CLIENT, NS_LOGMODULE_QUERY,
			      ISC_LOG_INFO, "recursion loop detected");
		return (ISC_R_FAILURE);
	}

	recparam_update(&client->query.recparam, qtype, qname, qdomain);

	if (!resuming) {
		inc_stats(client, ns_statscounter_recursion);
	}

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
		if (result == ISC_R_SUCCESS || result == ISC_R_SOFTQUOTA) {
			ns_stats_increment(client->sctx->nsstats,
					   ns_statscounter_recursclients);
		}

		if (result == ISC_R_SOFTQUOTA) {
			isc_stdtime_t now;
			isc_stdtime_get(&now);
			if (now != atomic_load_relaxed(&last_soft)) {
				atomic_store_relaxed(&last_soft, now);
				ns_client_log(client, NS_LOGCATEGORY_CLIENT,
					      NS_LOGMODULE_QUERY,
					      ISC_LOG_WARNING,
					      "recursive-clients soft limit "
					      "exceeded (%u/%u/%u), "
					      "aborting oldest query",
					      isc_quota_getused(
						      client->recursionquota),
					      isc_quota_getsoft(
						      client->recursionquota),
					      isc_quota_getmax(
						      client->recursionquota));
			}
			ns_client_killoldestquery(client);
			result = ISC_R_SUCCESS;
		} else if (result == ISC_R_QUOTA) {
			isc_stdtime_t now;
			isc_stdtime_get(&now);
			if (now != atomic_load_relaxed(&last_hard)) {
				ns_server_t *sctx = client->sctx;
				atomic_store_relaxed(&last_hard, now);
				ns_client_log(
					client, NS_LOGCATEGORY_CLIENT,
					NS_LOGMODULE_QUERY, ISC_LOG_WARNING,
					"no more recursive clients "
					"(%u/%u/%u): %s",
					isc_quota_getused(
						&sctx->recursionquota),
					isc_quota_getsoft(
						&sctx->recursionquota),
					isc_quota_getmax(&sctx->recursionquota),
					isc_result_totext(result));
			}
			ns_client_killoldestquery(client);
		}
		if (result != ISC_R_SUCCESS) {
			return (result);
		}

		dns_message_clonebuffer(client->message);
		ns_client_recursing(client);
	}

	/*
	 * Invoke the resolver.
	 */
	REQUIRE(nameservers == NULL || nameservers->type == dns_rdatatype_ns);
	REQUIRE(client->query.fetch == NULL);

	rdataset = ns_client_newrdataset(client);
	if (rdataset == NULL) {
		return (ISC_R_NOMEMORY);
	}

	if (WANTDNSSEC(client)) {
		sigrdataset = ns_client_newrdataset(client);
		if (sigrdataset == NULL) {
			ns_client_putrdataset(client, &rdataset);
			return (ISC_R_NOMEMORY);
		}
	} else {
		sigrdataset = NULL;
	}

	if (!client->query.timerset) {
		ns_client_settimeout(client, 60);
	}

	if (!TCP(client)) {
		peeraddr = &client->peeraddr;
	}

	if (client->view->staleanswerclienttimeout > 0 &&
	    client->view->staleanswerclienttimeout != (uint32_t)-1 &&
	    dns_view_staleanswerenabled(client->view))
	{
		client->query.fetchoptions |= DNS_FETCHOPT_TRYSTALE_ONTIMEOUT;
	}

	isc_nmhandle_attach(client->handle, &client->fetchhandle);
	result = dns_resolver_createfetch(
		client->view->resolver, qname, qtype, qdomain, nameservers,
		NULL, peeraddr, client->message->id, client->query.fetchoptions,
		0, NULL, client->task, fetch_callback, client, rdataset,
		sigrdataset, &client->query.fetch);
	if (result != ISC_R_SUCCESS) {
		isc_nmhandle_detach(&client->fetchhandle);
		ns_client_putrdataset(client, &rdataset);
		if (sigrdataset != NULL) {
			ns_client_putrdataset(client, &sigrdataset);
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
	char mbuf[4 * DNS_NAME_FORMATSIZE];
	char qbuf[DNS_NAME_FORMATSIZE];
	char tbuf[DNS_RDATATYPE_FORMATSIZE];
#endif /* ifdef WANT_QUERYTRACE */

	CCTRACE(ISC_LOG_DEBUG(3), "query_resume");

	CALL_HOOK(NS_QUERY_RESUME_BEGIN, qctx);

	qctx->want_restart = false;

	qctx->rpz_st = qctx->client->query.rpz_st;
	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0)
	{
		CCTRACE(ISC_LOG_DEBUG(3), "resume from RPZ recursion");
#ifdef WANT_QUERYTRACE
		{
			char pbuf[DNS_NAME_FORMATSIZE] = "<unset>";
			char fbuf[DNS_NAME_FORMATSIZE] = "<unset>";
			if (qctx->rpz_st->r_name != NULL) {
				dns_name_format(qctx->rpz_st->r_name, qbuf,
						sizeof(qbuf));
			} else {
				snprintf(qbuf, sizeof(qbuf), "<unset>");
			}
			if (qctx->rpz_st->p_name != NULL) {
				dns_name_format(qctx->rpz_st->p_name, pbuf,
						sizeof(pbuf));
			}
			if (qctx->rpz_st->fname != NULL) {
				dns_name_format(qctx->rpz_st->fname, fbuf,
						sizeof(fbuf));
			}

			snprintf(mbuf, sizeof(mbuf) - 1,
				 "rpz rname:%s, pname:%s, qctx->fname:%s", qbuf,
				 pbuf, fbuf);
			CCTRACE(ISC_LOG_DEBUG(3), mbuf);
		}
#endif /* ifdef WANT_QUERYTRACE */

		qctx->is_zone = qctx->rpz_st->q.is_zone;
		qctx->authoritative = qctx->rpz_st->q.authoritative;
		RESTORE(qctx->zone, qctx->rpz_st->q.zone);
		RESTORE(qctx->node, qctx->rpz_st->q.node);
		RESTORE(qctx->db, qctx->rpz_st->q.db);
		RESTORE(qctx->rdataset, qctx->rpz_st->q.rdataset);
		RESTORE(qctx->sigrdataset, qctx->rpz_st->q.sigrdataset);
		qctx->qtype = qctx->rpz_st->q.qtype;

		if (qctx->event->node != NULL) {
			dns_db_detachnode(qctx->event->db, &qctx->event->node);
		}
		SAVE(qctx->rpz_st->r.db, qctx->event->db);
		qctx->rpz_st->r.r_type = qctx->event->qtype;
		SAVE(qctx->rpz_st->r.r_rdataset, qctx->event->rdataset);
		ns_client_putrdataset(qctx->client, &qctx->event->sigrdataset);
	} else if (REDIRECT(qctx->client)) {
		/*
		 * Restore saved state.
		 */
		CCTRACE(ISC_LOG_DEBUG(3), "resume from redirect recursion");
#ifdef WANT_QUERYTRACE
		dns_name_format(qctx->client->query.redirect.fname, qbuf,
				sizeof(qbuf));
		dns_rdatatype_format(qctx->client->query.redirect.qtype, tbuf,
				     sizeof(tbuf));
		snprintf(mbuf, sizeof(mbuf) - 1,
			 "redirect qctx->fname:%s, qtype:%s, auth:%d", qbuf,
			 tbuf, qctx->client->query.redirect.authoritative);
		CCTRACE(ISC_LOG_DEBUG(3), mbuf);
#endif /* ifdef WANT_QUERYTRACE */
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

		/*
		 * Free resources used while recursing.
		 */
		ns_client_putrdataset(qctx->client, &qctx->event->rdataset);
		ns_client_putrdataset(qctx->client, &qctx->event->sigrdataset);
		if (qctx->event->node != NULL) {
			dns_db_detachnode(qctx->event->db, &qctx->event->node);
		}
		if (qctx->event->db != NULL) {
			dns_db_detach(&qctx->event->db);
		}
	} else {
		CCTRACE(ISC_LOG_DEBUG(3), "resume from normal recursion");
		qctx->authoritative = false;

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

	CALL_HOOK(NS_QUERY_RESUME_RESTORED, qctx);

	if (DNS64(qctx->client)) {
		qctx->client->query.attributes &= ~NS_QUERYATTR_DNS64;
		qctx->dns64 = true;
	}

	if (DNS64EXCLUDE(qctx->client)) {
		qctx->client->query.attributes &= ~NS_QUERYATTR_DNS64EXCLUDE;
		qctx->dns64_exclude = true;
	}

	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0)
	{
		/*
		 * Has response policy changed out from under us?
		 */
		if (qctx->rpz_st->rpz_ver != qctx->view->rpzs->rpz_ver) {
			ns_client_log(qctx->client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_QUERY, DNS_RPZ_INFO_LEVEL,
				      "query_resume: RPZ settings "
				      "out of date "
				      "(rpz_ver %d, expected %d)",
				      qctx->view->rpzs->rpz_ver,
				      qctx->rpz_st->rpz_ver);
			QUERY_ERROR(qctx, DNS_R_SERVFAIL);
			return (ns_query_done(qctx));
		}
	}

	/*
	 * We'll need some resources...
	 */
	qctx->dbuf = ns_client_getnamebuf(qctx->client);
	if (qctx->dbuf == NULL) {
		CCTRACE(ISC_LOG_ERROR, "query_resume: ns_client_getnamebuf "
				       "failed (1)");
		QUERY_ERROR(qctx, ISC_R_NOMEMORY);
		return (ns_query_done(qctx));
	}

	qctx->fname = ns_client_newname(qctx->client, qctx->dbuf, &b);
	if (qctx->fname == NULL) {
		CCTRACE(ISC_LOG_ERROR, "query_resume: ns_client_newname failed "
				       "(1)");
		QUERY_ERROR(qctx, ISC_R_NOMEMORY);
		return (ns_query_done(qctx));
	}

	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0)
	{
		tname = qctx->rpz_st->fname;
	} else if (REDIRECT(qctx->client)) {
		tname = qctx->client->query.redirect.fname;
	} else {
		tname = dns_fixedname_name(&qctx->event->foundname);
	}

	dns_name_copynf(tname, qctx->fname);

	if (qctx->rpz_st != NULL &&
	    (qctx->rpz_st->state & DNS_RPZ_RECURSING) != 0)
	{
		qctx->rpz_st->r.r_result = qctx->event->result;
		result = qctx->rpz_st->q.result;
		free_devent(qctx->client, ISC_EVENT_PTR(&qctx->event),
			    &qctx->event);
	} else if (REDIRECT(qctx->client)) {
		result = qctx->client->query.redirect.result;
	} else {
		result = qctx->event->result;
	}

	qctx->resuming = true;

	return (query_gotanswer(qctx, result));

cleanup:
	return (result);
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
	bool failcache;
	uint32_t flags;

	/*
	 * The SERVFAIL cache doesn't apply to authoritative queries.
	 */
	if (!RECURSIONOK(qctx->client)) {
		return (ISC_R_COMPLETE);
	}

	flags = 0;
#ifdef ENABLE_AFL
	if (qctx->client->sctx->fuzztype == isc_fuzz_resolver) {
		failcache = false;
	} else {
		failcache = dns_badcache_find(
			qctx->view->failcache, qctx->client->query.qname,
			qctx->qtype, &flags, &qctx->client->tnow);
	}
#else  /* ifdef ENABLE_AFL */
	failcache = dns_badcache_find(qctx->view->failcache,
				      qctx->client->query.qname, qctx->qtype,
				      &flags, &qctx->client->tnow);
#endif /* ifdef ENABLE_AFL */
	if (failcache &&
	    (((flags & NS_FAILCACHE_CD) != 0) ||
	     ((qctx->client->message->flags & DNS_MESSAGEFLAG_CD) == 0)))
	{
		if (isc_log_wouldlog(ns_lctx, ISC_LOG_DEBUG(1))) {
			char namebuf[DNS_NAME_FORMATSIZE];
			char typebuf[DNS_RDATATYPE_FORMATSIZE];

			dns_name_format(qctx->client->query.qname, namebuf,
					sizeof(namebuf));
			dns_rdatatype_format(qctx->qtype, typebuf,
					     sizeof(typebuf));
			ns_client_log(qctx->client, NS_LOGCATEGORY_CLIENT,
				      NS_LOGMODULE_QUERY, ISC_LOG_DEBUG(1),
				      "servfail cache hit %s/%s (%s)", namebuf,
				      typebuf,
				      ((flags & NS_FAILCACHE_CD) != 0) ? "CD=1"
								       : "CD="
									 "0");
		}

		qctx->client->attributes |= NS_CLIENTATTR_NOSETFC;
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (ns_query_done(qctx));
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

	/*
	 * XXXMPA the rrl system tests fails sometimes and RRL_CHECKED
	 * is set when we are called the second time preventing the
	 * response being dropped.
	 */
	ns_client_log(
		qctx->client, DNS_LOGCATEGORY_RRL, NS_LOGMODULE_QUERY,
		ISC_LOG_DEBUG(99),
		"rrl=%p, HAVECOOKIE=%u, result=%s, "
		"fname=%p(%u), is_zone=%u, RECURSIONOK=%u, "
		"query.rpz_st=%p(%u), RRL_CHECKED=%u\n",
		qctx->client->view->rrl, HAVECOOKIE(qctx->client),
		isc_result_toid(result), qctx->fname,
		qctx->fname != NULL ? dns_name_isabsolute(qctx->fname) : 0,
		qctx->is_zone, RECURSIONOK(qctx->client),
		qctx->client->query.rpz_st,
		qctx->client->query.rpz_st != NULL
			? ((qctx->client->query.rpz_st->state &
			    DNS_RPZ_REWRITTEN) != 0)
			: 0,
		(qctx->client->query.attributes & NS_QUERYATTR_RRL_CHECKED) !=
			0);

	if (qctx->view->rrl != NULL && !HAVECOOKIE(qctx->client) &&
	    ((qctx->fname != NULL && dns_name_isabsolute(qctx->fname)) ||
	     (result == ISC_R_NOTFOUND && !RECURSIONOK(qctx->client))) &&
	    !(result == DNS_R_DELEGATION && !qctx->is_zone &&
	      RECURSIONOK(qctx->client)) &&
	    (qctx->client->query.rpz_st == NULL ||
	     (qctx->client->query.rpz_st->state & DNS_RPZ_REWRITTEN) == 0) &&
	    (qctx->client->query.attributes & NS_QUERYATTR_RRL_CHECKED) == 0)
	{
		dns_rdataset_t nc_rdataset;
		bool wouldlog;
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
			if (qctx->db != NULL) {
				constname = dns_db_origin(qctx->db);
			}
			resp_result = result;
		} else if (result == DNS_R_NCACHENXDOMAIN &&
			   qctx->rdataset != NULL &&
			   dns_rdataset_isassociated(qctx->rdataset) &&
			   (qctx->rdataset->attributes &
			    DNS_RDATASETATTR_NEGATIVE) != 0)
		{
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
		} else if (result == DNS_R_NXRRSET || result == DNS_R_EMPTYNAME)
		{
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

		rrl_result = dns_rrl(
			qctx->view, qctx->zone, &qctx->client->peeraddr,
			TCP(qctx->client), qctx->client->message->rdclass,
			qctx->qtype, constname, resp_result, qctx->client->now,
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
					      DNS_RRL_LOG_DROP, "%s", log_buf);
			}

			if (!qctx->view->rrl->log_only) {
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
							qctx->client->message
								->rcode =
								dns_rcode_nxdomain;
						}
					}
				}
				return (DNS_R_DROP);
			}
		}
	}

	return (ISC_R_SUCCESS);
}

/*%
 * Do any RPZ rewriting that may be needed for this query.
 */
static isc_result_t
query_checkrpz(query_ctx_t *qctx, isc_result_t result) {
	isc_result_t rresult;

	CCTRACE(ISC_LOG_DEBUG(3), "query_checkrpz");

	rresult = rpz_rewrite(qctx->client, qctx->qtype, result, qctx->resuming,
			      qctx->rdataset, qctx->sigrdataset);
	qctx->rpz_st = qctx->client->query.rpz_st;
	switch (rresult) {
	case ISC_R_SUCCESS:
		break;
	case ISC_R_NOTFOUND:
	case DNS_R_DISALLOWED:
		return (result);
	case DNS_R_DELEGATION:
		/*
		 * recursing for NS names or addresses,
		 * so save the main query state
		 */
		INSIST(!RECURSING(qctx->client));
		qctx->rpz_st->q.qtype = qctx->qtype;
		qctx->rpz_st->q.is_zone = qctx->is_zone;
		qctx->rpz_st->q.authoritative = qctx->authoritative;
		SAVE(qctx->rpz_st->q.zone, qctx->zone);
		SAVE(qctx->rpz_st->q.db, qctx->db);
		SAVE(qctx->rpz_st->q.node, qctx->node);
		SAVE(qctx->rpz_st->q.rdataset, qctx->rdataset);
		SAVE(qctx->rpz_st->q.sigrdataset, qctx->sigrdataset);
		dns_name_copynf(qctx->fname, qctx->rpz_st->fname);
		qctx->rpz_st->q.result = result;
		qctx->client->query.attributes |= NS_QUERYATTR_RECURSING;
		return (ISC_R_COMPLETE);
	default:
		QUERY_ERROR(qctx, rresult);
		return (ISC_R_COMPLETE);
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
		dns_name_copynf(qctx->client->query.qname, qctx->fname);
		rpz_clean(&qctx->zone, &qctx->db, &qctx->node, NULL);
		if (qctx->rpz_st->m.rdataset != NULL) {
			ns_client_putrdataset(qctx->client, &qctx->rdataset);
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
		if (qctx->rpz_st->m.rpz->addsoa) {
			bool override_ttl =
				dns_rdataset_isassociated(qctx->rdataset);
			rresult = query_addsoa(qctx, override_ttl,
					       DNS_SECTION_ADDITIONAL);
			if (rresult != ISC_R_SUCCESS) {
				QUERY_ERROR(qctx, result);
				return (ISC_R_COMPLETE);
			}
		}

		switch (qctx->rpz_st->m.policy) {
		case DNS_RPZ_POLICY_TCP_ONLY:
			qctx->client->message->flags |= DNS_MESSAGEFLAG_TC;
			if (result == DNS_R_NXDOMAIN ||
			    result == DNS_R_NCACHENXDOMAIN)
			{
				qctx->client->message->rcode =
					dns_rcode_nxdomain;
			}
			rpz_log_rewrite(qctx->client, false,
					qctx->rpz_st->m.policy,
					qctx->rpz_st->m.type, qctx->zone,
					qctx->rpz_st->p_name, NULL,
					qctx->rpz_st->m.rpz->num);
			return (ISC_R_COMPLETE);
		case DNS_RPZ_POLICY_DROP:
			QUERY_ERROR(qctx, DNS_R_DROP);
			rpz_log_rewrite(qctx->client, false,
					qctx->rpz_st->m.policy,
					qctx->rpz_st->m.type, qctx->zone,
					qctx->rpz_st->p_name, NULL,
					qctx->rpz_st->m.rpz->num);
			return (ISC_R_COMPLETE);
		case DNS_RPZ_POLICY_NXDOMAIN:
			result = DNS_R_NXDOMAIN;
			qctx->nxrewrite = true;
			qctx->rpz = true;
			break;
		case DNS_RPZ_POLICY_NODATA:
			qctx->nxrewrite = true;
			FALLTHROUGH;
		case DNS_RPZ_POLICY_DNS64:
			result = DNS_R_NXRRSET;
			qctx->rpz = true;
			break;
		case DNS_RPZ_POLICY_RECORD:
			result = qctx->rpz_st->m.result;
			if (qctx->qtype == dns_rdatatype_any &&
			    result != DNS_R_CNAME)
			{
				/*
				 * We will add all of the rdatasets of
				 * the node by iterating later,
				 * and set the TTL then.
				 */
				if (dns_rdataset_isassociated(qctx->rdataset)) {
					dns_rdataset_disassociate(
						qctx->rdataset);
				}
			} else {
				/*
				 * We will add this rdataset.
				 */
				qctx->rdataset->ttl =
					ISC_MIN(qctx->rdataset->ttl,
						qctx->rpz_st->m.ttl);
			}
			qctx->rpz = true;
			break;
		case DNS_RPZ_POLICY_WILDCNAME: {
			dns_rdata_t rdata = DNS_RDATA_INIT;
			dns_rdata_cname_t cname;
			result = dns_rdataset_first(qctx->rdataset);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			dns_rdataset_current(qctx->rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &cname, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			dns_rdata_reset(&rdata);
			result = query_rpzcname(qctx, &cname.cname);
			if (result != ISC_R_SUCCESS) {
				return (ISC_R_COMPLETE);
			}
			qctx->fname = NULL;
			qctx->want_restart = true;
			return (ISC_R_COMPLETE);
		}
		case DNS_RPZ_POLICY_CNAME:
			/*
			 * Add overriding CNAME from a named.conf
			 * response-policy statement
			 */
			result = query_rpzcname(qctx,
						&qctx->rpz_st->m.rpz->cname);
			if (result != ISC_R_SUCCESS) {
				return (ISC_R_COMPLETE);
			}
			qctx->fname = NULL;
			qctx->want_restart = true;
			return (ISC_R_COMPLETE);
		default:
			UNREACHABLE();
		}

		/*
		 * Turn off DNSSEC because the results of a
		 * response policy zone cannot verify.
		 */
		qctx->client->attributes &= ~(NS_CLIENTATTR_WANTDNSSEC |
					      NS_CLIENTATTR_WANTAD);
		qctx->client->message->flags &= ~DNS_MESSAGEFLAG_AD;
		ns_client_putrdataset(qctx->client, &qctx->sigrdataset);
		qctx->rpz_st->q.is_zone = qctx->is_zone;
		qctx->is_zone = true;
		rpz_log_rewrite(qctx->client, false, qctx->rpz_st->m.policy,
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
	ns_client_t *client;
	dns_fixedname_t prefix, suffix;
	unsigned int labels;
	isc_result_t result;

	REQUIRE(qctx != NULL && qctx->client != NULL);

	client = qctx->client;

	CTRACE(ISC_LOG_DEBUG(3), "query_rpzcname");

	labels = dns_name_countlabels(cname);
	if (labels > 2 && dns_name_iswildcard(cname)) {
		dns_fixedname_init(&prefix);
		dns_name_split(client->query.qname, 1,
			       dns_fixedname_name(&prefix), NULL);
		dns_fixedname_init(&suffix);
		dns_name_split(cname, labels - 1, NULL,
			       dns_fixedname_name(&suffix));
		result = dns_name_concatenate(dns_fixedname_name(&prefix),
					      dns_fixedname_name(&suffix),
					      qctx->fname, NULL);
		if (result == DNS_R_NAMETOOLONG) {
			client->message->rcode = dns_rcode_yxdomain;
		} else if (result != ISC_R_SUCCESS) {
			return (result);
		}
	} else {
		dns_name_copynf(cname, qctx->fname);
	}

	ns_client_keepname(client, qctx->fname, qctx->dbuf);
	result = query_addcname(qctx, dns_trust_authanswer,
				qctx->rpz_st->m.ttl);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	rpz_log_rewrite(client, false, qctx->rpz_st->m.policy,
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
 * Return true when found, otherwise return false.
 */
static bool
has_ta(query_ctx_t *qctx) {
	dns_keytable_t *keytable = NULL;
	dns_keynode_t *keynode = NULL;
	dns_rdataset_t dsset;
	dns_keytag_t sentinel = qctx->client->query.root_key_sentinel_keyid;
	isc_result_t result;

	result = dns_view_getsecroots(qctx->view, &keytable);
	if (result != ISC_R_SUCCESS) {
		return (false);
	}

	result = dns_keytable_find(keytable, dns_rootname, &keynode);
	if (result != ISC_R_SUCCESS) {
		if (keynode != NULL) {
			dns_keytable_detachkeynode(keytable, &keynode);
		}
		dns_keytable_detach(&keytable);
		return (false);
	}

	dns_rdataset_init(&dsset);
	if (dns_keynode_dsset(keynode, &dsset)) {
		for (result = dns_rdataset_first(&dsset);
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&dsset))
		{
			dns_rdata_t rdata = DNS_RDATA_INIT;
			dns_rdata_ds_t ds;

			dns_rdata_reset(&rdata);
			dns_rdataset_current(&dsset, &rdata);
			result = dns_rdata_tostruct(&rdata, &ds, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			if (ds.key_tag == sentinel) {
				dns_keytable_detachkeynode(keytable, &keynode);
				dns_keytable_detach(&keytable);
				dns_rdataset_disassociate(&dsset);
				return (true);
			}
		}
		dns_rdataset_disassociate(&dsset);
	}

	if (keynode != NULL) {
		dns_keytable_detachkeynode(keytable, &keynode);
	}

	dns_keytable_detach(&keytable);

	return (false);
}

/*%
 * Check if a root key sentinel SERVFAIL should be returned.
 */
static bool
root_key_sentinel_return_servfail(query_ctx_t *qctx, isc_result_t result) {
	/*
	 * Are we looking at a "root-key-sentinel" query?
	 */
	if (!qctx->client->query.root_key_sentinel_is_ta &&
	    !qctx->client->query.root_key_sentinel_not_ta)
	{
		return (false);
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
		return (false);
	}

	/*
	 * Do we meet the specified conditions to return SERVFAIL?
	 */
	if (!qctx->is_zone && qctx->rdataset->trust == dns_trust_secure &&
	    ((qctx->client->query.root_key_sentinel_is_ta && !has_ta(qctx)) ||
	     (qctx->client->query.root_key_sentinel_not_ta && has_ta(qctx))))
	{
		return (true);
	}

	/*
	 * As special processing may only be triggered by the original QNAME,
	 * disable it after following a CNAME/DNAME.
	 */
	qctx->client->query.root_key_sentinel_is_ta = false;
	qctx->client->query.root_key_sentinel_not_ta = false;

	return (false);
}

/*%
 * If serving stale answers is allowed, set up 'qctx' to look for one and
 * return true; otherwise, return false.
 */
static bool
query_usestale(query_ctx_t *qctx, isc_result_t result) {
	if ((qctx->client->query.dboptions & DNS_DBFIND_STALEOK) != 0) {
		/*
		 * Query was already using stale, if that didn't work the
		 * last time, it won't work this time either.
		 */
		return (false);
	}

	if (result == DNS_R_DUPLICATE || result == DNS_R_DROP) {
		/*
		 * Don't enable serve-stale if the result signals a duplicate
		 * query or query that is being dropped.
		 */
		return (false);
	}

	qctx_clean(qctx);
	qctx_freedata(qctx);

	if (dns_view_staleanswerenabled(qctx->client->view)) {
		dns_db_attach(qctx->client->view->cachedb, &qctx->db);
		qctx->version = NULL;
		qctx->client->query.dboptions |= DNS_DBFIND_STALEOK;
		if (qctx->client->query.fetch != NULL) {
			dns_resolver_destroyfetch(&qctx->client->query.fetch);
		}

		/*
		 * Start the stale-refresh-time window in case there was a
		 * resolver query timeout.
		 */
		if (qctx->resuming && result == ISC_R_TIMEDOUT) {
			qctx->client->query.dboptions |= DNS_DBFIND_STALESTART;
		}
		return (true);
	}

	return (false);
}

/*%
 * Continue after doing a database lookup or returning from
 * recursion, and call out to the next function depending on the
 * result from the search.
 */
static isc_result_t
query_gotanswer(query_ctx_t *qctx, isc_result_t res) {
	isc_result_t result = res;
	char errmsg[256];

	CCTRACE(ISC_LOG_DEBUG(3), "query_gotanswer");

	CALL_HOOK(NS_QUERY_GOT_ANSWER_BEGIN, qctx);

	if (query_checkrrl(qctx, result) != ISC_R_SUCCESS) {
		return (ns_query_done(qctx));
	}

	if (!dns_name_equal(qctx->client->query.qname, dns_rootname)) {
		result = query_checkrpz(qctx, result);
		if (result == ISC_R_NOTFOUND) {
			/*
			 * RPZ not configured for this view.
			 */
			goto root_key_sentinel;
		}
		if (RECURSING(qctx->client) && result == DNS_R_DISALLOWED) {
			/*
			 * We are recursing, and thus RPZ processing is not
			 * allowed at the moment. This could happen on a
			 * "stale-answer-client-timeout" lookup. In this case,
			 * bail out and wait for recursion to complete, as we
			 * we can't perform the RPZ rewrite rules.
			 */
			return (result);
		}
		if (result == ISC_R_COMPLETE) {
			return (ns_query_done(qctx));
		}
	}

root_key_sentinel:
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
		return (ns_query_done(qctx));
	}

	switch (result) {
	case ISC_R_SUCCESS:
		return (query_prepresponse(qctx));

	case DNS_R_GLUE:
	case DNS_R_ZONECUT:
		INSIST(qctx->is_zone);
		qctx->authoritative = false;
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
		return (query_nxdomain(qctx, true));

	case DNS_R_NXDOMAIN:
		return (query_nxdomain(qctx, false));

	case DNS_R_COVERINGNSEC:
		return (query_coveringnsec(qctx));

	case DNS_R_NCACHENXDOMAIN:
		result = query_redirect(qctx);
		if (result != ISC_R_COMPLETE) {
			return (result);
		}
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
		if (query_usestale(qctx, result)) {
			/*
			 * If serve-stale is enabled, query_usestale() already
			 * set up 'qctx' for looking up a stale response.
			 */
			return (query_lookup(qctx));
		}

		/*
		 * Regardless of the triggering result, we definitely
		 * want to return SERVFAIL from here.
		 */
		qctx->client->rcode_override = dns_rcode_servfail;

		QUERY_ERROR(qctx, result);
		return (ns_query_done(qctx));
	}

cleanup:
	return (result);
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

	dbuf = ns_client_getnamebuf(client);
	if (dbuf == NULL) {
		goto cleanup;
	}

	fname = ns_client_newname(client, dbuf, &b);
	neg = ns_client_newrdataset(client);
	negsig = ns_client_newrdataset(client);
	if (fname == NULL || neg == NULL || negsig == NULL) {
		goto cleanup;
	}

	result = dns_rdataset_getnoqname(qctx->noqname, fname, neg, negsig);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	query_addrrset(qctx, &fname, &neg, &negsig, dbuf,
		       DNS_SECTION_AUTHORITY);

	if ((qctx->noqname->attributes & DNS_RDATASETATTR_CLOSEST) == 0) {
		goto cleanup;
	}

	if (fname == NULL) {
		dbuf = ns_client_getnamebuf(client);
		if (dbuf == NULL) {
			goto cleanup;
		}
		fname = ns_client_newname(client, dbuf, &b);
	}

	if (neg == NULL) {
		neg = ns_client_newrdataset(client);
	} else if (dns_rdataset_isassociated(neg)) {
		dns_rdataset_disassociate(neg);
	}

	if (negsig == NULL) {
		negsig = ns_client_newrdataset(client);
	} else if (dns_rdataset_isassociated(negsig)) {
		dns_rdataset_disassociate(negsig);
	}

	if (fname == NULL || neg == NULL || negsig == NULL) {
		goto cleanup;
	}
	result = dns_rdataset_getclosest(qctx->noqname, fname, neg, negsig);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	query_addrrset(qctx, &fname, &neg, &negsig, dbuf,
		       DNS_SECTION_AUTHORITY);

cleanup:
	if (neg != NULL) {
		ns_client_putrdataset(client, &neg);
	}
	if (negsig != NULL) {
		ns_client_putrdataset(client, &negsig);
	}
	if (fname != NULL) {
		ns_client_releasename(client, &fname);
	}
}

/*%
 * Build the response for a query for type ANY.
 */
static isc_result_t
query_respond_any(query_ctx_t *qctx) {
	bool found = false, hidden = false;
	dns_rdatasetiter_t *rdsiter = NULL;
	isc_result_t result;
	dns_rdatatype_t onetype = 0; /* type to use for minimal-any */
	isc_buffer_t b;

	CCTRACE(ISC_LOG_DEBUG(3), "query_respond_any");

	CALL_HOOK(NS_QUERY_RESPOND_ANY_BEGIN, qctx);

	result = dns_db_allrdatasets(qctx->db, qctx->node, qctx->version, 0, 0,
				     &rdsiter);
	if (result != ISC_R_SUCCESS) {
		CCTRACE(ISC_LOG_ERROR, "query_respond_any: allrdatasets "
				       "failed");
		QUERY_ERROR(qctx, result);
		return (ns_query_done(qctx));
	}

	/*
	 * Calling query_addrrset() with a non-NULL dbuf is going
	 * to either keep or release the name.  We don't want it to
	 * release fname, since we may have to call query_addrrset()
	 * more than once.  That means we have to call ns_client_keepname()
	 * now, and pass a NULL dbuf to query_addrrset().
	 *
	 * If we do a query_addrrset() below, we must set qctx->fname to
	 * NULL before leaving this block, otherwise we might try to
	 * cleanup qctx->fname even though we're using it!
	 */
	ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);
	qctx->tname = qctx->fname;

	result = dns_rdatasetiter_first(rdsiter);
	while (result == ISC_R_SUCCESS) {
		dns_rdatasetiter_current(rdsiter, qctx->rdataset);

		/*
		 * We found an NS RRset; no need to add one later.
		 */
		if (qctx->qtype == dns_rdatatype_any &&
		    qctx->rdataset->type == dns_rdatatype_ns)
		{
			qctx->answer_has_ns = true;
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
			 * The zone may be transitioning from insecure
			 * to secure. Hide DNSSEC records from ANY queries.
			 */
			dns_rdataset_disassociate(qctx->rdataset);
			hidden = true;
		} else if (qctx->view->minimal_any && !TCP(qctx->client) &&
			   !WANTDNSSEC(qctx->client) &&
			   qctx->qtype == dns_rdatatype_any &&
			   (qctx->rdataset->type == dns_rdatatype_sig ||
			    qctx->rdataset->type == dns_rdatatype_rrsig))
		{
			CCTRACE(ISC_LOG_DEBUG(5), "query_respond_any: "
						  "minimal-any skip signature");
			dns_rdataset_disassociate(qctx->rdataset);
		} else if (qctx->view->minimal_any && !TCP(qctx->client) &&
			   onetype != 0 && qctx->rdataset->type != onetype &&
			   qctx->rdataset->covers != onetype)
		{
			CCTRACE(ISC_LOG_DEBUG(5), "query_respond_any: "
						  "minimal-any skip rdataset");
			dns_rdataset_disassociate(qctx->rdataset);
		} else if ((qctx->qtype == dns_rdatatype_any ||
			    qctx->rdataset->type == qctx->qtype) &&
			   qctx->rdataset->type != 0)
		{
			if (NOQNAME(qctx->rdataset) && WANTDNSSEC(qctx->client))
			{
				qctx->noqname = qctx->rdataset;
			} else {
				qctx->noqname = NULL;
			}

			qctx->rpz_st = qctx->client->query.rpz_st;
			if (qctx->rpz_st != NULL) {
				qctx->rdataset->ttl =
					ISC_MIN(qctx->rdataset->ttl,
						qctx->rpz_st->m.ttl);
			}

			if (!qctx->is_zone && RECURSIONOK(qctx->client)) {
				dns_name_t *name;
				name = (qctx->fname != NULL) ? qctx->fname
							     : qctx->tname;
				query_prefetch(qctx->client, name,
					       qctx->rdataset);
			}

			/*
			 * Remember the first RRtype we find so we
			 * can skip others with minimal-any.
			 */
			if (qctx->rdataset->type == dns_rdatatype_sig ||
			    qctx->rdataset->type == dns_rdatatype_rrsig)
			{
				onetype = qctx->rdataset->covers;
			} else {
				onetype = qctx->rdataset->type;
			}

			query_addrrset(qctx,
				       (qctx->fname != NULL) ? &qctx->fname
							     : &qctx->tname,
				       &qctx->rdataset, NULL, NULL,
				       DNS_SECTION_ANSWER);

			query_addnoqnameproof(qctx);

			found = true;
			INSIST(qctx->tname != NULL);

			/*
			 * rdataset is non-NULL only in certain
			 * pathological cases involving DNAMEs.
			 */
			if (qctx->rdataset != NULL) {
				ns_client_putrdataset(qctx->client,
						      &qctx->rdataset);
			}

			qctx->rdataset = ns_client_newrdataset(qctx->client);
			if (qctx->rdataset == NULL) {
				break;
			}
		} else {
			/*
			 * We're not interested in this rdataset.
			 */
			dns_rdataset_disassociate(qctx->rdataset);
		}

		result = dns_rdatasetiter_next(rdsiter);
	}

	dns_rdatasetiter_destroy(&rdsiter);

	if (result != ISC_R_NOMORE) {
		CCTRACE(ISC_LOG_ERROR, "query_respond_any: rdataset iterator "
				       "failed");
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
		return (ns_query_done(qctx));
	}

	if (found) {
		/*
		 * Call hook if any answers were found.
		 * Do this before releasing qctx->fname, in case
		 * the hook function needs it.
		 */
		CALL_HOOK(NS_QUERY_RESPOND_ANY_FOUND, qctx);
	}

	if (qctx->fname != NULL) {
		dns_message_puttempname(qctx->client->message, &qctx->fname);
	}

	if (found) {
		/*
		 * At least one matching rdataset was found
		 */
		query_addauth(qctx);
	} else if (qctx->qtype == dns_rdatatype_rrsig ||
		   qctx->qtype == dns_rdatatype_sig)
	{
		/*
		 * No matching rdatasets were found, but we got
		 * here on a search for RRSIG/SIG, so that's okay.
		 */
		if (!qctx->is_zone) {
			qctx->authoritative = false;
			qctx->client->attributes &= ~NS_CLIENTATTR_RA;
			query_addauth(qctx);
			return (ns_query_done(qctx));
		}

		if (qctx->qtype == dns_rdatatype_rrsig &&
		    dns_db_issecure(qctx->db))
		{
			char namebuf[DNS_NAME_FORMATSIZE];
			dns_name_format(qctx->client->query.qname, namebuf,
					sizeof(namebuf));
			ns_client_log(qctx->client, DNS_LOGCATEGORY_DNSSEC,
				      NS_LOGMODULE_QUERY, ISC_LOG_WARNING,
				      "missing signature for %s", namebuf);
		}

		qctx->fname = ns_client_newname(qctx->client, qctx->dbuf, &b);
		return (query_sign_nodata(qctx));
	} else if (!hidden) {
		/*
		 * No matching rdatasets were found and nothing was
		 * deliberately hidden: something must have gone wrong.
		 */
		QUERY_ERROR(qctx, DNS_R_SERVFAIL);
	}

	return (ns_query_done(qctx));

cleanup:
	return (result);
}

/*
 * Set the expire time, if requested, when answering from a slave, mirror, or
 * master zone.
 */
static void
query_getexpire(query_ctx_t *qctx) {
	dns_zone_t *raw = NULL, *mayberaw;

	CCTRACE(ISC_LOG_DEBUG(3), "query_getexpire");

	if (qctx->zone == NULL || !qctx->is_zone ||
	    qctx->qtype != dns_rdatatype_soa ||
	    qctx->client->query.restarts != 0 ||
	    (qctx->client->attributes & NS_CLIENTATTR_WANTEXPIRE) == 0)
	{
		return;
	}

	dns_zone_getraw(qctx->zone, &raw);
	mayberaw = (raw != NULL) ? raw : qctx->zone;

	if (dns_zone_gettype(mayberaw) == dns_zone_secondary ||
	    dns_zone_gettype(mayberaw) == dns_zone_mirror)
	{
		isc_time_t expiretime;
		uint32_t secs;
		dns_zone_getexpiretime(qctx->zone, &expiretime);
		secs = isc_time_seconds(&expiretime);
		if (secs >= qctx->client->now && qctx->result == ISC_R_SUCCESS)
		{
			qctx->client->attributes |= NS_CLIENTATTR_HAVEEXPIRE;
			qctx->client->expire = secs - qctx->client->now;
		}
	} else if (dns_zone_gettype(mayberaw) == dns_zone_primary) {
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

/*%
 * Fill the ANSWER section of a positive response.
 */
static isc_result_t
query_addanswer(query_ctx_t *qctx) {
	dns_rdataset_t **sigrdatasetp = NULL;
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_addanswer");

	CALL_HOOK(NS_QUERY_ADDANSWER_BEGIN, qctx);

	/*
	 * On normal lookups, clear any rdatasets that were added on a
	 * lookup due to stale-answer-client-timeout. Do not clear if we
	 * are going to refresh the RRset, because the stale contents are
	 * prioritized.
	 */
	if (QUERY_STALEOK(&qctx->client->query) &&
	    !QUERY_STALETIMEOUT(&qctx->client->query) && !qctx->refresh_rrset)
	{
		CCTRACE(ISC_LOG_DEBUG(3), "query_clear_stale");
		query_clear_stale(qctx->client);
		/*
		 * We can clear the attribute to prevent redundant clearing
		 * in subsequent lookups.
		 */
		qctx->client->query.attributes &= ~NS_QUERYATTR_STALEOK;
	}

	if (qctx->dns64) {
		result = query_dns64(qctx);
		qctx->noqname = NULL;
		dns_rdataset_disassociate(qctx->rdataset);
		dns_message_puttemprdataset(qctx->client->message,
					    &qctx->rdataset);
		if (result == ISC_R_NOMORE) {
#ifndef dns64_bis_return_excluded_addresses
			if (qctx->dns64_exclude) {
				if (!qctx->is_zone) {
					return (ns_query_done(qctx));
				}
				/*
				 * Add a fake SOA record.
				 */
				(void)query_addsoa(qctx, 600,
						   DNS_SECTION_AUTHORITY);
				return (ns_query_done(qctx));
			}
#endif /* ifndef dns64_bis_return_excluded_addresses */
			if (qctx->is_zone) {
				return (query_nodata(qctx, DNS_R_NXDOMAIN));
			} else {
				return (query_ncache(qctx, DNS_R_NXDOMAIN));
			}
		} else if (result != ISC_R_SUCCESS) {
			qctx->result = result;
			return (ns_query_done(qctx));
		}
	} else if (qctx->client->query.dns64_aaaaok != NULL) {
		query_filter64(qctx);
		ns_client_putrdataset(qctx->client, &qctx->rdataset);
	} else {
		if (!qctx->is_zone && RECURSIONOK(qctx->client) &&
		    !QUERY_STALETIMEOUT(&qctx->client->query))
		{
			query_prefetch(qctx->client, qctx->fname,
				       qctx->rdataset);
		}
		if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL) {
			sigrdatasetp = &qctx->sigrdataset;
		}
		query_addrrset(qctx, &qctx->fname, &qctx->rdataset,
			       sigrdatasetp, qctx->dbuf, DNS_SECTION_ANSWER);
	}

	return (ISC_R_COMPLETE);

cleanup:
	return (result);
}

/*%
 * Build a response for a "normal" query, for a type other than ANY,
 * for which we have an answer (either positive or negative).
 */
static isc_result_t
query_respond(query_ctx_t *qctx) {
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_respond");

	/*
	 * Check to see if the AAAA RRset has non-excluded addresses
	 * in it.  If not look for a A RRset.
	 */
	INSIST(qctx->client->query.dns64_aaaaok == NULL);

	if (qctx->qtype == dns_rdatatype_aaaa && !qctx->dns64_exclude &&
	    !ISC_LIST_EMPTY(qctx->view->dns64) &&
	    qctx->client->message->rdclass == dns_rdataclass_in &&
	    !dns64_aaaaok(qctx->client, qctx->rdataset, qctx->sigrdataset))
	{
		/*
		 * Look to see if there are A records for this name.
		 */
		qctx->client->query.dns64_ttl = qctx->rdataset->ttl;
		SAVE(qctx->client->query.dns64_aaaa, qctx->rdataset);
		SAVE(qctx->client->query.dns64_sigaaaa, qctx->sigrdataset);
		ns_client_releasename(qctx->client, &qctx->fname);
		dns_db_detachnode(qctx->db, &qctx->node);
		qctx->type = qctx->qtype = dns_rdatatype_a;
		qctx->dns64_exclude = qctx->dns64 = true;

		return (query_lookup(qctx));
	}

	/*
	 * XXX: This hook is meant to be at the top of this function,
	 * but is postponed until after DNS64 in order to avoid an
	 * assertion if the hook causes recursion. (When DNS64 also
	 * becomes a plugin, it will be necessary to find some
	 * other way to prevent that assertion, since the order in
	 * which plugins are configured can't be enforced.)
	 */
	CALL_HOOK(NS_QUERY_RESPOND_BEGIN, qctx);

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
			qctx->answer_has_ns = true;
		}

		/*
		 * Always add glue for root priming queries, regardless
		 * of "minimal-responses" setting.
		 */
		if (dns_name_equal(qctx->client->query.qname, dns_rootname)) {
			qctx->client->query.attributes &=
				~NS_QUERYATTR_NOADDITIONAL;
			dns_db_attach(qctx->db, &qctx->client->query.gluedb);
		}
	}

	/*
	 * Set expire time
	 */
	query_getexpire(qctx);

	result = query_addanswer(qctx);
	if (result != ISC_R_COMPLETE) {
		return (result);
	}

	query_addnoqnameproof(qctx);

	/*
	 * 'qctx->rdataset' will only be non-NULL here if the ANSWER section of
	 * the message to be sent to the client already contains an RRset with
	 * the same owner name and the same type as 'qctx->rdataset'.  This
	 * should never happen, with one exception: when chasing DNAME records,
	 * one of the DNAME records placed in the ANSWER section may turn out
	 * to be the final answer to the client's query, but we have no way of
	 * knowing that until now.  In such a case, 'qctx->rdataset' will be
	 * freed later, so we do not need to free it here.
	 */
	INSIST(qctx->rdataset == NULL || qctx->qtype == dns_rdatatype_dname);

	query_addauth(qctx);

	return (ns_query_done(qctx));

cleanup:
	return (result);
}

static isc_result_t
query_dns64(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	dns_aclenv_t *env =
		ns_interfacemgr_getaclenv(client->manager->interface->mgr);
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
	result = dns_message_findname(
		client->message, section, name, dns_rdatatype_aaaa,
		qctx->rdataset->covers, &mname, &mrdataset);
	if (result == ISC_R_SUCCESS) {
		/*
		 * We've already got an RRset of the given name and type.
		 * There's nothing else to do;
		 */
		CTRACE(ISC_LOG_DEBUG(3), "query_dns64: dns_message_findname "
					 "succeeded: done");
		if (qctx->dbuf != NULL) {
			ns_client_releasename(client, &qctx->fname);
		}
		return (ISC_R_SUCCESS);
	} else if (result == DNS_R_NXDOMAIN) {
		/*
		 * The name doesn't exist.
		 */
		if (qctx->dbuf != NULL) {
			ns_client_keepname(client, name, qctx->dbuf);
		}
		dns_message_addname(client->message, name, section);
		qctx->fname = NULL;
		mname = name;
	} else {
		RUNTIME_CHECK(result == DNS_R_NXRRSET);
		if (qctx->dbuf != NULL) {
			ns_client_releasename(client, &qctx->fname);
		}
	}

	if (qctx->rdataset->trust != dns_trust_secure) {
		client->query.attributes &= ~NS_QUERYATTR_SECURE;
	}

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);

	isc_buffer_allocate(client->mctx, &buffer,
			    view->dns64cnt * 16 *
				    dns_rdataset_count(qctx->rdataset));
	result = dns_message_gettemprdataset(client->message, &dns64_rdataset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_gettemprdatalist(client->message,
					      &dns64_rdatalist);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	dns_rdatalist_init(dns64_rdatalist);
	dns64_rdatalist->rdclass = dns_rdataclass_in;
	dns64_rdatalist->type = dns_rdatatype_aaaa;
	if (client->query.dns64_ttl != UINT32_MAX) {
		dns64_rdatalist->ttl = ISC_MIN(qctx->rdataset->ttl,
					       client->query.dns64_ttl);
	} else {
		dns64_rdatalist->ttl = ISC_MIN(qctx->rdataset->ttl, 600);
	}

	if (RECURSIONOK(client)) {
		flags |= DNS_DNS64_RECURSIVE;
	}

	/*
	 * We use the signatures from the A lookup to set DNS_DNS64_DNSSEC
	 * as this provides a easy way to see if the answer was signed.
	 */
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL &&
	    dns_rdataset_isassociated(qctx->sigrdataset))
	{
		flags |= DNS_DNS64_DNSSEC;
	}

	for (result = dns_rdataset_first(qctx->rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(qctx->rdataset))
	{
		for (dns64 = ISC_LIST_HEAD(client->view->dns64); dns64 != NULL;
		     dns64 = dns_dns64_next(dns64))
		{
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
			if (result != ISC_R_SUCCESS) {
				goto cleanup;
			}
			dns_rdata_init(dns64_rdata);
			dns_rdata_fromregion(dns64_rdata, dns_rdataclass_in,
					     dns_rdatatype_aaaa, &r);
			ISC_LIST_APPEND(dns64_rdatalist->rdata, dns64_rdata,
					link);
			dns64_rdata = NULL;
			dns_rdata_reset(&rdata);
		}
	}
	if (result != ISC_R_NOMORE) {
		goto cleanup;
	}

	if (ISC_LIST_EMPTY(dns64_rdatalist->rdata)) {
		goto cleanup;
	}

	result = dns_rdatalist_tordataset(dns64_rdatalist, dns64_rdataset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	dns_rdataset_setownercase(dns64_rdataset, mname);
	client->query.attributes |= NS_QUERYATTR_NOADDITIONAL;
	dns64_rdataset->trust = qctx->rdataset->trust;

	query_addtoname(mname, dns64_rdataset);
	query_setorder(qctx, mname, dns64_rdataset);

	dns64_rdataset = NULL;
	dns64_rdatalist = NULL;
	dns_message_takebuffer(client->message, &buffer);
	inc_stats(client, ns_statscounter_dns64);
	result = ISC_R_SUCCESS;

cleanup:
	if (buffer != NULL) {
		isc_buffer_free(&buffer);
	}

	if (dns64_rdata != NULL) {
		dns_message_puttemprdata(client->message, &dns64_rdata);
	}

	if (dns64_rdataset != NULL) {
		dns_message_puttemprdataset(client->message, &dns64_rdataset);
	}

	if (dns64_rdatalist != NULL) {
		for (dns64_rdata = ISC_LIST_HEAD(dns64_rdatalist->rdata);
		     dns64_rdata != NULL;
		     dns64_rdata = ISC_LIST_HEAD(dns64_rdatalist->rdata))
		{
			ISC_LIST_UNLINK(dns64_rdatalist->rdata, dns64_rdata,
					link);
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
	result = dns_message_findname(
		client->message, section, name, dns_rdatatype_aaaa,
		qctx->rdataset->covers, &mname, &myrdataset);
	if (result == ISC_R_SUCCESS) {
		/*
		 * We've already got an RRset of the given name and type.
		 * There's nothing else to do;
		 */
		CTRACE(ISC_LOG_DEBUG(3), "query_filter64: dns_message_findname "
					 "succeeded: done");
		if (qctx->dbuf != NULL) {
			ns_client_releasename(client, &qctx->fname);
		}
		return;
	} else if (result == DNS_R_NXDOMAIN) {
		mname = name;
		qctx->fname = NULL;
	} else {
		RUNTIME_CHECK(result == DNS_R_NXRRSET);
		if (qctx->dbuf != NULL) {
			ns_client_releasename(client, &qctx->fname);
		}
		qctx->dbuf = NULL;
	}

	if (qctx->rdataset->trust != dns_trust_secure) {
		client->query.attributes &= ~NS_QUERYATTR_SECURE;
	}

	isc_buffer_allocate(client->mctx, &buffer,
			    16 * dns_rdataset_count(qctx->rdataset));
	result = dns_message_gettemprdataset(client->message, &myrdataset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	result = dns_message_gettemprdatalist(client->message, &myrdatalist);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	dns_rdatalist_init(myrdatalist);
	myrdatalist->rdclass = dns_rdataclass_in;
	myrdatalist->type = dns_rdatatype_aaaa;
	myrdatalist->ttl = qctx->rdataset->ttl;

	i = 0;
	for (result = dns_rdataset_first(qctx->rdataset);
	     result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(qctx->rdataset))
	{
		if (!client->query.dns64_aaaaok[i++]) {
			continue;
		}
		dns_rdataset_current(qctx->rdataset, &rdata);
		INSIST(rdata.length == 16);
		isc_buffer_putmem(buffer, rdata.data, rdata.length);
		isc_buffer_remainingregion(buffer, &r);
		isc_buffer_forward(buffer, rdata.length);
		result = dns_message_gettemprdata(client->message, &myrdata);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		dns_rdata_init(myrdata);
		dns_rdata_fromregion(myrdata, dns_rdataclass_in,
				     dns_rdatatype_aaaa, &r);
		ISC_LIST_APPEND(myrdatalist->rdata, myrdata, link);
		myrdata = NULL;
		dns_rdata_reset(&rdata);
	}
	if (result != ISC_R_NOMORE) {
		goto cleanup;
	}

	result = dns_rdatalist_tordataset(myrdatalist, myrdataset);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}
	dns_rdataset_setownercase(myrdataset, name);
	client->query.attributes |= NS_QUERYATTR_NOADDITIONAL;
	if (mname == name) {
		if (qctx->dbuf != NULL) {
			ns_client_keepname(client, name, qctx->dbuf);
		}
		dns_message_addname(client->message, name, section);
		qctx->dbuf = NULL;
	}
	myrdataset->trust = qctx->rdataset->trust;

	query_addtoname(mname, myrdataset);
	query_setorder(qctx, mname, myrdataset);

	myrdataset = NULL;
	myrdatalist = NULL;
	dns_message_takebuffer(client->message, &buffer);

cleanup:
	if (buffer != NULL) {
		isc_buffer_free(&buffer);
	}

	if (myrdata != NULL) {
		dns_message_puttemprdata(client->message, &myrdata);
	}

	if (myrdataset != NULL) {
		dns_message_puttemprdataset(client->message, &myrdataset);
	}

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
	if (qctx->dbuf != NULL) {
		ns_client_releasename(client, &name);
	}

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

	CCTRACE(ISC_LOG_DEBUG(3), "query_notfound");

	CALL_HOOK(NS_QUERY_NOTFOUND_BEGIN, qctx);

	INSIST(!qctx->is_zone);

	if (qctx->db != NULL) {
		dns_db_detach(&qctx->db);
	}

	/*
	 * If the cache doesn't even have the root NS,
	 * try to get that from the hints DB.
	 */
	if (qctx->view->hints != NULL) {
		dns_clientinfomethods_t cm;
		dns_clientinfo_t ci;

		dns_clientinfomethods_init(&cm, ns_client_sourceip);
		dns_clientinfo_init(&ci, qctx->client, NULL, NULL);

		dns_db_attach(qctx->view->hints, &qctx->db);
		result = dns_db_findext(qctx->db, dns_rootname, NULL,
					dns_rdatatype_ns, 0, qctx->client->now,
					&qctx->node, qctx->fname, &cm, &ci,
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
			result = ns_query_recurse(qctx->client, qctx->qtype,
						  qctx->client->query.qname,
						  NULL, NULL, qctx->resuming);
			if (result == ISC_R_SUCCESS) {
				CALL_HOOK(NS_QUERY_NOTFOUND_RECURSE, qctx);
				qctx->client->query.attributes |=
					NS_QUERYATTR_RECURSING;

				if (qctx->dns64) {
					qctx->client->query.attributes |=
						NS_QUERYATTR_DNS64;
				}
				if (qctx->dns64_exclude) {
					qctx->client->query.attributes |=
						NS_QUERYATTR_DNS64EXCLUDE;
				}
			} else if (query_usestale(qctx, result)) {
				/*
				 * If serve-stale is enabled, query_usestale()
				 * already set up 'qctx' for looking up a
				 * stale response.
				 */
				return (query_lookup(qctx));
			} else {
				QUERY_ERROR(qctx, result);
			}
			return (ns_query_done(qctx));
		} else {
			/* Unable to give root server referral. */
			CCTRACE(ISC_LOG_ERROR, "unable to give root server "
					       "referral");
			QUERY_ERROR(qctx, result);
			return (ns_query_done(qctx));
		}
	}

	return (query_delegation(qctx));

cleanup:
	return (result);
}

/*%
 * We have a delegation but recursion is not allowed, so return the delegation
 * to the client.
 */
static isc_result_t
query_prepare_delegation_response(query_ctx_t *qctx) {
	isc_result_t result;
	dns_rdataset_t **sigrdatasetp = NULL;
	bool detach = false;

	CALL_HOOK(NS_QUERY_PREP_DELEGATION_BEGIN, qctx);

	/*
	 * qctx->fname could be released in query_addrrset(), so save a copy of
	 * it here in case we need it.
	 */
	dns_fixedname_init(&qctx->dsname);
	dns_name_copynf(qctx->fname, dns_fixedname_name(&qctx->dsname));

	/*
	 * This is the best answer.
	 */
	qctx->client->query.isreferral = true;

	if (!dns_db_iscache(qctx->db) && qctx->client->query.gluedb == NULL) {
		dns_db_attach(qctx->db, &qctx->client->query.gluedb);
		detach = true;
	}

	/*
	 * We must ensure NOADDITIONAL is off, because the generation of
	 * additional data is required in delegations.
	 */
	qctx->client->query.attributes &= ~NS_QUERYATTR_NOADDITIONAL;
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL) {
		sigrdatasetp = &qctx->sigrdataset;
	}
	query_addrrset(qctx, &qctx->fname, &qctx->rdataset, sigrdatasetp,
		       qctx->dbuf, DNS_SECTION_AUTHORITY);
	if (detach) {
		dns_db_detach(&qctx->client->query.gluedb);
	}

	/*
	 * Add DS/NSEC(3) record(s) if needed.
	 */
	query_addds(qctx);

	return (ns_query_done(qctx));

cleanup:
	return (result);
}

/*%
 * Handle a delegation response from an authoritative lookup. This
 * may trigger additional lookups, e.g. from the cache database to
 * see if we have a better answer; if that is not allowed, return the
 * delegation to the client and call ns_query_done().
 */
static isc_result_t
query_zone_delegation(query_ctx_t *qctx) {
	isc_result_t result;

	CALL_HOOK(NS_QUERY_ZONE_DELEGATION_BEGIN, qctx);

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
		result = query_getzonedb(
			qctx->client, qctx->client->query.qname, qctx->qtype,
			DNS_GETDB_PARTIAL, &tzone, &tdb, &tversion);
		if (result != ISC_R_SUCCESS) {
			if (tdb != NULL) {
				dns_db_detach(&tdb);
			}
			if (tzone != NULL) {
				dns_zone_detach(&tzone);
			}
		} else {
			qctx->options &= ~DNS_GETDB_NOEXACT;
			ns_client_putrdataset(qctx->client, &qctx->rdataset);
			if (qctx->sigrdataset != NULL) {
				ns_client_putrdataset(qctx->client,
						      &qctx->sigrdataset);
			}
			if (qctx->fname != NULL) {
				ns_client_releasename(qctx->client,
						      &qctx->fname);
			}
			if (qctx->node != NULL) {
				dns_db_detachnode(qctx->db, &qctx->node);
			}
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
			qctx->authoritative = true;

			return (query_lookup(qctx));
		}
	}

	if (USECACHE(qctx->client) &&
	    (RECURSIONOK(qctx->client) ||
	     (qctx->zone != NULL &&
	      dns_zone_gettype(qctx->zone) == dns_zone_mirror)))
	{
		/*
		 * We might have a better answer or delegation in the
		 * cache.  We'll remember the current values of fname,
		 * rdataset, and sigrdataset.  We'll then go looking for
		 * QNAME in the cache.  If we find something better, we'll
		 * use it instead. If not, then query_lookup() calls
		 * query_notfound() which calls query_delegation(), and
		 * we'll restore these values there.
		 */
		ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);
		SAVE(qctx->zdb, qctx->db);
		SAVE(qctx->znode, qctx->node);
		SAVE(qctx->zfname, qctx->fname);
		SAVE(qctx->zversion, qctx->version);
		SAVE(qctx->zrdataset, qctx->rdataset);
		SAVE(qctx->zsigrdataset, qctx->sigrdataset);
		dns_db_attach(qctx->view->cachedb, &qctx->db);
		qctx->is_zone = false;

		return (query_lookup(qctx));
	}

	return (query_prepare_delegation_response(qctx));

cleanup:
	return (result);
}

/*%
 * Handle delegation responses, including root referrals.
 *
 * If the delegation was returned from authoritative data,
 * call query_zone_delgation().  Otherwise, we can start
 * recursion if allowed; or else return the delegation to the
 * client and call ns_query_done().
 */
static isc_result_t
query_delegation(query_ctx_t *qctx) {
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_delegation");

	CALL_HOOK(NS_QUERY_DELEGATION_BEGIN, qctx);

	qctx->authoritative = false;

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
		ns_client_releasename(qctx->client, &qctx->fname);

		/*
		 * We've already done ns_client_keepname() on
		 * qctx->zfname, so we must set dbuf to NULL to
		 * prevent query_addrrset() from trying to
		 * call ns_client_keepname() again.
		 */
		qctx->dbuf = NULL;
		ns_client_putrdataset(qctx->client, &qctx->rdataset);
		if (qctx->sigrdataset != NULL) {
			ns_client_putrdataset(qctx->client, &qctx->sigrdataset);
		}
		qctx->version = NULL;

		dns_db_detachnode(qctx->db, &qctx->node);
		dns_db_detach(&qctx->db);
		RESTORE(qctx->db, qctx->zdb);
		RESTORE(qctx->node, qctx->znode);
		RESTORE(qctx->fname, qctx->zfname);
		RESTORE(qctx->version, qctx->zversion);
		RESTORE(qctx->rdataset, qctx->zrdataset);
		RESTORE(qctx->sigrdataset, qctx->zsigrdataset);
	}

	result = query_delegation_recurse(qctx);
	if (result != ISC_R_COMPLETE) {
		return (result);
	}

	return (query_prepare_delegation_response(qctx));

cleanup:
	return (result);
}

/*%
 * Handle recursive queries that are triggered as part of the
 * delegation process.
 */
static isc_result_t
query_delegation_recurse(query_ctx_t *qctx) {
	isc_result_t result;
	dns_name_t *qname = qctx->client->query.qname;

	CCTRACE(ISC_LOG_DEBUG(3), "query_delegation_recurse");

	if (!RECURSIONOK(qctx->client)) {
		return (ISC_R_COMPLETE);
	}

	CALL_HOOK(NS_QUERY_DELEGATION_RECURSE_BEGIN, qctx);

	/*
	 * We have a delegation and recursion is allowed,
	 * so we call ns_query_recurse() to follow it.
	 * This phase of the query processing is done;
	 * we'll resume via fetch_callback() and
	 * query_resume() when the recursion is complete.
	 */

	INSIST(!REDIRECT(qctx->client));

	if (dns_rdatatype_atparent(qctx->type)) {
		/*
		 * Parent is authoritative for this RDATA type (i.e. DS).
		 */
		result = ns_query_recurse(qctx->client, qctx->qtype, qname,
					  NULL, NULL, qctx->resuming);
	} else if (qctx->dns64) {
		/*
		 * Look up an A record so we can synthesize DNS64.
		 */
		result = ns_query_recurse(qctx->client, dns_rdatatype_a, qname,
					  NULL, NULL, qctx->resuming);
	} else {
		/*
		 * Any other recursion.
		 */
		result = ns_query_recurse(qctx->client, qctx->qtype, qname,
					  qctx->fname, qctx->rdataset,
					  qctx->resuming);
	}

	if (result == ISC_R_SUCCESS) {
		qctx->client->query.attributes |= NS_QUERYATTR_RECURSING;
		if (qctx->dns64) {
			qctx->client->query.attributes |= NS_QUERYATTR_DNS64;
		}
		if (qctx->dns64_exclude) {
			qctx->client->query.attributes |=
				NS_QUERYATTR_DNS64EXCLUDE;
		}
	} else if (query_usestale(qctx, result)) {
		/*
		 * If serve-stale is enabled, query_usestale() already set up
		 * 'qctx' for looking up a stale response.
		 */
		return (query_lookup(qctx));
	} else {
		QUERY_ERROR(qctx, result);
	}

	return (ns_query_done(qctx));

cleanup:
	return (result);
}

/*%
 * Add DS/NSEC(3) record(s) if needed.
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
	rdataset = ns_client_newrdataset(client);
	sigrdataset = ns_client_newrdataset(client);
	if (rdataset == NULL || sigrdataset == NULL) {
		goto cleanup;
	}

	/*
	 * Look for the DS record, which may or may not be present.
	 */
	result = dns_db_findrdataset(qctx->db, qctx->node, qctx->version,
				     dns_rdatatype_ds, 0, client->now, rdataset,
				     sigrdataset);
	/*
	 * If we didn't find it, look for an NSEC.
	 */
	if (result == ISC_R_NOTFOUND) {
		result = dns_db_findrdataset(
			qctx->db, qctx->node, qctx->version, dns_rdatatype_nsec,
			0, client->now, rdataset, sigrdataset);
	}
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTFOUND) {
		goto addnsec3;
	}
	if (!dns_rdataset_isassociated(rdataset) ||
	    !dns_rdataset_isassociated(sigrdataset))
	{
		goto addnsec3;
	}

	/*
	 * We've already added the NS record, so if the name's not there,
	 * we have other problems.
	 */
	result = dns_message_firstname(client->message, DNS_SECTION_AUTHORITY);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/*
	 * Find the delegation in the response message - it is not necessarily
	 * the first name in the AUTHORITY section when wildcard processing is
	 * involved.
	 */
	while (result == ISC_R_SUCCESS) {
		rname = NULL;
		dns_message_currentname(client->message, DNS_SECTION_AUTHORITY,
					&rname);
		result = dns_message_findtype(rname, dns_rdatatype_ns, 0, NULL);
		if (result == ISC_R_SUCCESS) {
			break;
		}
		result = dns_message_nextname(client->message,
					      DNS_SECTION_AUTHORITY);
	}

	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	/*
	 * Add the relevant RRset (DS or NSEC) to the delegation.
	 */
	query_addrrset(qctx, &rname, &rdataset, &sigrdataset, NULL,
		       DNS_SECTION_AUTHORITY);
	goto cleanup;

addnsec3:
	if (!dns_db_iszone(qctx->db)) {
		goto cleanup;
	}
	/*
	 * Add the NSEC3 which proves the DS does not exist.
	 */
	dbuf = ns_client_getnamebuf(client);
	if (dbuf == NULL) {
		goto cleanup;
	}
	fname = ns_client_newname(client, dbuf, &b);
	dns_fixedname_init(&fixed);
	if (dns_rdataset_isassociated(rdataset)) {
		dns_rdataset_disassociate(rdataset);
	}
	if (dns_rdataset_isassociated(sigrdataset)) {
		dns_rdataset_disassociate(sigrdataset);
	}
	name = dns_fixedname_name(&qctx->dsname);
	query_findclosestnsec3(name, qctx->db, qctx->version, client, rdataset,
			       sigrdataset, fname, true,
			       dns_fixedname_name(&fixed));
	if (!dns_rdataset_isassociated(rdataset)) {
		goto cleanup;
	}
	query_addrrset(qctx, &fname, &rdataset, &sigrdataset, dbuf,
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
		if (fname == NULL || rdataset == NULL || sigrdataset == NULL) {
			goto cleanup;
		}
		query_findclosestnsec3(dns_fixedname_name(&fixed), qctx->db,
				       qctx->version, client, rdataset,
				       sigrdataset, fname, false, NULL);
		if (!dns_rdataset_isassociated(rdataset)) {
			goto cleanup;
		}
		query_addrrset(qctx, &fname, &rdataset, &sigrdataset, dbuf,
			       DNS_SECTION_AUTHORITY);
	}

cleanup:
	if (rdataset != NULL) {
		ns_client_putrdataset(client, &rdataset);
	}
	if (sigrdataset != NULL) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (fname != NULL) {
		ns_client_releasename(client, &fname);
	}
}

/*%
 * Handle authoritative NOERROR/NODATA responses.
 */
static isc_result_t
query_nodata(query_ctx_t *qctx, isc_result_t res) {
	isc_result_t result = res;

	CCTRACE(ISC_LOG_DEBUG(3), "query_nodata");

	CALL_HOOK(NS_QUERY_NODATA_BEGIN, qctx);

#ifdef dns64_bis_return_excluded_addresses
	if (qctx->dns64)
#else  /* ifdef dns64_bis_return_excluded_addresses */
	if (qctx->dns64 && !qctx->dns64_exclude)
#endif /* ifdef dns64_bis_return_excluded_addresses */
	{
		isc_buffer_t b;
		/*
		 * Restore the answers from the previous AAAA lookup.
		 */
		if (qctx->rdataset != NULL) {
			ns_client_putrdataset(qctx->client, &qctx->rdataset);
		}
		if (qctx->sigrdataset != NULL) {
			ns_client_putrdataset(qctx->client, &qctx->sigrdataset);
		}
		RESTORE(qctx->rdataset, qctx->client->query.dns64_aaaa);
		RESTORE(qctx->sigrdataset, qctx->client->query.dns64_sigaaaa);
		if (qctx->fname == NULL) {
			qctx->dbuf = ns_client_getnamebuf(qctx->client);
			if (qctx->dbuf == NULL) {
				CCTRACE(ISC_LOG_ERROR, "query_nodata: "
						       "ns_client_getnamebuf "
						       "failed (3)");
				QUERY_ERROR(qctx, ISC_R_NOMEMORY);
				return (ns_query_done(qctx));
			}
			qctx->fname = ns_client_newname(qctx->client,
							qctx->dbuf, &b);
			if (qctx->fname == NULL) {
				CCTRACE(ISC_LOG_ERROR, "query_nodata: "
						       "ns_client_newname "
						       "failed (3)");
				QUERY_ERROR(qctx, ISC_R_NOMEMORY);
				return (ns_query_done(qctx));
			}
		}
		dns_name_copynf(qctx->client->query.qname, qctx->fname);
		qctx->dns64 = false;
#ifdef dns64_bis_return_excluded_addresses
		/*
		 * Resume the diverted processing of the AAAA response?
		 */
		if (qctx->dns64_exclude) {
			return (query_prepresponse(qctx));
		}
#endif /* ifdef dns64_bis_return_excluded_addresses */
	} else if ((result == DNS_R_NXRRSET || result == DNS_R_NCACHENXRRSET) &&
		   !ISC_LIST_EMPTY(qctx->view->dns64) && !qctx->nxrewrite &&
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
			{
				qctx->client->query.dns64_ttl = 0;
			}
			break;
		case DNS_R_NXRRSET:
			qctx->client->query.dns64_ttl =
				dns64_ttl(qctx->db, qctx->version);
			break;
		default:
			UNREACHABLE();
		}

		SAVE(qctx->client->query.dns64_aaaa, qctx->rdataset);
		SAVE(qctx->client->query.dns64_sigaaaa, qctx->sigrdataset);
		ns_client_releasename(qctx->client, &qctx->fname);
		dns_db_detachnode(qctx->db, &qctx->node);
		qctx->type = qctx->qtype = dns_rdatatype_a;
		qctx->dns64 = true;
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
			ns_client_keepname(qctx->client, qctx->fname,
					   qctx->dbuf);
			dns_message_addname(qctx->client->message, qctx->fname,
					    DNS_SECTION_AUTHORITY);
			ISC_LIST_APPEND(qctx->fname->list, qctx->rdataset,
					link);
			qctx->fname = NULL;
			qctx->rdataset = NULL;
		}
	}

	return (ns_query_done(qctx));

cleanup:
	return (result);
}

/*%
 * Add RRSIGs for NOERROR/NODATA responses when answering authoritatively.
 */
isc_result_t
query_sign_nodata(query_ctx_t *qctx) {
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_sign_nodata");

	/*
	 * Look for a NSEC3 record if we don't have a NSEC record.
	 */
	if (qctx->redirected) {
		return (ns_query_done(qctx));
	}
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
					       true, found);
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
				query_addrrset(qctx, &qctx->fname,
					       &qctx->rdataset,
					       &qctx->sigrdataset, qctx->dbuf,
					       DNS_SECTION_AUTHORITY);

				count = dns_name_countlabels(found) + 1;
				skip = dns_name_countlabels(qname) - count;
				dns_name_getlabelsequence(qname, skip, count,
							  found);

				fixfname(qctx->client, &qctx->fname,
					 &qctx->dbuf, &b);
				fixrdataset(qctx->client, &qctx->rdataset);
				fixrdataset(qctx->client, &qctx->sigrdataset);
				if (qctx->fname == NULL ||
				    qctx->rdataset == NULL ||
				    qctx->sigrdataset == NULL)
				{
					CCTRACE(ISC_LOG_ERROR, "query_sign_"
							       "nodata: "
							       "failure "
							       "getting "
							       "closest "
							       "encloser");
					QUERY_ERROR(qctx, ISC_R_NOMEMORY);
					return (ns_query_done(qctx));
				}
				/*
				 * 'nearest' doesn't exist so
				 * 'exist' is set to false.
				 */
				query_findclosestnsec3(
					found, qctx->db, qctx->version,
					qctx->client, qctx->rdataset,
					qctx->sigrdataset, qctx->fname, false,
					NULL);
			}
		} else {
			ns_client_releasename(qctx->client, &qctx->fname);
			query_addwildcardproof(qctx, false, true);
		}
	}
	if (dns_rdataset_isassociated(qctx->rdataset)) {
		/*
		 * If we've got a NSEC record, we need to save the
		 * name now because we're going call query_addsoa()
		 * below, and it needs to use the name buffer.
		 */
		ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else if (qctx->fname != NULL) {
		/*
		 * We're not going to use fname, and need to release
		 * our hold on the name buffer so query_addsoa()
		 * may use it.
		 */
		ns_client_releasename(qctx->client, &qctx->fname);
	}

	/*
	 * The RPZ SOA has already been added to the additional section
	 * if this was an RPZ rewrite, but if it wasn't, add it now.
	 */
	if (!qctx->nxrewrite) {
		result = query_addsoa(qctx, UINT32_MAX, DNS_SECTION_AUTHORITY);
		if (result != ISC_R_SUCCESS) {
			QUERY_ERROR(qctx, result);
			return (ns_query_done(qctx));
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

	return (ns_query_done(qctx));
}

static void
query_addnxrrsetnsec(query_ctx_t *qctx) {
	ns_client_t *client = qctx->client;
	dns_rdata_t sigrdata;
	dns_rdata_rrsig_t sig;
	unsigned int labels;
	isc_buffer_t *dbuf, b;
	dns_name_t *fname;
	isc_result_t result;

	INSIST(qctx->fname != NULL);

	if ((qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) == 0) {
		query_addrrset(qctx, &qctx->fname, &qctx->rdataset,
			       &qctx->sigrdataset, NULL, DNS_SECTION_AUTHORITY);
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
	result = dns_rdata_tostruct(&sigrdata, &sig, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	labels = dns_name_countlabels(qctx->fname);
	if ((unsigned int)sig.labels + 1 >= labels) {
		return;
	}

	query_addwildcardproof(qctx, true, false);

	/*
	 * We'll need some resources...
	 */
	dbuf = ns_client_getnamebuf(client);
	if (dbuf == NULL) {
		return;
	}

	fname = ns_client_newname(client, dbuf, &b);
	if (fname == NULL) {
		return;
	}

	dns_name_split(qctx->fname, sig.labels + 1, NULL, fname);
	/* This will succeed, since we've stripped labels. */
	RUNTIME_CHECK(dns_name_concatenate(dns_wildcardname, fname, fname,
					   NULL) == ISC_R_SUCCESS);
	query_addrrset(qctx, &fname, &qctx->rdataset, &qctx->sigrdataset, dbuf,
		       DNS_SECTION_AUTHORITY);
}

/*%
 * Handle NXDOMAIN and empty wildcard responses.
 */
static isc_result_t
query_nxdomain(query_ctx_t *qctx, bool empty_wild) {
	dns_section_t section;
	uint32_t ttl;
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_nxdomain");

	CALL_HOOK(NS_QUERY_NXDOMAIN_BEGIN, qctx);

	INSIST(qctx->is_zone || REDIRECT(qctx->client));

	if (!empty_wild) {
		result = query_redirect(qctx);
		if (result != ISC_R_COMPLETE) {
			return (result);
		}
	}

	if (dns_rdataset_isassociated(qctx->rdataset)) {
		/*
		 * If we've got a NSEC record, we need to save the
		 * name now because we're going call query_addsoa()
		 * below, and it needs to use the name buffer.
		 */
		ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else if (qctx->fname != NULL) {
		/*
		 * We're not going to use fname, and need to release
		 * our hold on the name buffer so query_addsoa()
		 * may use it.
		 */
		ns_client_releasename(qctx->client, &qctx->fname);
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
	ttl = UINT32_MAX;
	if (!qctx->nxrewrite && qctx->qtype == dns_rdatatype_soa &&
	    qctx->zone != NULL && dns_zone_getzeronosoattl(qctx->zone))
	{
		ttl = 0;
	}
	if (!qctx->nxrewrite ||
	    (qctx->rpz_st != NULL && qctx->rpz_st->m.rpz->addsoa))
	{
		result = query_addsoa(qctx, ttl, section);
		if (result != ISC_R_SUCCESS) {
			QUERY_ERROR(qctx, result);
			return (ns_query_done(qctx));
		}
	}

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NSEC record if we found one.
		 */
		if (dns_rdataset_isassociated(qctx->rdataset)) {
			query_addrrset(qctx, &qctx->fname, &qctx->rdataset,
				       &qctx->sigrdataset, NULL,
				       DNS_SECTION_AUTHORITY);
		}
		query_addwildcardproof(qctx, false, false);
	}

	/*
	 * Set message rcode.
	 */
	if (empty_wild) {
		qctx->client->message->rcode = dns_rcode_noerror;
	} else {
		qctx->client->message->rcode = dns_rcode_nxdomain;
	}

	return (ns_query_done(qctx));

cleanup:
	return (result);
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
query_redirect(query_ctx_t *qctx) {
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_redirect");

	result = redirect(qctx->client, qctx->fname, qctx->rdataset,
			  &qctx->node, &qctx->db, &qctx->version, qctx->type);
	switch (result) {
	case ISC_R_SUCCESS:
		inc_stats(qctx->client, ns_statscounter_nxdomainredirect);
		return (query_prepresponse(qctx));
	case DNS_R_NXRRSET:
		qctx->redirected = true;
		qctx->is_zone = true;
		return (query_nodata(qctx, DNS_R_NXRRSET));
	case DNS_R_NCACHENXRRSET:
		qctx->redirected = true;
		qctx->is_zone = false;
		return (query_ncache(qctx, DNS_R_NCACHENXRRSET));
	default:
		break;
	}

	result = redirect2(qctx->client, qctx->fname, qctx->rdataset,
			   &qctx->node, &qctx->db, &qctx->version, qctx->type,
			   &qctx->is_zone);
	switch (result) {
	case ISC_R_SUCCESS:
		inc_stats(qctx->client, ns_statscounter_nxdomainredirect);
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
		dns_name_copynf(qctx->fname,
				qctx->client->query.redirect.fname);
		qctx->client->query.redirect.authoritative =
			qctx->authoritative;
		qctx->client->query.redirect.is_zone = qctx->is_zone;
		return (ns_query_done(qctx));
	case DNS_R_NXRRSET:
		qctx->redirected = true;
		qctx->is_zone = true;
		return (query_nodata(qctx, DNS_R_NXRRSET));
	case DNS_R_NCACHENXRRSET:
		qctx->redirected = true;
		qctx->is_zone = false;
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
	ns_client_logv(qctx->client, NS_LOGCATEGORY_QUERIES, NS_LOGMODULE_QUERY,
		       level, fmt, ap);
	va_end(ap);
}

static dns_ttl_t
query_synthttl(dns_rdataset_t *soardataset, dns_rdataset_t *sigsoardataset,
	       dns_rdataset_t *p1rdataset, dns_rdataset_t *sigp1rdataset,
	       dns_rdataset_t *p2rdataset, dns_rdataset_t *sigp2rdataset) {
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
	result = dns_rdata_tostruct(&rdata, &soa, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);

	ttl = ISC_MIN(soa.minimum, soardataset->ttl);
	ttl = ISC_MIN(ttl, sigsoardataset->ttl);
	ttl = ISC_MIN(ttl, p1rdataset->ttl);
	ttl = ISC_MIN(ttl, sigp1rdataset->ttl);
	if (p2rdataset != NULL) {
		ttl = ISC_MIN(ttl, p2rdataset->ttl);
	}
	if (sigp2rdataset != NULL) {
		ttl = ISC_MIN(ttl, sigp2rdataset->ttl);
	}

	return (ttl);
}

/*
 * Synthesize a NODATA response from the SOA and covering NSEC in cache.
 */
static isc_result_t
query_synthnodata(query_ctx_t *qctx, const dns_name_t *signer,
		  dns_rdataset_t **soardatasetp,
		  dns_rdataset_t **sigsoardatasetp) {
	dns_name_t *name = NULL;
	dns_ttl_t ttl;
	isc_buffer_t *dbuf, b;
	isc_result_t result;

	/*
	 * Determine the correct TTL to use for the SOA and RRSIG
	 */
	ttl = query_synthttl(*soardatasetp, *sigsoardatasetp, qctx->rdataset,
			     qctx->sigrdataset, NULL, NULL);
	(*soardatasetp)->ttl = (*sigsoardatasetp)->ttl = ttl;

	/*
	 * We want the SOA record to be first, so save the
	 * NODATA proof's name now or else discard it.
	 */
	if (WANTDNSSEC(qctx->client)) {
		ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else {
		ns_client_releasename(qctx->client, &qctx->fname);
	}

	dbuf = ns_client_getnamebuf(qctx->client);
	if (dbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	name = ns_client_newname(qctx->client, dbuf, &b);
	if (name == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	dns_name_copynf(signer, name);

	/*
	 * Add SOA record. Omit the RRSIG if DNSSEC was not requested.
	 */
	if (!WANTDNSSEC(qctx->client)) {
		sigsoardatasetp = NULL;
	}
	query_addrrset(qctx, &name, soardatasetp, sigsoardatasetp, dbuf,
		       DNS_SECTION_AUTHORITY);

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NODATA proof.
		 */
		query_addrrset(qctx, &qctx->fname, &qctx->rdataset,
			       &qctx->sigrdataset, NULL, DNS_SECTION_AUTHORITY);
	}

	result = ISC_R_SUCCESS;
	inc_stats(qctx->client, ns_statscounter_nodatasynth);

cleanup:
	if (name != NULL) {
		ns_client_releasename(qctx->client, &name);
	}
	return (result);
}

/*
 * Synthesize a wildcard answer using the contents of 'rdataset'.
 * qctx contains the NODATA proof.
 */
static isc_result_t
query_synthwildcard(query_ctx_t *qctx, dns_rdataset_t *rdataset,
		    dns_rdataset_t *sigrdataset) {
	dns_name_t *name = NULL;
	isc_buffer_t *dbuf, b;
	isc_result_t result;
	dns_rdataset_t *cloneset = NULL, *clonesigset = NULL;
	dns_rdataset_t **sigrdatasetp;

	CCTRACE(ISC_LOG_DEBUG(3), "query_synthwildcard");

	/*
	 * We want the answer to be first, so save the
	 * NOQNAME proof's name now or else discard it.
	 */
	if (WANTDNSSEC(qctx->client)) {
		ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else {
		ns_client_releasename(qctx->client, &qctx->fname);
	}

	dbuf = ns_client_getnamebuf(qctx->client);
	if (dbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	name = ns_client_newname(qctx->client, dbuf, &b);
	if (name == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	dns_name_copynf(qctx->client->query.qname, name);

	cloneset = ns_client_newrdataset(qctx->client);
	if (cloneset == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}
	dns_rdataset_clone(rdataset, cloneset);

	/*
	 * Add answer RRset. Omit the RRSIG if DNSSEC was not requested.
	 */
	if (WANTDNSSEC(qctx->client)) {
		clonesigset = ns_client_newrdataset(qctx->client);
		if (clonesigset == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		dns_rdataset_clone(sigrdataset, clonesigset);
		sigrdatasetp = &clonesigset;
	} else {
		sigrdatasetp = NULL;
	}

	query_addrrset(qctx, &name, &cloneset, sigrdatasetp, dbuf,
		       DNS_SECTION_ANSWER);

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NOQNAME proof.
		 */
		query_addrrset(qctx, &qctx->fname, &qctx->rdataset,
			       &qctx->sigrdataset, NULL, DNS_SECTION_AUTHORITY);
	}

	result = ISC_R_SUCCESS;
	inc_stats(qctx->client, ns_statscounter_wildcardsynth);

cleanup:
	if (name != NULL) {
		ns_client_releasename(qctx->client, &name);
	}
	if (cloneset != NULL) {
		ns_client_putrdataset(qctx->client, &cloneset);
	}
	if (clonesigset != NULL) {
		ns_client_putrdataset(qctx->client, &clonesigset);
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
			 dns_rdataset_t *sigrdataset) {
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
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	dns_rdata_reset(&rdata);

	dns_name_copynf(&cname.cname, tname);

	dns_rdata_freestruct(&cname);
	ns_client_qnamereplace(qctx->client, tname);
	qctx->want_restart = true;
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
query_synthnxdomain(query_ctx_t *qctx, dns_name_t *nowild,
		    dns_rdataset_t *nowildrdataset,
		    dns_rdataset_t *signowildrdataset, dns_name_t *signer,
		    dns_rdataset_t **soardatasetp,
		    dns_rdataset_t **sigsoardatasetp) {
	dns_name_t *name = NULL;
	dns_ttl_t ttl;
	isc_buffer_t *dbuf, b;
	isc_result_t result;
	dns_rdataset_t *cloneset = NULL, *clonesigset = NULL;

	CCTRACE(ISC_LOG_DEBUG(3), "query_synthnxdomain");

	/*
	 * Determine the correct TTL to use for the SOA and RRSIG
	 */
	ttl = query_synthttl(*soardatasetp, *sigsoardatasetp, qctx->rdataset,
			     qctx->sigrdataset, nowildrdataset,
			     signowildrdataset);
	(*soardatasetp)->ttl = (*sigsoardatasetp)->ttl = ttl;

	/*
	 * We want the SOA record to be first, so save the
	 * NOQNAME proof's name now or else discard it.
	 */
	if (WANTDNSSEC(qctx->client)) {
		ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);
	} else {
		ns_client_releasename(qctx->client, &qctx->fname);
	}

	dbuf = ns_client_getnamebuf(qctx->client);
	if (dbuf == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	name = ns_client_newname(qctx->client, dbuf, &b);
	if (name == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	dns_name_copynf(signer, name);

	/*
	 * Add SOA record. Omit the RRSIG if DNSSEC was not requested.
	 */
	if (!WANTDNSSEC(qctx->client)) {
		sigsoardatasetp = NULL;
	}
	query_addrrset(qctx, &name, soardatasetp, sigsoardatasetp, dbuf,
		       DNS_SECTION_AUTHORITY);

	if (WANTDNSSEC(qctx->client)) {
		/*
		 * Add NOQNAME proof.
		 */
		query_addrrset(qctx, &qctx->fname, &qctx->rdataset,
			       &qctx->sigrdataset, NULL, DNS_SECTION_AUTHORITY);

		dbuf = ns_client_getnamebuf(qctx->client);
		if (dbuf == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		name = ns_client_newname(qctx->client, dbuf, &b);
		if (name == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		dns_name_copynf(nowild, name);

		cloneset = ns_client_newrdataset(qctx->client);
		clonesigset = ns_client_newrdataset(qctx->client);
		if (cloneset == NULL || clonesigset == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		dns_rdataset_clone(nowildrdataset, cloneset);
		dns_rdataset_clone(signowildrdataset, clonesigset);

		/*
		 * Add NOWILDCARD proof.
		 */
		query_addrrset(qctx, &name, &cloneset, &clonesigset, dbuf,
			       DNS_SECTION_AUTHORITY);
	}

	qctx->client->message->rcode = dns_rcode_nxdomain;
	result = ISC_R_SUCCESS;
	inc_stats(qctx->client, ns_statscounter_nxdomainsynth);

cleanup:
	if (name != NULL) {
		ns_client_releasename(qctx->client, &name);
	}
	if (cloneset != NULL) {
		ns_client_putrdataset(qctx->client, &cloneset);
	}
	if (clonesigset != NULL) {
		ns_client_putrdataset(qctx->client, &clonesigset);
	}
	return (result);
}

/*
 * Check that all signer names in sigrdataset match the expected signer.
 */
static isc_result_t
checksignames(dns_name_t *signer, dns_rdataset_t *sigrdataset) {
	isc_result_t result;

	for (result = dns_rdataset_first(sigrdataset); result == ISC_R_SUCCESS;
	     result = dns_rdataset_next(sigrdataset))
	{
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_rrsig_t rrsig;

		dns_rdataset_current(sigrdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &rrsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		if (dns_name_countlabels(signer) == 0) {
			dns_name_copynf(&rrsig.signer, signer);
		} else if (!dns_name_equal(signer, &rrsig.signer)) {
			return (ISC_R_FAILURE);
		}
	}

	return (ISC_R_SUCCESS);
}

/*%
 * Handle covering NSEC responses.
 *
 * Verify the NSEC record is appropriate for the QNAME; if not,
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
	bool done = false;
	bool exists = true, data = true;
	bool redirected = false;
	isc_result_t result = ISC_R_SUCCESS;
	unsigned int dboptions = qctx->client->query.dboptions;

	CCTRACE(ISC_LOG_DEBUG(3), "query_coveringnsec");

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
	dns_clientinfo_init(&ci, qctx->client, NULL, NULL);

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
					qctx->fname, qctx->rdataset, &exists,
					&data, wild, log_noexistnodata, qctx);

	if (result != ISC_R_SUCCESS || (exists && data)) {
		goto cleanup;
	}

	if (exists) {
		if (qctx->type == dns_rdatatype_any) { /* XXX not yet */
			goto cleanup;
		}
		if (!ISC_LIST_EMPTY(qctx->view->dns64) &&
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

		soardataset = ns_client_newrdataset(qctx->client);
		sigsoardataset = ns_client_newrdataset(qctx->client);
		if (soardataset == NULL || sigsoardataset == NULL) {
			goto cleanup;
		}

		/*
		 * Look for SOA record to construct NODATA response.
		 */
		dns_db_attach(qctx->db, &db);
		result = dns_db_findext(db, signer, qctx->version,
					dns_rdatatype_soa, dboptions,
					qctx->client->now, &node, fname, &cm,
					&ci, soardataset, sigsoardataset);

		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}
		(void)query_synthnodata(qctx, signer, &soardataset,
					&sigsoardataset);
		done = true;
		goto cleanup;
	}

	/*
	 * Look up the no-wildcard proof.
	 */
	dns_db_attach(qctx->db, &db);
	result = dns_db_findext(db, wild, qctx->version, qctx->type,
				dboptions | DNS_DBFIND_COVERINGNSEC,
				qctx->client->now, &node, nowild, &cm, &ci,
				&rdataset, &sigrdataset);

	if (rdataset.trust != dns_trust_secure ||
	    sigrdataset.trust != dns_trust_secure)
	{
		goto cleanup;
	}

	/*
	 * Zero TTL handling of wildcard record.
	 *
	 * We don't yet have code to handle synthesis and type ANY or dns64
	 * processing so we abort the synthesis here if there would be a
	 * interaction.
	 */
	switch (result) {
	case ISC_R_SUCCESS:
		if (qctx->type == dns_rdatatype_any) { /* XXX not yet */
			goto cleanup;
		}
		if (!ISC_LIST_EMPTY(qctx->view->dns64) &&
		    (qctx->type == dns_rdatatype_a ||
		     qctx->type == dns_rdatatype_aaaa)) /* XXX not yet */
		{
			goto cleanup;
		}
		FALLTHROUGH;
	case DNS_R_CNAME:
		if (!qctx->resuming && !STALE(&rdataset) && rdataset.ttl == 0 &&
		    RECURSIONOK(qctx->client))
		{
			goto cleanup;
		}
	default:
		break;
	}

	switch (result) {
	case DNS_R_COVERINGNSEC:
		result = dns_nsec_noexistnodata(qctx->qtype, wild, nowild,
						&rdataset, &exists, &data, NULL,
						log_noexistnodata, qctx);
		if (result != ISC_R_SUCCESS || exists) {
			goto cleanup;
		}
		break;
	case ISC_R_SUCCESS: /* wild card match */
		(void)query_synthwildcard(qctx, &rdataset, &sigrdataset);
		done = true;
		goto cleanup;
	case DNS_R_CNAME: /* wild card cname */
		(void)query_synthcnamewildcard(qctx, &rdataset, &sigrdataset);
		done = true;
		goto cleanup;
	case DNS_R_NCACHENXRRSET:  /* wild card nodata */
	case DNS_R_NCACHENXDOMAIN: /* direct nxdomain */
	default:
		goto cleanup;
	}

	/*
	 * We now have the proof that we have an NXDOMAIN.  Apply
	 * NXDOMAIN redirection if configured.
	 */
	result = query_redirect(qctx);
	if (result != ISC_R_COMPLETE) {
		redirected = true;
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

	soardataset = ns_client_newrdataset(qctx->client);
	sigsoardataset = ns_client_newrdataset(qctx->client);
	if (soardataset == NULL || sigsoardataset == NULL) {
		goto cleanup;
	}

	/*
	 * Look for SOA record to construct NXDOMAIN response.
	 */
	result = dns_db_findext(db, signer, qctx->version, dns_rdatatype_soa,
				dboptions, qctx->client->now, &node, fname, &cm,
				&ci, soardataset, sigsoardataset);

	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	(void)query_synthnxdomain(qctx, nowild, &rdataset, &sigrdataset, signer,
				  &soardataset, &sigsoardataset);
	done = true;

cleanup:
	if (dns_rdataset_isassociated(&rdataset)) {
		dns_rdataset_disassociate(&rdataset);
	}
	if (dns_rdataset_isassociated(&sigrdataset)) {
		dns_rdataset_disassociate(&sigrdataset);
	}
	if (soardataset != NULL) {
		ns_client_putrdataset(qctx->client, &soardataset);
	}
	if (sigsoardataset != NULL) {
		ns_client_putrdataset(qctx->client, &sigsoardataset);
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
		qctx->findcoveringnsec = false;
		if (qctx->fname != NULL) {
			ns_client_releasename(qctx->client, &qctx->fname);
		}
		if (qctx->node != NULL) {
			dns_db_detachnode(qctx->db, &qctx->node);
		}
		ns_client_putrdataset(qctx->client, &qctx->rdataset);
		if (qctx->sigrdataset != NULL) {
			ns_client_putrdataset(qctx->client, &qctx->sigrdataset);
		}
		return (query_lookup(qctx));
	}

	return (ns_query_done(qctx));
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
	       result == DNS_R_NCACHENXRRSET || result == DNS_R_NXDOMAIN);

	CCTRACE(ISC_LOG_DEBUG(3), "query_ncache");

	CALL_HOOK(NS_QUERY_NCACHE_BEGIN, qctx);

	qctx->authoritative = false;

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

cleanup:
	return (result);
}

/*
 * If we have a zero ttl from the cache, refetch.
 */
static isc_result_t
query_zerottl_refetch(query_ctx_t *qctx) {
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_zerottl_refetch");

	if (qctx->is_zone || qctx->resuming || STALE(qctx->rdataset) ||
	    qctx->rdataset->ttl != 0 || !RECURSIONOK(qctx->client))
	{
		return (ISC_R_COMPLETE);
	}

	qctx_clean(qctx);

	INSIST(!REDIRECT(qctx->client));

	result = ns_query_recurse(qctx->client, qctx->qtype,
				  qctx->client->query.qname, NULL, NULL,
				  qctx->resuming);
	if (result == ISC_R_SUCCESS) {
		CALL_HOOK(NS_QUERY_ZEROTTL_RECURSE, qctx);
		qctx->client->query.attributes |= NS_QUERYATTR_RECURSING;

		if (qctx->dns64) {
			qctx->client->query.attributes |= NS_QUERYATTR_DNS64;
		}
		if (qctx->dns64_exclude) {
			qctx->client->query.attributes |=
				NS_QUERYATTR_DNS64EXCLUDE;
		}
	} else {
		/*
		 * There was a zero ttl from the cache, don't fallback to
		 * serve-stale lookup.
		 */
		QUERY_ERROR(qctx, result);
	}

	return (ns_query_done(qctx));

cleanup:
	return (result);
}

/*
 * Handle CNAME responses.
 */
static isc_result_t
query_cname(query_ctx_t *qctx) {
	isc_result_t result = ISC_R_UNSET;
	dns_name_t *tname = NULL;
	dns_rdataset_t *trdataset = NULL;
	dns_rdataset_t **sigrdatasetp = NULL;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_cname_t cname;

	CCTRACE(ISC_LOG_DEBUG(3), "query_cname");

	CALL_HOOK(NS_QUERY_CNAME_BEGIN, qctx);

	result = query_zerottl_refetch(qctx);
	if (result != ISC_R_COMPLETE) {
		return (result);
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
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL) {
		sigrdatasetp = &qctx->sigrdataset;
	}

	if (WANTDNSSEC(qctx->client) &&
	    (qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) != 0)
	{
		dns_fixedname_init(&qctx->wildcardname);
		dns_name_copynf(qctx->fname,
				dns_fixedname_name(&qctx->wildcardname));
		qctx->need_wildcardproof = true;
	}

	if (NOQNAME(qctx->rdataset) && WANTDNSSEC(qctx->client)) {
		qctx->noqname = qctx->rdataset;
	} else {
		qctx->noqname = NULL;
	}

	if (!qctx->is_zone && RECURSIONOK(qctx->client)) {
		query_prefetch(qctx->client, qctx->fname, qctx->rdataset);
	}

	query_addrrset(qctx, &qctx->fname, &qctx->rdataset, sigrdatasetp,
		       qctx->dbuf, DNS_SECTION_ANSWER);

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
	result = dns_message_gettempname(qctx->client->message, &tname);
	if (result != ISC_R_SUCCESS) {
		return (ns_query_done(qctx));
	}

	result = dns_rdataset_first(trdataset);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (ns_query_done(qctx));
	}

	dns_rdataset_current(trdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &cname, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	dns_rdata_reset(&rdata);

	dns_name_copynf(&cname.cname, tname);

	dns_rdata_freestruct(&cname);
	ns_client_qnamereplace(qctx->client, tname);
	qctx->want_restart = true;
	if (!WANTRECURSION(qctx->client)) {
		qctx->options |= DNS_GETDB_NOLOG;
	}

	query_addauth(qctx);

	return (ns_query_done(qctx));

cleanup:
	return (result);
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

	CCTRACE(ISC_LOG_DEBUG(3), "query_dname");

	CALL_HOOK(NS_QUERY_DNAME_BEGIN, qctx);

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
	if (WANTDNSSEC(qctx->client) && qctx->sigrdataset != NULL) {
		sigrdatasetp = &qctx->sigrdataset;
	}

	if (WANTDNSSEC(qctx->client) &&
	    (qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) != 0)
	{
		dns_fixedname_init(&qctx->wildcardname);
		dns_name_copynf(qctx->fname,
				dns_fixedname_name(&qctx->wildcardname));
		qctx->need_wildcardproof = true;
	}

	if (!qctx->is_zone && RECURSIONOK(qctx->client)) {
		query_prefetch(qctx->client, qctx->fname, qctx->rdataset);
	}
	query_addrrset(qctx, &qctx->fname, &qctx->rdataset, sigrdatasetp,
		       qctx->dbuf, DNS_SECTION_ANSWER);

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
	if (result != ISC_R_SUCCESS) {
		return (ns_query_done(qctx));
	}

	result = dns_rdataset_first(trdataset);
	if (result != ISC_R_SUCCESS) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (ns_query_done(qctx));
	}

	dns_rdataset_current(trdataset, &rdata);
	result = dns_rdata_tostruct(&rdata, &dname, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	dns_rdata_reset(&rdata);

	dns_name_copynf(&dname.dname, tname);
	dns_rdata_freestruct(&dname);

	/*
	 * Construct the new qname consisting of
	 * <found name prefix>.<dname target>
	 */
	prefix = dns_fixedname_initname(&fixed);
	dns_name_split(qctx->client->query.qname, nlabels, prefix, NULL);
	INSIST(qctx->fname == NULL);
	qctx->dbuf = ns_client_getnamebuf(qctx->client);
	if (qctx->dbuf == NULL) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (ns_query_done(qctx));
	}
	qctx->fname = ns_client_newname(qctx->client, qctx->dbuf, &b);
	if (qctx->fname == NULL) {
		dns_message_puttempname(qctx->client->message, &tname);
		return (ns_query_done(qctx));
	}
	result = dns_name_concatenate(prefix, tname, qctx->fname, NULL);
	dns_message_puttempname(qctx->client->message, &tname);

	/*
	 * RFC2672, section 4.1, subsection 3c says
	 * we should return YXDOMAIN if the constructed
	 * name would be too long.
	 */
	if (result == DNS_R_NAMETOOLONG) {
		qctx->client->message->rcode = dns_rcode_yxdomain;
	}
	if (result != ISC_R_SUCCESS) {
		return (ns_query_done(qctx));
	}

	ns_client_keepname(qctx->client, qctx->fname, qctx->dbuf);

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
	if (result != ISC_R_SUCCESS) {
		return (ns_query_done(qctx));
	}

	/*
	 * If the original query was not for a CNAME or ANY then follow the
	 * CNAME.
	 */
	if (qctx->qtype != dns_rdatatype_cname &&
	    qctx->qtype != dns_rdatatype_any)
	{
		/*
		 * Switch to the new qname and restart.
		 */
		ns_client_qnamereplace(qctx->client, qctx->fname);
		qctx->fname = NULL;
		qctx->want_restart = true;
		if (!WANTRECURSION(qctx->client)) {
			qctx->options |= DNS_GETDB_NOLOG;
		}
	}

	query_addauth(qctx);

	return (ns_query_done(qctx));

cleanup:
	return (result);
}

/*%
 * Add CNAME to response.
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
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	dns_name_copynf(client->query.qname, aname);

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
	RUNTIME_CHECK(dns_rdatalist_tordataset(rdatalist, rdataset) ==
		      ISC_R_SUCCESS);
	rdataset->trust = trust;
	dns_rdataset_setownercase(rdataset, aname);

	query_addrrset(qctx, &aname, &rdataset, NULL, NULL, DNS_SECTION_ANSWER);
	if (rdataset != NULL) {
		if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}
		dns_message_puttemprdataset(client->message, &rdataset);
	}
	if (aname != NULL) {
		dns_message_puttempname(client->message, &aname);
	}

	return (ISC_R_SUCCESS);
}

/*%
 * Prepare to respond: determine whether a wildcard proof is needed,
 * then hand off to query_respond() or (for type ANY queries)
 * query_respond_any().
 */
static isc_result_t
query_prepresponse(query_ctx_t *qctx) {
	isc_result_t result;

	CCTRACE(ISC_LOG_DEBUG(3), "query_prepresponse");

	CALL_HOOK(NS_QUERY_PREP_RESPONSE_BEGIN, qctx);

	if (WANTDNSSEC(qctx->client) &&
	    (qctx->fname->attributes & DNS_NAMEATTR_WILDCARD) != 0)
	{
		dns_fixedname_init(&qctx->wildcardname);
		dns_name_copynf(qctx->fname,
				dns_fixedname_name(&qctx->wildcardname));
		qctx->need_wildcardproof = true;
	}

	if (qctx->type == dns_rdatatype_any) {
		return (query_respond_any(qctx));
	}

	result = query_zerottl_refetch(qctx);
	if (result != ISC_R_COMPLETE) {
		return (result);
	}

	return (query_respond(qctx));

cleanup:
	return (result);
}

/*%
 * Add SOA to the authority section when sending negative responses
 * (or to the additional section if sending negative responses triggered
 * by RPZ rewriting.)
 */
static isc_result_t
query_addsoa(query_ctx_t *qctx, unsigned int override_ttl,
	     dns_section_t section) {
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
	dns_clientinfo_init(&ci, client, NULL, NULL);

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
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * We'll be releasing 'name' before returning, so it's safe to
	 * use clone instead of copying here.
	 */
	dns_name_clone(dns_db_origin(qctx->db), name);

	rdataset = ns_client_newrdataset(client);
	if (rdataset == NULL) {
		CTRACE(ISC_LOG_ERROR, "unable to allocate rdataset");
		eresult = DNS_R_SERVFAIL;
		goto cleanup;
	}
	if (WANTDNSSEC(client) && dns_db_issecure(qctx->db)) {
		sigrdataset = ns_client_newrdataset(client);
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
					     dns_rdatatype_soa, 0, client->now,
					     rdataset, sigrdataset);
	} else {
		dns_fixedname_t foundname;
		dns_name_t *fname;

		fname = dns_fixedname_initname(&foundname);

		result = dns_db_findext(qctx->db, name, qctx->version,
					dns_rdatatype_soa,
					client->query.dboptions, 0, &node,
					fname, &cm, &ci, rdataset, sigrdataset);
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
		RUNTIME_CHECK(result == ISC_R_SUCCESS);

		if (override_ttl != UINT32_MAX && override_ttl < rdataset->ttl)
		{
			rdataset->ttl = override_ttl;
			if (sigrdataset != NULL) {
				sigrdataset->ttl = override_ttl;
			}
		}

		/*
		 * Add the SOA and its SIG to the response, with the
		 * TTLs adjusted per RFC2308 section 3.
		 */
		if (rdataset->ttl > soa.minimum) {
			rdataset->ttl = soa.minimum;
		}
		if (sigrdataset != NULL && sigrdataset->ttl > soa.minimum) {
			sigrdataset->ttl = soa.minimum;
		}

		if (sigrdataset != NULL) {
			sigrdatasetp = &sigrdataset;
		} else {
			sigrdatasetp = NULL;
		}

		if (section == DNS_SECTION_ADDITIONAL) {
			rdataset->attributes |= DNS_RDATASETATTR_REQUIRED;
		}
		query_addrrset(qctx, &name, &rdataset, sigrdatasetp, NULL,
			       section);
	}

cleanup:
	ns_client_putrdataset(client, &rdataset);
	if (sigrdataset != NULL) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (name != NULL) {
		ns_client_releasename(client, &name);
	}
	if (node != NULL) {
		dns_db_detachnode(qctx->db, &node);
	}

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
	dns_clientinfo_init(&ci, client, NULL, NULL);

	/*
	 * Get resources and make 'name' be the database origin.
	 */
	result = dns_message_gettempname(client->message, &name);
	if (result != ISC_R_SUCCESS) {
		CTRACE(ISC_LOG_DEBUG(3), "query_addns: dns_message_gettempname "
					 "failed: done");
		return (result);
	}
	dns_name_clone(dns_db_origin(qctx->db), name);
	rdataset = ns_client_newrdataset(client);
	if (rdataset == NULL) {
		CTRACE(ISC_LOG_ERROR, "query_addns: ns_client_newrdataset "
				      "failed");
		eresult = DNS_R_SERVFAIL;
		goto cleanup;
	}

	if (WANTDNSSEC(client) && dns_db_issecure(qctx->db)) {
		sigrdataset = ns_client_newrdataset(client);
		if (sigrdataset == NULL) {
			CTRACE(ISC_LOG_ERROR, "query_addns: "
					      "ns_client_newrdataset failed");
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
		CTRACE(ISC_LOG_ERROR, "query_addns: "
				      "dns_db_findrdataset or dns_db_find "
				      "failed");
		/*
		 * This is bad.  We tried to get the NS rdataset at the zone
		 * top and it didn't work!
		 */
		eresult = DNS_R_SERVFAIL;
	} else {
		if (sigrdataset != NULL) {
			sigrdatasetp = &sigrdataset;
		}
		query_addrrset(qctx, &name, &rdataset, sigrdatasetp, NULL,
			       DNS_SECTION_AUTHORITY);
	}

cleanup:
	CTRACE(ISC_LOG_DEBUG(3), "query_addns: cleanup");
	ns_client_putrdataset(client, &rdataset);
	if (sigrdataset != NULL) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (name != NULL) {
		ns_client_releasename(client, &name);
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
	bool is_zone = false, use_zone = false;
	isc_buffer_t *dbuf = NULL;
	isc_result_t result;
	dns_dbversion_t *version = NULL;
	dns_zone_t *zone = NULL;
	isc_buffer_t b;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	CTRACE(ISC_LOG_DEBUG(3), "query_addbestns");

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

	/*
	 * Find the right database.
	 */
	result = query_getdb(client, client->query.qname, dns_rdatatype_ns, 0,
			     &zone, &db, &version, &is_zone);
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

db_find:
	/*
	 * We'll need some resources...
	 */
	dbuf = ns_client_getnamebuf(client);
	if (dbuf == NULL) {
		goto cleanup;
	}
	fname = ns_client_newname(client, dbuf, &b);
	rdataset = ns_client_newrdataset(client);
	if (fname == NULL || rdataset == NULL) {
		goto cleanup;
	}

	/*
	 * Get the RRSIGs if the client requested them or if we may
	 * need to validate answers from the cache.
	 */
	if (WANTDNSSEC(client) || !is_zone) {
		sigrdataset = ns_client_newrdataset(client);
		if (sigrdataset == NULL) {
			goto cleanup;
		}
	}

	/*
	 * Now look for the zonecut.
	 */
	if (is_zone) {
		result = dns_db_findext(
			db, client->query.qname, version, dns_rdatatype_ns,
			client->query.dboptions, client->now, &node, fname, &cm,
			&ci, rdataset, sigrdataset);
		if (result != DNS_R_DELEGATION) {
			goto cleanup;
		}
		if (USECACHE(client)) {
			ns_client_keepname(client, fname, dbuf);
			dns_db_detachnode(db, &node);
			SAVE(zdb, db);
			SAVE(zfname, fname);
			SAVE(zrdataset, rdataset);
			SAVE(zsigrdataset, sigrdataset);
			version = NULL;
			dns_db_attach(client->view->cachedb, &db);
			is_zone = false;
			goto db_find;
		}
	} else {
		result = dns_db_findzonecut(
			db, client->query.qname, client->query.dboptions,
			client->now, &node, fname, NULL, rdataset, sigrdataset);
		if (result == ISC_R_SUCCESS) {
			if (zfname != NULL &&
			    !dns_name_issubdomain(fname, zfname))
			{
				/*
				 * We found a zonecut in the cache, but our
				 * zone delegation is better.
				 */
				use_zone = true;
			}
		} else if (result == ISC_R_NOTFOUND && zfname != NULL) {
			/*
			 * We didn't find anything in the cache, but we
			 * have a zone delegation, so use it.
			 */
			use_zone = true;
		} else {
			goto cleanup;
		}
	}

	if (use_zone) {
		ns_client_releasename(client, &fname);
		/*
		 * We've already done ns_client_keepname() on
		 * zfname, so we must set dbuf to NULL to
		 * prevent query_addrrset() from trying to
		 * call ns_client_keepname() again.
		 */
		dbuf = NULL;
		ns_client_putrdataset(client, &rdataset);
		if (sigrdataset != NULL) {
			ns_client_putrdataset(client, &sigrdataset);
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
		ns_client_putrdataset(client, &sigrdataset);
	}

	query_addrrset(qctx, &fname, &rdataset, &sigrdataset, dbuf,
		       DNS_SECTION_AUTHORITY);

cleanup:
	if (rdataset != NULL) {
		ns_client_putrdataset(client, &rdataset);
	}
	if (sigrdataset != NULL) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (fname != NULL) {
		ns_client_releasename(client, &fname);
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
		ns_client_putrdataset(client, &zrdataset);
		if (zsigrdataset != NULL) {
			ns_client_putrdataset(client, &zsigrdataset);
		}
		if (zfname != NULL) {
			ns_client_releasename(client, &zfname);
		}
		dns_db_detach(&zdb);
	}
}

static void
query_addwildcardproof(query_ctx_t *qctx, bool ispositive, bool nodata) {
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
	bool have_wname;
	int order;
	dns_fixedname_t cfixed;
	dns_name_t *cname;
	dns_clientinfomethods_t cm;
	dns_clientinfo_t ci;

	CTRACE(ISC_LOG_DEBUG(3), "query_addwildcardproof");

	dns_clientinfomethods_init(&cm, ns_client_sourceip);
	dns_clientinfo_init(&ci, client, NULL, NULL);

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
	have_wname = false;
	/*
	 * We'll need some resources...
	 */
	dbuf = ns_client_getnamebuf(client);
	if (dbuf == NULL) {
		goto cleanup;
	}
	fname = ns_client_newname(client, dbuf, &b);
	rdataset = ns_client_newrdataset(client);
	sigrdataset = ns_client_newrdataset(client);
	if (fname == NULL || rdataset == NULL || sigrdataset == NULL) {
		goto cleanup;
	}

	result = dns_db_findext(qctx->db, name, qctx->version,
				dns_rdatatype_nsec, options, 0, &node, fname,
				&cm, &ci, rdataset, sigrdataset);
	if (node != NULL) {
		dns_db_detachnode(qctx->db, &node);
	}

	if (!dns_rdataset_isassociated(rdataset)) {
		/*
		 * No NSEC proof available, return NSEC3 proofs instead.
		 */
		cname = dns_fixedname_initname(&cfixed);
		/*
		 * Find the closest encloser.
		 */
		dns_name_copynf(name, cname);
		while (result == DNS_R_NXDOMAIN) {
			labels = dns_name_countlabels(cname) - 1;
			/*
			 * Sanity check.
			 */
			if (labels == 0U) {
				goto cleanup;
			}
			dns_name_split(cname, labels, NULL, cname);
			result = dns_db_findext(qctx->db, cname, qctx->version,
						dns_rdatatype_nsec, options, 0,
						NULL, fname, &cm, &ci, NULL,
						NULL);
		}
		/*
		 * Add closest (provable) encloser NSEC3.
		 */
		query_findclosestnsec3(cname, qctx->db, qctx->version, client,
				       rdataset, sigrdataset, fname, true,
				       cname);
		if (!dns_rdataset_isassociated(rdataset)) {
			goto cleanup;
		}
		if (!ispositive) {
			query_addrrset(qctx, &fname, &rdataset, &sigrdataset,
				       dbuf, DNS_SECTION_AUTHORITY);
		}

		/*
		 * Replace resources which were consumed by query_addrrset.
		 */
		if (fname == NULL) {
			dbuf = ns_client_getnamebuf(client);
			if (dbuf == NULL) {
				goto cleanup;
			}
			fname = ns_client_newname(client, dbuf, &b);
		}

		if (rdataset == NULL) {
			rdataset = ns_client_newrdataset(client);
		} else if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}

		if (sigrdataset == NULL) {
			sigrdataset = ns_client_newrdataset(client);
		} else if (dns_rdataset_isassociated(sigrdataset)) {
			dns_rdataset_disassociate(sigrdataset);
		}

		if (fname == NULL || rdataset == NULL || sigrdataset == NULL) {
			goto cleanup;
		}
		/*
		 * Add no qname proof.
		 */
		labels = dns_name_countlabels(cname) + 1;
		if (dns_name_countlabels(name) == labels) {
			dns_name_copynf(name, wname);
		} else {
			dns_name_split(name, labels, NULL, wname);
		}

		query_findclosestnsec3(wname, qctx->db, qctx->version, client,
				       rdataset, sigrdataset, fname, false,
				       NULL);
		if (!dns_rdataset_isassociated(rdataset)) {
			goto cleanup;
		}
		query_addrrset(qctx, &fname, &rdataset, &sigrdataset, dbuf,
			       DNS_SECTION_AUTHORITY);

		if (ispositive) {
			goto cleanup;
		}

		/*
		 * Replace resources which were consumed by query_addrrset.
		 */
		if (fname == NULL) {
			dbuf = ns_client_getnamebuf(client);
			if (dbuf == NULL) {
				goto cleanup;
			}
			fname = ns_client_newname(client, dbuf, &b);
		}

		if (rdataset == NULL) {
			rdataset = ns_client_newrdataset(client);
		} else if (dns_rdataset_isassociated(rdataset)) {
			dns_rdataset_disassociate(rdataset);
		}

		if (sigrdataset == NULL) {
			sigrdataset = ns_client_newrdataset(client);
		} else if (dns_rdataset_isassociated(sigrdataset)) {
			dns_rdataset_disassociate(sigrdataset);
		}

		if (fname == NULL || rdataset == NULL || sigrdataset == NULL) {
			goto cleanup;
		}
		/*
		 * Add the no wildcard proof.
		 */
		result = dns_name_concatenate(dns_wildcardname, cname, wname,
					      NULL);
		if (result != ISC_R_SUCCESS) {
			goto cleanup;
		}

		query_findclosestnsec3(wname, qctx->db, qctx->version, client,
				       rdataset, sigrdataset, fname, nodata,
				       NULL);
		if (!dns_rdataset_isassociated(rdataset)) {
			goto cleanup;
		}
		query_addrrset(qctx, &fname, &rdataset, &sigrdataset, dbuf,
			       DNS_SECTION_AUTHORITY);

		goto cleanup;
	} else if (result == DNS_R_NXDOMAIN) {
		if (!ispositive) {
			result = dns_rdataset_first(rdataset);
		}
		if (result == ISC_R_SUCCESS) {
			dns_rdataset_current(rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &nsec, NULL);
			RUNTIME_CHECK(result == ISC_R_SUCCESS);
			(void)dns_name_fullcompare(name, fname, &order,
						   &olabels);
			(void)dns_name_fullcompare(name, &nsec.next, &order,
						   &nlabels);
			/*
			 * Check for a pathological condition created when
			 * serving some malformed signed zones and bail out.
			 */
			if (dns_name_countlabels(name) == nlabels) {
				goto cleanup;
			}

			if (olabels > nlabels) {
				dns_name_split(name, olabels, NULL, wname);
			} else {
				dns_name_split(name, nlabels, NULL, wname);
			}
			result = dns_name_concatenate(dns_wildcardname, wname,
						      wname, NULL);
			if (result == ISC_R_SUCCESS) {
				have_wname = true;
			}
			dns_rdata_freestruct(&nsec);
		}
		query_addrrset(qctx, &fname, &rdataset, &sigrdataset, dbuf,
			       DNS_SECTION_AUTHORITY);
	}
	if (rdataset != NULL) {
		ns_client_putrdataset(client, &rdataset);
	}
	if (sigrdataset != NULL) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (fname != NULL) {
		ns_client_releasename(client, &fname);
	}
	if (have_wname) {
		ispositive = true; /* prevent loop */
		if (!dns_name_equal(name, wname)) {
			name = wname;
			goto again;
		}
	}
cleanup:
	if (rdataset != NULL) {
		ns_client_putrdataset(client, &rdataset);
	}
	if (sigrdataset != NULL) {
		ns_client_putrdataset(client, &sigrdataset);
	}
	if (fname != NULL) {
		ns_client_releasename(client, &fname);
	}
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
				ns_client_releasename(qctx->client,
						      &qctx->fname);
			}
			query_addbestns(qctx);
		}
	}

	/*
	 * Add NSEC records to the authority section if they're needed for
	 * DNSSEC wildcard proofs.
	 */
	if (qctx->need_wildcardproof && dns_db_issecure(qctx->db)) {
		query_addwildcardproof(qctx, true, false);
	}
}

/*
 * Find the sort order of 'rdata' in the topology-like
 * ACL forming the second element in a 2-element top-level
 * sortlist statement.
 */
static int
query_sortlist_order_2element(const dns_rdata_t *rdata, const void *arg) {
	isc_netaddr_t netaddr;

	if (rdata_tonetaddr(rdata, &netaddr) != ISC_R_SUCCESS) {
		return (INT_MAX);
	}
	return (ns_sortlist_addrorder2(&netaddr, arg));
}

/*
 * Find the sort order of 'rdata' in the matching element
 * of a 1-element top-level sortlist statement.
 */
static int
query_sortlist_order_1element(const dns_rdata_t *rdata, const void *arg) {
	isc_netaddr_t netaddr;

	if (rdata_tonetaddr(rdata, &netaddr) != ISC_R_SUCCESS) {
		return (INT_MAX);
	}
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
	dns_aclenv_t *env =
		ns_interfacemgr_getaclenv(client->manager->interface->mgr);
	const void *order_arg = NULL;

	isc_netaddr_fromsockaddr(&netaddr, &client->peeraddr);
	switch (ns_sortlist_setup(client->view->sortlist, env, &netaddr,
				  &order_arg))
	{
	case NS_SORTLISTTYPE_1ELEMENT:
		dns_message_setsortorder(client->message,
					 query_sortlist_order_1element, env,
					 NULL, order_arg);
		break;
	case NS_SORTLISTTYPE_2ELEMENT:
		dns_message_setsortorder(client->message,
					 query_sortlist_order_2element, env,
					 order_arg, NULL);
		break;
	case NS_SORTLISTTYPE_NONE:
		break;
	default:
		UNREACHABLE();
	}
}

/*
 * When sending a referral, if the answer to the question is
 * in the glue, sort it to the start of the additional section.
 */
static void
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
	for (name = ISC_LIST_HEAD(msg->sections[section]); name != NULL;
	     name = ISC_LIST_NEXT(name, link))
	{
		if (dns_name_equal(name, qctx->client->query.qname)) {
			for (rdataset = ISC_LIST_HEAD(name->list);
			     rdataset != NULL;
			     rdataset = ISC_LIST_NEXT(rdataset, link))
			{
				if (rdataset->type == qctx->qtype) {
					break;
				}
			}
			break;
		}
	}
	if (rdataset != NULL) {
		ISC_LIST_UNLINK(msg->sections[section], name, link);
		ISC_LIST_PREPEND(msg->sections[section], name, link);
		ISC_LIST_UNLINK(name->list, rdataset, link);
		ISC_LIST_PREPEND(name->list, rdataset, link);
		rdataset->attributes |= DNS_RDATASETATTR_REQUIRED;
	}
}

isc_result_t
ns_query_done(query_ctx_t *qctx) {
	isc_result_t result;
	const dns_namelist_t *secs = qctx->client->message->sections;
	bool nodetach;

	CCTRACE(ISC_LOG_DEBUG(3), "ns_query_done");

	CALL_HOOK(NS_QUERY_DONE_BEGIN, qctx);

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

	if (qctx->client->query.gluedb != NULL) {
		dns_db_detach(&qctx->client->query.gluedb);
	}

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

		qctx->detach_client = true;
		return (qctx->result);
	}

	/*
	 * If we're recursing then just return; the query will
	 * resume when recursion ends.
	 */
	if (RECURSING(qctx->client) &&
	    (!QUERY_STALETIMEOUT(&qctx->client->query) ||
	     ((qctx->options & DNS_GETDB_STALEFIRST) != 0)))
	{
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
	    qctx->view->auth_nxdomain)
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

	CALL_HOOK(NS_QUERY_DONE_SEND, qctx);

	/*
	 * Client may have been detached after query_send(), so
	 * we test and store the flag state here, for safety.
	 * If we are refreshing the RRSet, we must not detach from the client
	 * in the query_send(), so we need to override the flag.
	 */
	if (qctx->refresh_rrset) {
		qctx->client->nodetach = true;
	}
	nodetach = qctx->client->nodetach;
	query_send(qctx->client);

	if (qctx->refresh_rrset) {
		/*
		 * If we reached this point then it means that we have found a
		 * stale RRset entry in cache and BIND is configured to allow
		 * queries to be answered with stale data if no active RRset
		 * is available, i.e. "stale-anwer-client-timeout 0". But, we
		 * still need to refresh the RRset. To prevent adding duplicate
		 * RRsets, clear the RRsets from the message before doing the
		 * refresh.
		 */
		message_clearrdataset(qctx->client->message, 0);
		query_refresh_rrset(qctx);
	}

	if (!nodetach) {
		qctx->detach_client = true;
	}
	return (qctx->result);

cleanup:
	return (result);
}

static void
log_tat(ns_client_t *client) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char clientbuf[ISC_NETADDR_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];
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
	isc_netaddr_format(&netaddr, clientbuf, sizeof(clientbuf));
	dns_rdataclass_format(client->view->rdclass, classbuf,
			      sizeof(classbuf));

	if (client->query.qtype == dns_rdatatype_dnskey) {
		uint16_t keytags = client->keytag_len / 2;
		size_t len = taglen = sizeof("65000") * keytags + 1;
		char *cp = tags = isc_mem_get(client->mctx, taglen);
		int i = 0;

		INSIST(client->keytag != NULL);
		if (tags != NULL) {
			while (keytags-- > 0U) {
				int n;
				uint16_t keytag;
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
		      namebuf, classbuf, clientbuf, tags != NULL ? tags : "");
	if (tags != NULL) {
		isc_mem_put(client->mctx, tags, taglen);
	}
}

static void
log_query(ns_client_t *client, unsigned int flags, unsigned int extflags) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];
	char onbuf[ISC_NETADDR_FORMATSIZE];
	char ecsbuf[DNS_ECS_FORMATSIZE + sizeof(" [ECS ]") - 1] = { 0 };
	char ednsbuf[sizeof("E(65535)")] = { 0 };
	dns_rdataset_t *rdataset;
	int level = ISC_LOG_INFO;

	if (!isc_log_wouldlog(ns_lctx, level)) {
		return;
	}

	rdataset = ISC_LIST_HEAD(client->query.qname->list);
	INSIST(rdataset != NULL);
	dns_name_format(client->query.qname, namebuf, sizeof(namebuf));
	dns_rdataclass_format(rdataset->rdclass, classbuf, sizeof(classbuf));
	dns_rdatatype_format(rdataset->type, typebuf, sizeof(typebuf));
	isc_netaddr_format(&client->destaddr, onbuf, sizeof(onbuf));

	if (client->ednsversion >= 0) {
		snprintf(ednsbuf, sizeof(ednsbuf), "E(%hd)",
			 client->ednsversion);
	}

	if (HAVEECS(client)) {
		strlcpy(ecsbuf, " [ECS ", sizeof(ecsbuf));
		dns_ecs_format(&client->ecs, ecsbuf + 6, sizeof(ecsbuf) - 6);
		strlcat(ecsbuf, "]", sizeof(ecsbuf));
	}

	ns_client_log(client, NS_LOGCATEGORY_QUERIES, NS_LOGMODULE_QUERY, level,
		      "query: %s %s %s %s%s%s%s%s%s%s (%s)%s", namebuf,
		      classbuf, typebuf, WANTRECURSION(client) ? "+" : "-",
		      (client->signer != NULL) ? "S" : "", ednsbuf,
		      TCP(client) ? "T" : "",
		      ((extflags & DNS_MESSAGEEXTFLAG_DO) != 0) ? "D" : "",
		      ((flags & DNS_MESSAGEFLAG_CD) != 0) ? "C" : "",
		      HAVECOOKIE(client)   ? "V"
		      : WANTCOOKIE(client) ? "K"
					   : "",
		      onbuf, ecsbuf);
}

static void
log_queryerror(ns_client_t *client, isc_result_t result, int line, int level) {
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];
	const char *namep, *typep, *classp, *sep1, *sep2;
	dns_rdataset_t *rdataset;

	if (!isc_log_wouldlog(ns_lctx, level)) {
		return;
	}

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
			dns_rdataclass_format(rdataset->rdclass, classbuf,
					      sizeof(classbuf));
			classp = classbuf;
			dns_rdatatype_format(rdataset->type, typebuf,
					     sizeof(typebuf));
			typep = typebuf;
			sep2 = "/";
		}
	}

	ns_client_log(client, NS_LOGCATEGORY_QUERY_ERRORS, NS_LOGMODULE_QUERY,
		      level, "query failed (%s)%s%s%s%s%s%s at %s:%d",
		      isc_result_totext(result), sep1, namep, sep2, classp,
		      sep2, typep, __FILE__, line);
}

void
ns_query_start(ns_client_t *client, isc_nmhandle_t *handle) {
	isc_result_t result;
	dns_message_t *message;
	dns_rdataset_t *rdataset;
	dns_rdatatype_t qtype;
	unsigned int saved_extflags;
	unsigned int saved_flags;

	REQUIRE(NS_CLIENT_VALID(client));

	/*
	 * Attach to the request handle
	 */
	isc_nmhandle_attach(handle, &client->reqhandle);

	message = client->message;
	saved_extflags = client->extflags;
	saved_flags = client->message->flags;

	CTRACE(ISC_LOG_DEBUG(3), "ns_query_start");

	/*
	 * Ensure that appropriate cleanups occur.
	 */
	client->cleanup = query_cleanup;

	if ((message->flags & DNS_MESSAGEFLAG_RD) != 0) {
		client->query.attributes |= NS_QUERYATTR_WANTRECURSION;
	}

	if ((client->extflags & DNS_MESSAGEEXTFLAG_DO) != 0) {
		client->attributes |= NS_CLIENTATTR_WANTDNSSEC;
	}

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
		if ((message->flags & DNS_MESSAGEFLAG_RD) != 0) {
			client->query.attributes |= NS_QUERYATTR_NOAUTHORITY;
		}
		break;
	}

	if (client->view->cachedb == NULL || !client->view->recursion) {
		/*
		 * We don't have a cache.  Turn off cache support and
		 * recursion.
		 */
		client->query.attributes &= ~(NS_QUERYATTR_RECURSIONOK |
					      NS_QUERYATTR_CACHEOK);
		client->attributes |= NS_CLIENTATTR_NOSETFC;
	} else if ((client->attributes & NS_CLIENTATTR_RA) == 0 ||
		   (message->flags & DNS_MESSAGEFLAG_RD) == 0)
	{
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
		} else {
			query_error(client, result, __LINE__);
		}
		return;
	}

	if ((client->sctx->options & NS_SERVER_LOGQUERIES) != 0) {
		log_query(client, saved_flags, saved_extflags);
	}

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
			result = dns_tkey_processquery(
				client->message, client->sctx->tkeyctx,
				client->view->dynamickeys);
			if (result == ISC_R_SUCCESS) {
				query_send(client);
			} else {
				query_error(client, result, __LINE__);
			}
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
	} else if (qtype == dns_rdatatype_ns) {
		/*
		 * Always turn on additional records for NS queries.
		 */
		client->query.attributes &= ~(NS_QUERYATTR_NOAUTHORITY |
					      NS_QUERYATTR_NOADDITIONAL);
	}

	/*
	 * Maybe turn on minimal responses for ANY queries.
	 */
	if (qtype == dns_rdatatype_any && client->view->minimal_any &&
	    !TCP(client))
	{
		client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
					     NS_QUERYATTR_NOADDITIONAL);
	}

	/*
	 * Turn on minimal responses for EDNS/UDP bufsize 512 queries.
	 */
	if (client->ednsversion >= 0 && client->udpsize <= 512U && !TCP(client))
	{
		client->query.attributes |= (NS_QUERYATTR_NOAUTHORITY |
					     NS_QUERYATTR_NOADDITIONAL);
	}

	/*
	 * If the client has requested that DNSSEC checking be disabled,
	 * allow lookups to return pending data and instruct the resolver
	 * to return data before validation has completed.
	 *
	 * We don't need to set DNS_DBFIND_PENDINGOK when validation is
	 * disabled as there will be no pending data.
	 */
	if ((message->flags & DNS_MESSAGEFLAG_CD) != 0 ||
	    qtype == dns_rdatatype_rrsig)
	{
		client->query.dboptions |= DNS_DBFIND_PENDINGOK;
		client->query.fetchoptions |= DNS_FETCHOPT_NOVALIDATE;
	} else if (!client->view->enablevalidation) {
		client->query.fetchoptions |= DNS_FETCHOPT_NOVALIDATE;
	}

	if (client->view->qminimization) {
		client->query.fetchoptions |= DNS_FETCHOPT_QMINIMIZE |
					      DNS_FETCHOPT_QMIN_SKIP_IP6A;
		if (client->view->qmin_strict) {
			client->query.fetchoptions |= DNS_FETCHOPT_QMIN_STRICT;
		} else {
			client->query.fetchoptions |= DNS_FETCHOPT_QMIN_USE_A;
		}
	}

	/*
	 * Allow glue NS records to be added to the authority section
	 * if the answer is secure.
	 */
	if ((message->flags & DNS_MESSAGEFLAG_CD) != 0) {
		client->query.attributes &= ~NS_QUERYATTR_SECURE;
	}

	/*
	 * Set NS_CLIENTATTR_WANTAD if the client has set AD in the query.
	 * This allows AD to be returned on queries without DO set.
	 */
	if ((message->flags & DNS_MESSAGEFLAG_AD) != 0) {
		client->attributes |= NS_CLIENTATTR_WANTAD;
	}

	/*
	 * This is an ordinary query.
	 */
	result = dns_message_reply(message, true);
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
	if ((client->sctx->options & NS_SERVER_NOAA) == 0) {
		message->flags |= DNS_MESSAGEFLAG_AA;
	}

	/*
	 * Set AD.  We must clear it if we add non-validated data to a
	 * response.
	 */
	if (WANTDNSSEC(client) || WANTAD(client)) {
		message->flags |= DNS_MESSAGEFLAG_AD;
	}

	(void)query_setup(client, qtype);
}
