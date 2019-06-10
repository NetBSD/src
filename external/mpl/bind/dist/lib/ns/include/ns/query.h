/*	$NetBSD: query.h,v 1.3.2.2 2019/06/10 22:04:50 christos Exp $	*/

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

#ifndef NS_QUERY_H
#define NS_QUERY_H 1

/*! \file */

#include <stdbool.h>

#include <isc/types.h>
#include <isc/buffer.h>
#include <isc/netaddr.h>

#include <dns/rdataset.h>
#include <dns/resolver.h>
#include <dns/rpz.h>
#include <dns/types.h>

#include <ns/types.h>

/*% nameserver database version structure */
typedef struct ns_dbversion {
	dns_db_t			*db;
	dns_dbversion_t			*version;
	bool				acl_checked;
	bool				queryok;
	ISC_LINK(struct ns_dbversion)	link;
} ns_dbversion_t;

/*%
 * nameserver recursion parameters, to uniquely identify a recursion
 * query; this is used to detect a recursion loop
 */
typedef struct ns_query_recparam {
	dns_rdatatype_t			qtype;
	dns_name_t *			qname;
	dns_fixedname_t			fqname;
	dns_name_t *			qdomain;
	dns_fixedname_t			fqdomain;
} ns_query_recparam_t;

/*% nameserver query structure */
struct ns_query {
	unsigned int			attributes;
	unsigned int			restarts;
	bool				timerset;
	dns_name_t *			qname;
	dns_name_t *			origqname;
	dns_rdatatype_t			qtype;
	unsigned int			dboptions;
	unsigned int			fetchoptions;
	dns_db_t *			gluedb;
	dns_db_t *			authdb;
	dns_zone_t *			authzone;
	bool				authdbset;
	bool				isreferral;
	isc_mutex_t			fetchlock;
	dns_fetch_t *			fetch;
	dns_fetch_t *			prefetch;
	dns_rpz_st_t *			rpz_st;
	isc_bufferlist_t		namebufs;
	ISC_LIST(ns_dbversion_t)	activeversions;
	ISC_LIST(ns_dbversion_t)	freeversions;
	dns_rdataset_t *		dns64_aaaa;
	dns_rdataset_t *		dns64_sigaaaa;
	bool *				dns64_aaaaok;
	unsigned int			dns64_aaaaoklen;
	unsigned int			dns64_options;
	unsigned int			dns64_ttl;

	struct {
		dns_db_t *      	db;
		dns_zone_t *      	zone;
		dns_dbnode_t *      	node;
		dns_rdatatype_t   	qtype;
		dns_name_t *		fname;
		dns_fixedname_t		fixed;
		isc_result_t		result;
		dns_rdataset_t *	rdataset;
		dns_rdataset_t *	sigrdataset;
		bool			authoritative;
		bool			is_zone;
	} redirect;

	ns_query_recparam_t		recparam;

	dns_keytag_t root_key_sentinel_keyid;
	bool root_key_sentinel_is_ta;
	bool root_key_sentinel_not_ta;
};

#define NS_QUERYATTR_RECURSIONOK	0x00001
#define NS_QUERYATTR_CACHEOK		0x00002
#define NS_QUERYATTR_PARTIALANSWER	0x00004
#define NS_QUERYATTR_NAMEBUFUSED	0x00008
#define NS_QUERYATTR_RECURSING		0x00010
#define NS_QUERYATTR_QUERYOKVALID	0x00040
#define NS_QUERYATTR_QUERYOK		0x00080
#define NS_QUERYATTR_WANTRECURSION	0x00100
#define NS_QUERYATTR_SECURE		0x00200
#define NS_QUERYATTR_NOAUTHORITY	0x00400
#define NS_QUERYATTR_NOADDITIONAL	0x00800
#define NS_QUERYATTR_CACHEACLOKVALID	0x01000
#define NS_QUERYATTR_CACHEACLOK		0x02000
#define NS_QUERYATTR_DNS64		0x04000
#define NS_QUERYATTR_DNS64EXCLUDE	0x08000
#define NS_QUERYATTR_RRL_CHECKED	0x10000
#define NS_QUERYATTR_REDIRECT		0x20000

typedef struct query_ctx query_ctx_t;

/* query context structure */
struct query_ctx {
	isc_buffer_t *dbuf;			/* name buffer */
	dns_name_t *fname;			/* found name from DB lookup */
	dns_name_t *tname;			/* temporary name, used
						 * when processing ANY
						 * queries */
	dns_rdataset_t *rdataset;		/* found rdataset */
	dns_rdataset_t *sigrdataset;		/* found sigrdataset */
	dns_rdataset_t *noqname;		/* rdataset needing
						 * NOQNAME proof */
	dns_rdatatype_t qtype;
	dns_rdatatype_t type;

	unsigned int options;			/* DB lookup options */

	bool redirected;			/* nxdomain redirected? */
	bool is_zone;				/* is DB a zone DB? */
	bool is_staticstub_zone;
	bool resuming;				/* resumed from recursion? */
	bool dns64, dns64_exclude, rpz;
	bool authoritative;			/* authoritative query? */
	bool want_restart;			/* CNAME chain or other
						 * restart needed */
	bool need_wildcardproof;		/* wilcard proof needed */
	bool nxrewrite;				/* negative answer from RPZ */
	bool findcoveringnsec;			/* lookup covering NSEC */
	bool answer_has_ns;			/* NS is in answer */
	dns_fixedname_t wildcardname;		/* name needing wcard proof */
	dns_fixedname_t dsname;			/* name needing DS */

	ns_client_t *client;			/* client object */
	bool detach_client;			/* client needs detaching */

	dns_fetchevent_t *event;		/* recursion event */

	dns_db_t *db;				/* zone or cache database */
	dns_dbversion_t *version;		/* DB version */
	dns_dbnode_t *node;			/* DB node */

	dns_db_t *zdb;				/* zone DB values, saved */
	dns_dbnode_t *znode;			/* while searching cache */
	dns_name_t *zfname;			/* for a better answer */
	dns_dbversion_t *zversion;
	dns_rdataset_t *zrdataset;
	dns_rdataset_t *zsigrdataset;

	dns_rpz_st_t *rpz_st;			/* RPZ state */
	dns_zone_t *zone;			/* zone to search */

	dns_view_t *view;			/* client view */

	isc_result_t result;			/* query result */
	int line;				/* line to report error */
};

/*
 * The following functions are expected to be used only within query.c
 * and query modules.
 */

isc_result_t
ns_query_done(query_ctx_t *qctx);
/*%<
 * Finalize this phase of the query process:
 *
 * - Clean up.
 * - If we have an answer ready (positive or negative), send it.
 * - If we need to restart for a chaining query, call ns__query_start() again.
 * - If we've started recursion, then just clean up; things will be
 *   restarted via fetch_callback()/query_resume().
 */

isc_result_t
ns_query_recurse(ns_client_t *client, dns_rdatatype_t qtype, dns_name_t *qname,
		 dns_name_t *qdomain, dns_rdataset_t *nameservers,
		 bool resuming);
/*%<
 * Prepare client for recursion, then create a resolver fetch, with
 * the event callback set to fetch_callback(). Afterward we terminate
 * this phase of the query, and resume with a new query context when
 * recursion completes.
 */


isc_result_t
ns_query_init(ns_client_t *client);

void
ns_query_free(ns_client_t *client);

void
ns_query_start(ns_client_t *client);

void
ns_query_cancel(ns_client_t *client);

/*
 * The following functions are expected to be used only within query.c
 * and query modules.
 */

isc_result_t
ns_query_done(query_ctx_t *qctx);
/*%<
 * Finalize this phase of the query process:
 *
 * - Clean up.
 * - If we have an answer ready (positive or negative), send it.
 * - If we need to restart for a chaining query, call ns__query_start() again.
 * - If we've started recursion, then just clean up; things will be
 *   restarted via fetch_callback()/query_resume().
 */

isc_result_t
ns_query_recurse(ns_client_t *client, dns_rdatatype_t qtype, dns_name_t *qname,
		 dns_name_t *qdomain, dns_rdataset_t *nameservers,
		 bool resuming);
/*%<
 * Prepare client for recursion, then create a resolver fetch, with
 * the event callback set to fetch_callback(). Afterward we terminate
 * this phase of the query, and resume with a new query context when
 * recursion completes.
 */


isc_result_t
ns__query_sfcache(query_ctx_t *qctx);
/*%<
 * (Must not be used outside this module and its associated unit tests.)
 */

isc_result_t
ns__query_start(query_ctx_t *qctx);
/*%<
 * (Must not be used outside this module and its associated unit tests.)
 */

#endif /* NS_QUERY_H */
