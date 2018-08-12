/*	$NetBSD: iptable.h,v 1.1.1.1 2018/08/12 12:08:19 christos Exp $	*/

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


#ifndef DNS_IPTABLE_H
#define DNS_IPTABLE_H 1

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/radix.h>

#include <dns/types.h>

struct dns_iptable {
	unsigned int		magic;
	isc_mem_t		*mctx;
	isc_refcount_t		refcount;
	isc_radix_tree_t	*radix;
	ISC_LINK(dns_iptable_t)	nextincache;
};

#define DNS_IPTABLE_MAGIC	ISC_MAGIC('T','a','b','l')
#define DNS_IPTABLE_VALID(a)	ISC_MAGIC_VALID(a, DNS_IPTABLE_MAGIC)

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
dns_iptable_create(isc_mem_t *mctx, dns_iptable_t **target);
/*
 * Create a new IP table and the underlying radix structure
 */

isc_result_t
dns_iptable_addprefix(dns_iptable_t *tab, const isc_netaddr_t *addr,
		      isc_uint16_t bitlen, isc_boolean_t pos);
isc_result_t
dns_iptable_addprefix2(dns_iptable_t *tab, const isc_netaddr_t *addr,
		       isc_uint16_t bitlen, isc_boolean_t pos,
		       isc_boolean_t is_ecs);
/*
 * Add an IP prefix to an existing IP table
 */

isc_result_t
dns_iptable_merge(dns_iptable_t *tab, dns_iptable_t *source, isc_boolean_t pos);
/*
 * Merge one IP table into another one.
 */

void
dns_iptable_attach(dns_iptable_t *source, dns_iptable_t **target);

void
dns_iptable_detach(dns_iptable_t **tabp);

ISC_LANG_ENDDECLS

#endif /* DNS_IPTABLE_H */
