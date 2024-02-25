/*	$NetBSD: transport.h,v 1.2.2.2 2024/02/25 15:46:59 martin Exp $	*/

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

#include <dns/name.h>

typedef enum {
	DNS_TRANSPORT_NONE = 0,
	DNS_TRANSPORT_UDP = 1,
	DNS_TRANSPORT_TCP = 2,
	DNS_TRANSPORT_TLS = 3,
	DNS_TRANSPORT_HTTP = 4,
	DNS_TRANSPORT_COUNT = 5,
} dns_transport_type_t;

typedef enum {
	DNS_HTTP_GET = 0,
	DNS_HTTP_POST = 1,
} dns_http_mode_t;

typedef struct dns_transport	  dns_transport_t;
typedef struct dns_transport_list dns_transport_list_t;

dns_transport_t *
dns_transport_new(const dns_name_t *name, dns_transport_type_t type,
		  dns_transport_list_t *list);
/*%<
 * Create a new transport object with name 'name' and type 'type',
 * and append it to 'list'.
 */

dns_transport_type_t
dns_transport_get_type(dns_transport_t *transport);
char *
dns_transport_get_certfile(dns_transport_t *transport);
char *
dns_transport_get_keyfile(dns_transport_t *transport);
char *
dns_transport_get_cafile(dns_transport_t *transport);
char *
dns_transport_get_remote_hostname(dns_transport_t *transport);
char *
dns_transport_get_endpoint(dns_transport_t *transport);
dns_http_mode_t
dns_transport_get_mode(dns_transport_t *transport);
char *
dns_transport_get_ciphers(dns_transport_t *transport);
char *
dns_transport_get_tlsname(dns_transport_t *transport);
uint32_t
dns_transport_get_tls_versions(const dns_transport_t *transport);
bool
dns_transport_get_prefer_server_ciphers(const dns_transport_t *transport,
					bool		      *preferp);
/*%<
 * Getter functions: return the type, cert file, key file, CA file,
 * hostname, HTTP endpoint, or HTTP mode (GET or POST) for 'transport'.
 *
 * dns_transport_get_prefer_server_ciphers() returns 'true' is value
 * was set, 'false' otherwise. The actual value is returned via
 * 'preferp' pointer.
 */

void
dns_transport_set_certfile(dns_transport_t *transport, const char *certfile);
void
dns_transport_set_keyfile(dns_transport_t *transport, const char *keyfile);
void
dns_transport_set_cafile(dns_transport_t *transport, const char *cafile);
void
dns_transport_set_remote_hostname(dns_transport_t *transport,
				  const char	  *hostname);
void
dns_transport_set_endpoint(dns_transport_t *transport, const char *endpoint);
void
dns_transport_set_mode(dns_transport_t *transport, dns_http_mode_t mode);
void
dns_transport_set_ciphers(dns_transport_t *transport, const char *ciphers);
void
dns_transport_set_tlsname(dns_transport_t *transport, const char *tlsname);

void
dns_transport_set_tls_versions(dns_transport_t *transport,
			       const uint32_t	tls_versions);
void
dns_transport_set_prefer_server_ciphers(dns_transport_t *transport,
					const bool	 prefer);
/*%<
 * Setter functions: set the type, cert file, key file, CA file,
 * hostname, HTTP endpoint, or HTTP mode (GET or POST) for 'transport'.
 *
 * Requires:
 *\li	'transport' is valid.
 *\li	'transport' is of type DNS_TRANSPORT_TLS or DNS_TRANSPORT_HTTP
 *	(for certfile, keyfile, cafile, or hostname).
 *\li	'transport' is of type DNS_TRANSPORT_HTTP (for endpoint or mode).
 */

void
dns_transport_attach(dns_transport_t *source, dns_transport_t **targetp);
/*%<
 * Attach to a transport object.
 *
 * Requires:
 *\li	'source' is a valid transport.
 *\li	'targetp' is not NULL and '*targetp' is NULL.
 */

void
dns_transport_detach(dns_transport_t **transportp);
/*%<
 * Detach a transport object; destroy it if there are no remaining
 * references.
 *
 * Requires:
 *\li	'transportp' is not NULL.
 *\li	'*transportp' is a valid transport.
 */

dns_transport_t *
dns_transport_find(const dns_transport_type_t type, const dns_name_t *name,
		   dns_transport_list_t *list);
/*%<
 * Find a transport matching type 'type' and name `name` in 'list'.
 *
 * Requires:
 *\li	'list' is valid.
 *\li	'list' contains a table of type 'type' transports.
 */

dns_transport_list_t *
dns_transport_list_new(isc_mem_t *mctx);
/*%<
 * Create a new transport list.
 */

void
dns_transport_list_attach(dns_transport_list_t	*source,
			  dns_transport_list_t **targetp);
/*%<
 * Attach to a transport list.
 *
 * Requires:
 *\li	'source' is a valid transport list.
 *\li	'targetp' is not NULL and '*targetp' is NULL.
 */

void
dns_transport_list_detach(dns_transport_list_t **listp);
/*%<
 * Detach a transport list; destroy it if there are no remaining
 * references.
 *
 * Requires:
 *\li	'listp' is not NULL.
 *\li	'*listp' is a valid transport list.
 */
