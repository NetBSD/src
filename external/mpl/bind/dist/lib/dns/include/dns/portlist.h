/*	$NetBSD: portlist.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

/*! \file dns/portlist.h */

#include <isc/lang.h>
#include <isc/net.h>
#include <isc/types.h>

#include <dns/types.h>

#ifndef DNS_PORTLIST_H
#define DNS_PORTLIST_H 1

ISC_LANG_BEGINDECLS

isc_result_t
dns_portlist_create(isc_mem_t *mctx, dns_portlist_t **portlistp);
/*%<
 * Create a port list.
 *
 * Requires:
 *\li	'mctx' to be valid.
 *\li	'portlistp' to be non NULL and '*portlistp' to be NULL;
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 */

isc_result_t
dns_portlist_add(dns_portlist_t *portlist, int af, in_port_t port);
/*%<
 * Add the given <port,af> tuple to the portlist.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'af' to be AF_INET or AF_INET6
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

void
dns_portlist_remove(dns_portlist_t *portlist, int af, in_port_t port);
/*%<
 * Remove the given <port,af> tuple to the portlist.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'af' to be AF_INET or AF_INET6
 */

isc_boolean_t
dns_portlist_match(dns_portlist_t *portlist, int af, in_port_t port);
/*%<
 * Find the given <port,af> tuple to the portlist.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'af' to be AF_INET or AF_INET6
 *
 * Returns
 * \li	#ISC_TRUE if the tuple is found, ISC_FALSE otherwise.
 */

void
dns_portlist_attach(dns_portlist_t *portlist, dns_portlist_t **portlistp);
/*%<
 * Attach to a port list.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'portlistp' to be non NULL and '*portlistp' to be NULL;
 */

void
dns_portlist_detach(dns_portlist_t **portlistp);
/*%<
 * Detach from a port list.
 *
 * Requires:
 *\li	'*portlistp' to be valid.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_PORTLIST_H */
