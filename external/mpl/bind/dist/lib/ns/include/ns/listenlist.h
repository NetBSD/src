/*	$NetBSD: listenlist.h,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#ifndef NS_LISTENLIST_H
#define NS_LISTENLIST_H 1

/*****
 ***** Module Info
 *****/

/*! \file
 * \brief
 * "Listen lists", as in the "listen-on" configuration statement.
 */

/***
 *** Imports
 ***/
#include <isc/net.h>

#include <dns/types.h>

/***
 *** Types
 ***/

typedef struct ns_listenelt ns_listenelt_t;
typedef struct ns_listenlist ns_listenlist_t;

struct ns_listenelt {
	isc_mem_t *	       		mctx;
	in_port_t			port;
	isc_dscp_t			dscp;  /* -1 = not set, 0..63 */
	dns_acl_t *	       		acl;
	ISC_LINK(ns_listenelt_t)	link;
};

struct ns_listenlist {
	isc_mem_t *			mctx;
	int				refcount;
	ISC_LIST(ns_listenelt_t)	elts;
};

/***
 *** Functions
 ***/

isc_result_t
ns_listenelt_create(isc_mem_t *mctx, in_port_t port, isc_dscp_t dscp,
		    dns_acl_t *acl, ns_listenelt_t **target);
/*%<
 * Create a listen-on list element.
 */

void
ns_listenelt_destroy(ns_listenelt_t *elt);
/*%<
 * Destroy a listen-on list element.
 */

isc_result_t
ns_listenlist_create(isc_mem_t *mctx, ns_listenlist_t **target);
/*%<
 * Create a new, empty listen-on list.
 */

void
ns_listenlist_attach(ns_listenlist_t *source, ns_listenlist_t **target);
/*%<
 * Attach '*target' to '*source'.
 */

void
ns_listenlist_detach(ns_listenlist_t **listp);
/*%<
 * Detach 'listp'.
 */

isc_result_t
ns_listenlist_default(isc_mem_t *mctx, in_port_t port, isc_dscp_t dscp,
		      isc_boolean_t enabled, ns_listenlist_t **target);
/*%<
 * Create a listen-on list with default contents, matching
 * all addresses with port 'port' (if 'enabled' is ISC_TRUE),
 * or no addresses (if 'enabled' is ISC_FALSE).
 */

#endif /* NS_LISTENLIST_H */


