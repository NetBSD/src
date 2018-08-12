/*	$NetBSD: iptable.c,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#include <config.h>

#include <isc/mem.h>
#include <isc/radix.h>
#include <isc/util.h>

#include <dns/acl.h>

static void destroy_iptable(dns_iptable_t *dtab);

/*
 * Create a new IP table and the underlying radix structure
 */
isc_result_t
dns_iptable_create(isc_mem_t *mctx, dns_iptable_t **target) {
	isc_result_t result;
	dns_iptable_t *tab;

	tab = isc_mem_get(mctx, sizeof(*tab));
	if (tab == NULL)
		return (ISC_R_NOMEMORY);
	tab->mctx = NULL;
	isc_mem_attach(mctx, &tab->mctx);
	isc_refcount_init(&tab->refcount, 1);
	tab->radix = NULL;
	tab->magic = DNS_IPTABLE_MAGIC;

	result = isc_radix_create(mctx, &tab->radix, RADIX_MAXBITS);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	*target = tab;
	return (ISC_R_SUCCESS);

 cleanup:
	dns_iptable_detach(&tab);
	return (result);
}

static isc_boolean_t dns_iptable_neg = ISC_FALSE;
static isc_boolean_t dns_iptable_pos = ISC_TRUE;

/*
 * Add an IP prefix to an existing IP table
 */
isc_result_t
dns_iptable_addprefix(dns_iptable_t *tab, const isc_netaddr_t *addr,
		      isc_uint16_t bitlen, isc_boolean_t pos)
{
	return(dns_iptable_addprefix2(tab, addr, bitlen, pos, ISC_FALSE));
}

isc_result_t
dns_iptable_addprefix2(dns_iptable_t *tab, const isc_netaddr_t *addr,
		       isc_uint16_t bitlen, isc_boolean_t pos,
		       isc_boolean_t is_ecs)
{
	isc_result_t result;
	isc_prefix_t pfx;
	isc_radix_node_t *node = NULL;
	int i;

	INSIST(DNS_IPTABLE_VALID(tab));
	INSIST(tab->radix);

	NETADDR_TO_PREFIX_T(addr, pfx, bitlen, is_ecs);

	result = isc_radix_insert(tab->radix, &node, NULL, &pfx);
	if (result != ISC_R_SUCCESS) {
		isc_refcount_destroy(&pfx.refcount);
		return(result);
	}

	/* If a node already contains data, don't overwrite it */
	if (pfx.family == AF_UNSPEC) {
		/* "any" or "none" */
		INSIST(pfx.bitlen == 0);
		for (i = 0; i < RADIX_FAMILIES; i++) {
			if (node->data[i] == NULL) {
				node->data[i] = pos ? &dns_iptable_pos
						    : &dns_iptable_neg;
			}
		}
	} else {
		/* any other prefix */
		int fam = ISC_RADIX_FAMILY(&pfx);
		if (node->data[fam] == NULL) {
			node->data[fam] = pos ? &dns_iptable_pos
						 : &dns_iptable_neg;
		}
	}

	isc_refcount_destroy(&pfx.refcount);
	return (ISC_R_SUCCESS);
}

/*
 * Merge one IP table into another one.
 */
isc_result_t
dns_iptable_merge(dns_iptable_t *tab, dns_iptable_t *source, isc_boolean_t pos)
{
	isc_result_t result;
	isc_radix_node_t *node, *new_node;
	int i, max_node = 0;

	RADIX_WALK (source->radix->head, node) {
		new_node = NULL;
		result = isc_radix_insert (tab->radix, &new_node, node, NULL);

		if (result != ISC_R_SUCCESS)
			return(result);

		/*
		 * If we're negating a nested ACL, then we should
		 * reverse the sense of every node.  However, this
		 * could lead to a negative node in a nested ACL
		 * becoming a positive match in the parent, which
		 * could be a security risk.  To prevent this, we
		 * just leave the negative nodes negative.
		 */
		for (i = 0; i < RADIX_FAMILIES; i++) {
			if (!pos) {
				if (node->data[i] &&
				    *(isc_boolean_t *) node->data[i])
					new_node->data[i] = &dns_iptable_neg;
			}
			if (node->node_num[i] > max_node)
				max_node = node->node_num[i];
		}
	} RADIX_WALK_END;

	tab->radix->num_added_node += max_node;
	return (ISC_R_SUCCESS);
}

void
dns_iptable_attach(dns_iptable_t *source, dns_iptable_t **target) {
	REQUIRE(DNS_IPTABLE_VALID(source));
	isc_refcount_increment(&source->refcount, NULL);
	*target = source;
}

void
dns_iptable_detach(dns_iptable_t **tabp) {
	dns_iptable_t *tab = *tabp;
	unsigned int refs;
	REQUIRE(DNS_IPTABLE_VALID(tab));
	isc_refcount_decrement(&tab->refcount, &refs);
	if (refs == 0)
		destroy_iptable(tab);
	*tabp = NULL;
}

static void
destroy_iptable(dns_iptable_t *dtab) {

	REQUIRE(DNS_IPTABLE_VALID(dtab));

	if (dtab->radix != NULL) {
		isc_radix_destroy(dtab->radix, NULL);
		dtab->radix = NULL;
	}

	isc_refcount_destroy(&dtab->refcount);
	dtab->magic = 0;
	isc_mem_putanddetach(&dtab->mctx, dtab, sizeof(*dtab));
}
