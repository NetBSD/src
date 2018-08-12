/*	$NetBSD: query.h,v 1.2 2018/08/12 13:02:41 christos Exp $	*/

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
	isc_boolean_t			acl_checked;
	isc_boolean_t			queryok;
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
	isc_boolean_t			timerset;
	dns_name_t *			qname;
	dns_name_t *			origqname;
	dns_rdatatype_t			qtype;
	unsigned int			dboptions;
	unsigned int			fetchoptions;
	dns_db_t *			gluedb;
	dns_db_t *			authdb;
	dns_zone_t *			authzone;
	isc_boolean_t			authdbset;
	isc_boolean_t			isreferral;
	isc_mutex_t			fetchlock;
	dns_fetch_t *			fetch;
	dns_fetch_t *			prefetch;
	dns_rpz_st_t *			rpz_st;
	isc_bufferlist_t		namebufs;
	ISC_LIST(ns_dbversion_t)	activeversions;
	ISC_LIST(ns_dbversion_t)	freeversions;
	dns_rdataset_t *		dns64_aaaa;
	dns_rdataset_t *		dns64_sigaaaa;
	isc_boolean_t *			dns64_aaaaok;
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
		isc_boolean_t		authoritative;
		isc_boolean_t		is_zone;
	} redirect;

	dns_keytag_t			root_key_sentinel_keyid;
	isc_boolean_t			root_key_sentinel_is_ta;
	isc_boolean_t			root_key_sentinel_not_ta;

	ns_query_recparam_t		recparam;
};

#define NS_QUERYATTR_RECURSIONOK	0x0001
#define NS_QUERYATTR_CACHEOK		0x0002
#define NS_QUERYATTR_PARTIALANSWER	0x0004
#define NS_QUERYATTR_NAMEBUFUSED	0x0008
#define NS_QUERYATTR_RECURSING		0x0010
#define NS_QUERYATTR_CACHEGLUEOK	0x0020
#define NS_QUERYATTR_QUERYOKVALID	0x0040
#define NS_QUERYATTR_QUERYOK		0x0080
#define NS_QUERYATTR_WANTRECURSION	0x0100
#define NS_QUERYATTR_SECURE		0x0200
#define NS_QUERYATTR_NOAUTHORITY	0x0400
#define NS_QUERYATTR_NOADDITIONAL	0x0800
#define NS_QUERYATTR_CACHEACLOKVALID	0x1000
#define NS_QUERYATTR_CACHEACLOK		0x2000
#define NS_QUERYATTR_DNS64		0x4000
#define NS_QUERYATTR_DNS64EXCLUDE	0x8000
#define NS_QUERYATTR_RRL_CHECKED	0x10000
#define NS_QUERYATTR_REDIRECT		0x20000

/* query context structure */

typedef struct query_ctx {
	isc_buffer_t *dbuf;			/* name buffer */
	dns_name_t *fname;			/* found name from DB lookup */
	dns_rdataset_t *rdataset;		/* found rdataset */
	dns_rdataset_t *sigrdataset;		/* found sigrdataset */
	dns_rdataset_t *noqname;		/* rdataset needing
						 * NOQNAME proof */
	dns_rdatatype_t qtype;
	dns_rdatatype_t type;

	unsigned int options;			/* DB lookup options */

	isc_boolean_t redirected;		/* nxdomain redirected? */
	isc_boolean_t is_zone;			/* is DB a zone DB? */
	isc_boolean_t is_staticstub_zone;
	isc_boolean_t resuming;			/* resumed from recursion? */
	isc_boolean_t dns64, dns64_exclude, rpz;
	isc_boolean_t authoritative;		/* authoritative query? */
	isc_boolean_t want_restart;		/* CNAME chain or other
						 * restart needed */
	isc_boolean_t need_wildcardproof;	/* wilcard proof needed */
	isc_boolean_t nxrewrite;		/* negative answer from RPZ */
	isc_boolean_t findcoveringnsec;		/* lookup covering NSEC */
	isc_boolean_t want_stale;		/* want stale records? */
	isc_boolean_t answer_has_ns;		/* NS is in answer */
	dns_fixedname_t wildcardname;		/* name needing wcard proof */
	dns_fixedname_t dsname;			/* name needing DS */

	ns_client_t *client;			/* client object */
	dns_fetchevent_t *event;		/* recursion event */

	dns_db_t *db;				/* zone or cache database */
	dns_dbversion_t *version;		/* DB version */
	dns_dbnode_t *node;			/* DB node */

	dns_db_t *zdb;				/* zone DB values, saved */
	dns_name_t *zfname;			/* while searching cache */
	dns_dbversion_t *zversion;		/* for a better answer */
	dns_rdataset_t *zrdataset;
	dns_rdataset_t *zsigrdataset;

	dns_rpz_st_t *rpz_st;			/* RPZ state */
	dns_zone_t *zone;			/* zone to search */

	isc_result_t result;			/* query result */
	int line;				/* line to report error */
} query_ctx_t;

isc_result_t
ns_query_init(ns_client_t *client);

void
ns_query_free(ns_client_t *client);

void
ns_query_start(ns_client_t *client);

void
ns_query_cancel(ns_client_t *client);

/*%
 * (Must not be used outside this module and its associated unit tests.)
 */
isc_result_t
ns__query_sfcache(query_ctx_t *qctx);

/*%
 * (Must not be used outside this module and its associated unit tests.)
 */
isc_result_t
ns__query_start(query_ctx_t *qctx);

#endif /* NS_QUERY_H */
