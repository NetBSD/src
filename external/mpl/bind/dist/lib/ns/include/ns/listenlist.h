/*	$NetBSD: listenlist.h,v 1.6.2.1 2024/02/25 15:47:35 martin Exp $	*/

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

/*! \file
 * \brief
 * "Listen lists", as in the "listen-on" configuration statement.
 */

/***
 *** Imports
 ***/

#include <stdbool.h>

#include <isc/net.h>
#include <isc/tls.h>

#include <dns/types.h>

/***
 *** Types
 ***/

typedef struct ns_listenelt  ns_listenelt_t;
typedef struct ns_listenlist ns_listenlist_t;

struct ns_listenelt {
	isc_mem_t	   *mctx;
	in_port_t	    port;
	bool		    is_http;
	dns_acl_t	   *acl;
	isc_tlsctx_t	   *sslctx;
	isc_tlsctx_cache_t *sslctx_cache;
	char		  **http_endpoints;
	size_t		    http_endpoints_number;
	uint32_t	    http_max_clients;
	uint32_t	    max_concurrent_streams;
	ISC_LINK(ns_listenelt_t) link;
};

struct ns_listenlist {
	isc_mem_t *mctx;
	int	   refcount;
	ISC_LIST(ns_listenelt_t) elts;
};

typedef struct ns_listen_tls_params {
	const char *name;
	const char *key;
	const char *cert;
	const char *ca_file;
	uint32_t    protocols;
	const char *dhparam_file;
	const char *ciphers;
	bool	    prefer_server_ciphers;
	bool	    prefer_server_ciphers_set;
	bool	    session_tickets;
	bool	    session_tickets_set;
} ns_listen_tls_params_t;

/***
 *** Functions
 ***/

isc_result_t
ns_listenelt_create(isc_mem_t *mctx, in_port_t port, dns_acl_t *acl,
		    const uint16_t family, bool tls,
		    const ns_listen_tls_params_t *tls_params,
		    isc_tlsctx_cache_t *tlsctx_cache, ns_listenelt_t **target);
/*%<
 * Create a listen-on list element.
 *
 * Requires:
 * \li	'targetp' is a valid pointer to a pointer containing 'NULL';
 * \li	'tls_params' is a valid, non-'NULL' pointer if 'tls' equals 'true'.
 * \li	'tlsctx_cache' is a valid, non-'NULL' pointer if 'tls' equals 'true'.
 */

isc_result_t
ns_listenelt_create_http(isc_mem_t *mctx, in_port_t http_port, dns_acl_t *acl,
			 const uint16_t family, bool tls,
			 const ns_listen_tls_params_t *tls_params,
			 isc_tlsctx_cache_t *tlsctx_cache, char **endpoints,
			 size_t nendpoints, const uint32_t max_clients,
			 const uint32_t max_streams, ns_listenelt_t **target);
/*%<
 * Create a listen-on list element for HTTP(S).
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
ns_listenlist_default(isc_mem_t *mctx, in_port_t port, bool enabled,
		      const uint16_t family, ns_listenlist_t **target);
/*%<
 * Create a listen-on list with default contents, matching
 * all addresses with port 'port' (if 'enabled' is true),
 * or no addresses (if 'enabled' is false).
 */
