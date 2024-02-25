/*	$NetBSD: forward.h,v 1.6.2.1 2024/02/25 15:46:56 martin Exp $	*/

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

/*! \file dns/forward.h */

#include <isc/lang.h>
#include <isc/result.h>
#include <isc/sockaddr.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

struct dns_forwarder {
	isc_sockaddr_t addr;
	ISC_LINK(dns_forwarder_t) link;
};

typedef ISC_LIST(struct dns_forwarder) dns_forwarderlist_t;

struct dns_forwarders {
	dns_forwarderlist_t fwdrs;
	dns_fwdpolicy_t	    fwdpolicy;
};

isc_result_t
dns_fwdtable_create(isc_mem_t *mctx, dns_fwdtable_t **fwdtablep);
/*%<
 * Creates a new forwarding table.
 *
 * Requires:
 * \li 	mctx is a valid memory context.
 * \li	fwdtablep != NULL && *fwdtablep == NULL
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_fwdtable_addfwd(dns_fwdtable_t *fwdtable, const dns_name_t *name,
		    dns_forwarderlist_t *fwdrs, dns_fwdpolicy_t policy);
isc_result_t
dns_fwdtable_add(dns_fwdtable_t *fwdtable, const dns_name_t *name,
		 isc_sockaddrlist_t *addrs, dns_fwdpolicy_t policy);
/*%<
 * Adds an entry to the forwarding table.  The entry associates
 * a domain with a list of forwarders and a forwarding policy.  The
 * addrs/fwdrs list is copied if not empty, so the caller should free
 * its copy.
 *
 * Requires:
 * \li	fwdtable is a valid forwarding table.
 * \li	name is a valid name
 * \li	addrs/fwdrs is a valid list of isc_sockaddr/dns_forwarder
 *      structures, which may be empty.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_fwdtable_delete(dns_fwdtable_t *fwdtable, const dns_name_t *name);
/*%<
 * Removes an entry for 'name' from the forwarding table.  If an entry
 * that exactly matches 'name' does not exist, ISC_R_NOTFOUND will be returned.
 *
 * Requires:
 * \li	fwdtable is a valid forwarding table.
 * \li	name is a valid name
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 * \li	#ISC_R_NOSPACE
 */

isc_result_t
dns_fwdtable_find(dns_fwdtable_t *fwdtable, const dns_name_t *name,
		  dns_name_t *foundname, dns_forwarders_t **forwardersp);
/*%<
 * Finds a domain in the forwarding table.  The closest matching parent
 * domain is returned.
 *
 * Requires:
 * \li	fwdtable is a valid forwarding table.
 * \li	name is a valid name
 * \li	forwardersp != NULL && *forwardersp == NULL
 * \li	foundname to be NULL or a valid name with buffer.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS         Success
 * \li	#DNS_R_PARTIALMATCH    Superdomain found with data
 * \li	#ISC_R_NOTFOUND        No match
 * \li	#ISC_R_NOSPACE         Concatenating nodes to form foundname failed
 */

void
dns_fwdtable_destroy(dns_fwdtable_t **fwdtablep);
/*%<
 * Destroys a forwarding table.
 *
 * Requires:
 * \li	fwtablep != NULL && *fwtablep != NULL
 *
 * Ensures:
 * \li	all memory associated with the forwarding table is freed.
 */

ISC_LANG_ENDDECLS
