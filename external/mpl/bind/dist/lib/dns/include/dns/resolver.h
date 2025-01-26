/*	$NetBSD: resolver.h,v 1.10 2025/01/26 16:25:28 christos Exp $	*/

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

/*****
***** Module Info
*****/

/*! \file dns/resolver.h
 *
 * \brief
 * This is the BIND 9 resolver, the module responsible for resolving DNS
 * requests by iteratively querying authoritative servers and following
 * referrals.  This is a "full resolver", not to be confused with
 * the stub resolvers most people associate with the word "resolver".
 * The full resolver is part of the caching name server or resolver
 * daemon the stub resolver talks to.
 *
 * MP:
 *\li	The module ensures appropriate synchronization of data structures it
 *	creates and manipulates.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	RFCs:	1034, 1035, 2181, TBS
 *\li	Drafts:	TBS
 */

#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/loop.h>
#include <isc/refcount.h>
#include <isc/stats.h>
#include <isc/tls.h>
#include <isc/types.h>

#include <dns/fixedname.h>
#include <dns/types.h>

/* Add -DDNS_RESOLVER_TRACE=1 to CFLAGS for detailed reference tracing */

ISC_LANG_BEGINDECLS

/*%
 * A dns_fetchresponse_t is sent to the caller when a fetch completes.
 * Any of 'db', 'node', 'rdataset', and 'sigrdataset' may be bound; it
 * is the receiver's responsibility to detach them, and also free the
 * structure.
 *
 * \brief
 * 'rdataset', 'sigrdataset', 'client' and 'id' are the values that were
 * supplied when dns_resolver_createfetch() was called.  They are returned
 *  to the caller so that they may be freed.
 */
typedef struct dns_fetchresponse dns_fetchresponse_t;

struct dns_fetchresponse {
	dns_fetch_t	     *fetch;
	isc_mem_t	     *mctx;
	isc_result_t	      result;
	isc_result_t	      vresult;
	dns_rdatatype_t	      qtype;
	dns_db_t	     *db;
	dns_dbnode_t	     *node;
	dns_rdataset_t	     *rdataset;
	dns_rdataset_t	     *sigrdataset;
	dns_fixedname_t	      fname;
	dns_name_t	     *foundname;
	const isc_sockaddr_t *client;
	dns_messageid_t	      id;
	isc_loop_t	     *loop;
	isc_job_cb	      cb;
	void		     *arg;
	ISC_LINK(dns_fetchresponse_t) link;
};

/*%
 * The two quota types (fetches-per-zone and fetches-per-server)
 */
typedef enum { dns_quotatype_zone = 0, dns_quotatype_server } dns_quotatype_t;

/*
 * Options that modify how a 'fetch' is done.
 */
enum {
	DNS_FETCHOPT_TCP = 1 << 0,	       /*%< Use TCP. */
	DNS_FETCHOPT_UNSHARED = 1 << 1,	       /*%< See below. */
	DNS_FETCHOPT_RECURSIVE = 1 << 2,       /*%< Set RD? */
	DNS_FETCHOPT_NOEDNS0 = 1 << 3,	       /*%< Do not use EDNS. */
	DNS_FETCHOPT_FORWARDONLY = 1 << 4,     /*%< Only use forwarders. */
	DNS_FETCHOPT_NOVALIDATE = 1 << 5,      /*%< Disable validation. */
	DNS_FETCHOPT_WANTNSID = 1 << 6,	       /*%< Request NSID */
	DNS_FETCHOPT_PREFETCH = 1 << 7,	       /*%< Do prefetch */
	DNS_FETCHOPT_NOCDFLAG = 1 << 8,	       /*%< Don't set CD flag. */
	DNS_FETCHOPT_NONTA = 1 << 9,	       /*%< Ignore NTA table. */
	DNS_FETCHOPT_NOCACHED = 1 << 10,       /*%< Force cache update. */
	DNS_FETCHOPT_QMINIMIZE = 1 << 11,      /*%< Use qname minimization. */
	DNS_FETCHOPT_NOFOLLOW = 1 << 12,       /*%< Don't retrieve the NS RRset
						* from the child zone when a
						* delegation is returned in
						* response to a NS query. */
	DNS_FETCHOPT_QMIN_STRICT = 1 << 13,    /*%< Do not work around servers
						* that return errors on
						* non-empty terminals. */
	DNS_FETCHOPT_QMIN_SKIP_IP6A = 1 << 14, /*%< Skip some labels when
						* doing qname minimization
						* on ip6.arpa. */
	DNS_FETCHOPT_NOFORWARD = 1 << 15,      /*%< Do not use forwarders if
						* possible. */

	/*% EDNS version bits: */
	DNS_FETCHOPT_EDNSVERSIONSET = 1 << 23,
	DNS_FETCHOPT_EDNSVERSIONSHIFT = 24,
	DNS_FETCHOPT_EDNSVERSIONMASK = 0xff000000,
};

/*
 * Upper bounds of class of query RTT (ms).  Corresponds to
 * dns_resstatscounter_queryrttX statistics counters.
 */
#define DNS_RESOLVER_QRYRTTCLASS0    10
#define DNS_RESOLVER_QRYRTTCLASS0STR "10"
#define DNS_RESOLVER_QRYRTTCLASS1    100
#define DNS_RESOLVER_QRYRTTCLASS1STR "100"
#define DNS_RESOLVER_QRYRTTCLASS2    500
#define DNS_RESOLVER_QRYRTTCLASS2STR "500"
#define DNS_RESOLVER_QRYRTTCLASS3    800
#define DNS_RESOLVER_QRYRTTCLASS3STR "800"
#define DNS_RESOLVER_QRYRTTCLASS4    1600
#define DNS_RESOLVER_QRYRTTCLASS4STR "1600"

/*
 * XXXRTH  Should this API be made semi-private?  (I.e.
 * _dns_resolver_create()).
 */

#define DNS_RESOLVER_CHECKNAMES	    0x01
#define DNS_RESOLVER_CHECKNAMESFAIL 0x02

#define DNS_QMIN_MAXLABELS	   7
#define DNS_QMIN_MAX_NO_DELEGATION 3

isc_result_t
dns_resolver_create(dns_view_t *view, isc_loopmgr_t *loopmgr, isc_nm_t *nm,
		    unsigned int options, isc_tlsctx_cache_t *tlsctx_cache,
		    dns_dispatch_t *dispatchv4, dns_dispatch_t *dispatchv6,
		    dns_resolver_t **resp);

/*%<
 * Create a resolver.
 *
 * Notes:
 *
 *\li	Generally, applications should not create a resolver directly, but
 *	should instead call dns_view_createresolver().
 *
 * Requires:
 *
 *\li	'view' is a valid view.
 *
 *\li	'nm' is a valid network manager.
 *
 *\li	'tlsctx_cache' != NULL.
 *
 *\li	'dispatchv4' is a dispatch with an IPv4 UDP socket, or is NULL.
 *	If not NULL, clones per loop of it will be created by the resolver.
 *
 *\li	'dispatchv6' is a dispatch with an IPv6 UDP socket, or is NULL.
 *	If not NULL, clones per loop of it will be created by the resolver.
 *
 *\li	resp != NULL && *resp == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS				On success.
 *
 *\li	Anything else				Failure.
 */

void
dns_resolver_freeze(dns_resolver_t *res);
/*%<
 * Freeze resolver.
 *
 * Notes:
 *
 *\li	Certain configuration changes cannot be made after the resolver
 *	is frozen.  Fetches cannot be created until the resolver is frozen.
 *
 * Requires:
 *
 *\li	'res' is a valid resolver.
 *
 * Ensures:
 *
 *\li	'res' is frozen.
 */

void
dns_resolver_prime(dns_resolver_t *res);
/*%<
 * Prime resolver.
 *
 * Notes:
 *
 *\li	Resolvers which have a forwarding policy other than dns_fwdpolicy_only
 *	need to be primed with the root nameservers, otherwise the root
 *	nameserver hints data may be used indefinitely.  This function requests
 *	that the resolver start a priming fetch, if it isn't already priming.
 *
 * Requires:
 *
 *\li	'res' is a valid, frozen resolver.
 */

void
dns_resolver_shutdown(dns_resolver_t *res);
/*%<
 * Start the shutdown process for 'res'.
 *
 * Notes:
 *
 *\li	This call has no effect if the resolver is already shutting down.
 *
 * Requires:
 *
 *\li	'res' is a valid resolver.
 */

#if DNS_RESOLVER_TRACE
#define dns_resolver_ref(ptr) \
	dns_resolver__ref(ptr, __func__, __FILE__, __LINE__)
#define dns_resolver_unref(ptr) \
	dns_resolver__unref(ptr, __func__, __FILE__, __LINE__)
#define dns_resolver_attach(ptr, ptrp) \
	dns_resolver__attach(ptr, ptrp, __func__, __FILE__, __LINE__)
#define dns_resolver_detach(ptrp) \
	dns_resolver__detach(ptrp, __func__, __FILE__, __LINE__)
ISC_REFCOUNT_TRACE_DECL(dns_resolver);
#else
ISC_REFCOUNT_DECL(dns_resolver);
#endif

isc_result_t
dns_resolver_createfetch(dns_resolver_t *res, const dns_name_t *name,
			 dns_rdatatype_t type, const dns_name_t *domain,
			 dns_rdataset_t	      *nameservers,
			 dns_forwarders_t     *forwarders,
			 const isc_sockaddr_t *client, dns_messageid_t id,
			 unsigned int options, unsigned int depth,
			 isc_counter_t *qc, isc_loop_t *loop, isc_job_cb cb,
			 void *arg, dns_rdataset_t *rdataset,
			 dns_rdataset_t *sigrdataset, dns_fetch_t **fetchp);
/*%<
 * Recurse to answer a question.
 *
 * Notes:
 *
 *\li	This call starts a query for 'name', type 'type'.
 *
 *\li	The 'domain' is a parent domain of 'name' for which
 *	a set of name servers 'nameservers' is known.  If no
 *	such name server information is available, set
 * 	'domain' and 'nameservers' to NULL.
 *
 *\li	'forwarders' is unimplemented, and subject to change when
 *	we figure out how selective forwarding will work.
 *
 *\li	When the fetch completes (successfully or otherwise), a
 *	dns_fetchresponse_t option is sent to callback 'cb'.
 *
 *\li	The values of 'rdataset' and 'sigrdataset' will be returned in
 *	the fetch completion event.
 *
 *\li	'client' and 'id' are used for duplicate query detection.  '*client'
 *	must remain stable until after 'action' has been called or
 *	dns_resolver_cancelfetch() is called.
 *
 * Requires:
 *
 *\li	'res' is a valid resolver that has been frozen.
 *
 *\li	'name' is a valid name.
 *
 *\li	'type' is not a meta type other than ANY.
 *
 *\li	'domain' is a valid name or NULL.
 *
 *\li	'nameservers' is a valid NS rdataset (whose owner name is 'domain')
 *	iff. 'domain' is not NULL.
 *
 *\li	'forwarders' is NULL.
 *
 *\li	'client' is a valid sockaddr or NULL.
 *
 *\li	'options' contains valid options.
 *
 *\li	'rdataset' is a valid, disassociated rdataset.
 *
 *\li	'sigrdataset' is NULL, or is a valid, disassociated rdataset.
 *
 *\li	fetchp != NULL && *fetchp == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS					Success
 *\li	#DNS_R_DUPLICATE
 *\li	#DNS_R_DROP
 *
 *\li	Many other values are possible, all of which indicate failure.
 */

void
dns_resolver_cancelfetch(dns_fetch_t *fetch);
/*%<
 * Cancel 'fetch'.
 *
 * Notes:
 *
 *\li	If 'fetch' has not completed, post its completion event with a
 *	result code of #ISC_R_CANCELED.
 *
 * Requires:
 *
 *\li	'fetch' is a valid fetch.
 */

void
dns_resolver_destroyfetch(dns_fetch_t **fetchp);
/*%<
 * Destroy 'fetch'.
 *
 * Requires:
 *
 *\li	'*fetchp' is a valid fetch.
 *
 *\li	The caller has received the fetch completion event (either because the
 *	fetch completed or because dns_resolver_cancelfetch() was called).
 *
 * Ensures:
 *
 *\li	*fetchp == NULL.
 */

void
dns_resolver_logfetch(dns_fetch_t *fetch, isc_log_t *lctx,
		      isc_logcategory_t *category, isc_logmodule_t *module,
		      int level, bool duplicateok);
/*%<
 * Dump a log message on internal state at the completion of given 'fetch'.
 * 'lctx', 'category', 'module', and 'level' are used to write the log message.
 * By default, only one log message is written even if the corresponding fetch
 * context serves multiple clients; if 'duplicateok' is true the suppression
 * is disabled and the message can be written every time this function is
 * called.
 *
 * Requires:
 *
 *\li	'fetch' is a valid fetch, and has completed.
 */

dns_dispatch_t *
dns_resolver_dispatchv4(dns_resolver_t *resolver);

dns_dispatch_t *
dns_resolver_dispatchv6(dns_resolver_t *resolver);

void
dns_resolver_addalternate(dns_resolver_t *resolver, const isc_sockaddr_t *alt,
			  const dns_name_t *name, in_port_t port);
/*%<
 * Add alternate addresses to be tried in the event that the nameservers
 * for a zone are not available in the address families supported by the
 * operating system.
 *
 * Require:
 * \li	only one of 'name' or 'alt' to be valid.
 */

isc_result_t
dns_resolver_disable_algorithm(dns_resolver_t *resolver, const dns_name_t *name,
			       unsigned int alg);
/*%<
 * Mark the given DNSSEC algorithm as disabled and below 'name'.
 * Valid algorithms are less than 256.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_RANGE
 *\li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_resolver_disable_ds_digest(dns_resolver_t *resolver, const dns_name_t *name,
			       unsigned int digest_type);
/*%<
 * Mark the given DS digest type as disabled and below 'name'.
 * Valid types are less than 256.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_RANGE
 *\li	#ISC_R_NOMEMORY
 */

bool
dns_resolver_algorithm_supported(dns_resolver_t	  *resolver,
				 const dns_name_t *name, unsigned int alg);
/*%<
 * Check if the given algorithm is supported by this resolver.
 * This checks whether the algorithm has been disabled via
 * dns_resolver_disable_algorithm(), then checks the underlying
 * crypto libraries if it was not specifically disabled.
 */

bool
dns_resolver_ds_digest_supported(dns_resolver_t	  *resolver,
				 const dns_name_t *name,
				 unsigned int	   digest_type);
/*%<
 * Check if the given digest type is supported by this resolver.
 * This checks whether the digest type has been disabled via
 * dns_resolver_disable_ds_digest(), then checks the underlying
 * crypto libraries if it was not specifically disabled.
 */

isc_result_t
dns_resolver_setmustbesecure(dns_resolver_t *resolver, const dns_name_t *name,
			     bool value);

bool
dns_resolver_getmustbesecure(dns_resolver_t *resolver, const dns_name_t *name);

void
dns_resolver_settimeout(dns_resolver_t *resolver, unsigned int timeout);
/*%<
 * Set the length of time the resolver will work on a query, in milliseconds.
 *
 * 'timeout' was originally defined in seconds, and later redefined to be in
 * milliseconds.  Values less than or equal to 300 are treated as seconds.
 *
 * If timeout is 0, the default timeout will be applied.
 *
 * Requires:
 * \li  resolver to be valid.
 */

unsigned int
dns_resolver_gettimeout(dns_resolver_t *resolver);
/*%<
 * Get the current length of time the resolver will work on a query,
 * in milliseconds.
 *
 * Requires:
 * \li  resolver to be valid.
 */

void
dns_resolver_setclientsperquery(dns_resolver_t *resolver, uint32_t min,
				uint32_t max);
void
dns_resolver_setfetchesperzone(dns_resolver_t *resolver, uint32_t clients);

uint32_t
dns_resolver_getfetchesperzone(dns_resolver_t *resolver);

void
dns_resolver_getclientsperquery(dns_resolver_t *resolver, uint32_t *cur,
				uint32_t *min, uint32_t *max);

bool
dns_resolver_getzeronosoattl(dns_resolver_t *resolver);

void
dns_resolver_setzeronosoattl(dns_resolver_t *resolver, bool state);

unsigned int
dns_resolver_getoptions(dns_resolver_t *resolver);
/*%<
 * Get the resolver options.
 *
 * Requires:
 * \li	resolver to be valid.
 */

void
dns_resolver_setmaxvalidations(dns_resolver_t *resolver, uint32_t max);
void
dns_resolver_setmaxvalidationfails(dns_resolver_t *resolver, uint32_t max);
/*%
 * Set maximum numbers of validations and maximum validation failures per fetch.
 */

void
dns_resolver_setmaxdepth(dns_resolver_t *resolver, unsigned int maxdepth);
unsigned int
dns_resolver_getmaxdepth(dns_resolver_t *resolver);
/*%
 * Get and set how many NS indirections will be followed when looking for
 * nameserver addresses.
 *
 * Requires:
 * \li	resolver to be valid.
 */

void
dns_resolver_setmaxqueries(dns_resolver_t *resolver, unsigned int queries);
unsigned int
dns_resolver_getmaxqueries(dns_resolver_t *resolver);
/*%
 * Get and set how many iterative queries will be allowed before
 * terminating a recursive query.
 *
 * Requires:
 * \li	resolver to be valid.
 */

void
dns_resolver_setquotaresponse(dns_resolver_t *resolver, dns_quotatype_t which,
			      isc_result_t resp);
isc_result_t
dns_resolver_getquotaresponse(dns_resolver_t *resolver, dns_quotatype_t which);
/*%
 * Get and set the result code that will be used when quotas
 * are exceeded. If 'which' is set to quotatype "zone", then the
 * result specified in 'resp' will be used when the fetches-per-zone
 * quota is exceeded by a fetch.  If 'which' is set to quotatype "server",
 * then the result specified in 'resp' will be used when the
 * fetches-per-server quota has been exceeded for all the
 * authoritative servers for a zone.  Valid choices are
 * DNS_R_DROP or DNS_R_SERVFAIL.
 *
 * Requires:
 * \li	'resolver' to be valid.
 * \li	'which' to be dns_quotatype_zone or dns_quotatype_server
 * \li	'resp' to be DNS_R_DROP or DNS_R_SERVFAIL.
 */

void
dns_resolver_dumpfetches(dns_resolver_t *resolver, isc_statsformat_t format,
			 FILE *fp);
isc_result_t
dns_resolver_dumpquota(dns_resolver_t *res, isc_buffer_t **buf);

#ifdef ENABLE_AFL
/*%
 * Enable fuzzing of resolver, changes behaviour and eliminates retries
 */
void
dns_resolver_setfuzzing(void);
#endif /* ifdef ENABLE_AFL */

void
dns_resolver_setstats(dns_resolver_t *res, isc_stats_t *stats);
/*%<
 * Set a general resolver statistics counter set 'stats' for 'res'.
 *
 * Requires:
 * \li	'res' is valid.
 *
 *\li	stats is a valid statistics supporting resolver statistics counters
 *	(see dns/stats.h).
 */

void
dns_resolver_getstats(dns_resolver_t *res, isc_stats_t **statsp);
/*%<
 * Get the general statistics counter set for 'res'.  If a statistics set is
 * set '*statsp' will be attached to the set; otherwise, '*statsp' will be
 * untouched.
 *
 * Requires:
 * \li	'res' is valid.
 *
 *\li	'statsp' != NULL && '*statsp' != NULL
 */

void
dns_resolver_incstats(dns_resolver_t *res, isc_statscounter_t counter);
/*%<
 * Increment the specified statistics counter in res->stats, if res->stats
 * is set.
 *
 * Requires:
 * \li	'res' is valid.
 */

void
dns_resolver_setquerystats(dns_resolver_t *res, dns_stats_t *stats);
/*%<
 * Set a statistics counter set of rdata type, 'stats', for 'res'.  Once the
 * statistic set is installed, the resolver will count outgoing queries
 * per rdata type.
 *
 * Requires:
 * \li	'res' is valid.
 *
 *\li	stats is a valid statistics created by dns_rdatatypestats_create().
 */

void
dns_resolver_getquerystats(dns_resolver_t *res, dns_stats_t **statsp);
/*%<
 * Get the rdatatype statistics counter set for 'res'.  If a statistics set is
 * set '*statsp' will be attached to the set; otherwise, '*statsp' will be
 * untouched.
 *
 * Requires:
 * \li	'res' is valid.
 *
 *\li	'statsp' != NULL && '*statsp' != NULL
 */
ISC_LANG_ENDDECLS
