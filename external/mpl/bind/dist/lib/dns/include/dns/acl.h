/*	$NetBSD: acl.h,v 1.8.2.1 2024/02/25 15:46:55 martin Exp $	*/

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

/*! \file dns/acl.h
 * \brief
 * Address match list handling.
 */

/***
 *** Imports
 ***/

#include <stdbool.h>

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/netaddr.h>
#include <isc/refcount.h>
#include <isc/rwlock.h>

#include <dns/geoip.h>
#include <dns/iptable.h>
#include <dns/name.h>
#include <dns/types.h>

/***
 *** Types
 ***/

typedef enum {
	dns_aclelementtype_ipprefix,
	dns_aclelementtype_keyname,
	dns_aclelementtype_nestedacl,
	dns_aclelementtype_localhost,
	dns_aclelementtype_localnets,
#if defined(HAVE_GEOIP2)
	dns_aclelementtype_geoip,
#endif /* HAVE_GEOIP2 */
	dns_aclelementtype_any
} dns_aclelementtype_t;

typedef struct dns_acl_port_transports {
	in_port_t port;
	uint32_t  transports;
	bool encrypted; /* for protocols with optional encryption (e.g. HTTP) */
	bool negative;
	ISC_LINK(struct dns_acl_port_transports) link;
} dns_acl_port_transports_t;

typedef struct dns_aclipprefix dns_aclipprefix_t;

struct dns_aclipprefix {
	isc_netaddr_t address; /* IP4/IP6 */
	unsigned int  prefixlen;
};

struct dns_aclelement {
	dns_aclelementtype_t type;
	bool		     negative;
	dns_name_t	     keyname;
#if defined(HAVE_GEOIP2)
	dns_geoip_elem_t geoip_elem;
#endif /* HAVE_GEOIP2 */
	dns_acl_t *nestedacl;
	int	   node_num;
};

#define dns_acl_node_count(acl) acl->iptable->radix->num_added_node

struct dns_acl {
	unsigned int	  magic;
	isc_mem_t	 *mctx;
	isc_refcount_t	  refcount;
	dns_iptable_t	 *iptable;
	dns_aclelement_t *elements;
	bool		  has_negatives;
	unsigned int	  alloc;	 /*%< Elements allocated */
	unsigned int	  length;	 /*%< Elements initialized */
	char		 *name;		 /*%< Temporary use only */
	ISC_LINK(dns_acl_t) nextincache; /*%< Ditto */
	ISC_LIST(dns_acl_port_transports_t) ports_and_transports;
	size_t port_proto_entries;
};

struct dns_aclenv {
	unsigned int   magic;
	isc_mem_t     *mctx;
	isc_refcount_t references;

	isc_rwlock_t rwlock; /*%< Locks localhost and localnets */
	dns_acl_t   *localhost;
	dns_acl_t   *localnets;

	bool match_mapped;
#if defined(HAVE_GEOIP2)
	dns_geoip_databases_t *geoip;
#endif /* HAVE_GEOIP2 */
};

#define DNS_ACL_MAGIC	 ISC_MAGIC('D', 'a', 'c', 'l')
#define DNS_ACL_VALID(a) ISC_MAGIC_VALID(a, DNS_ACL_MAGIC)

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
dns_acl_create(isc_mem_t *mctx, int n, dns_acl_t **target);
/*%<
 * Create a new ACL, including an IP table and an array with room
 * for 'n' ACL elements.  The elements are uninitialized and the
 * length is 0.
 */

isc_result_t
dns_acl_any(isc_mem_t *mctx, dns_acl_t **target);
/*%<
 * Create a new ACL that matches everything.
 */

isc_result_t
dns_acl_none(isc_mem_t *mctx, dns_acl_t **target);
/*%<
 * Create a new ACL that matches nothing.
 */

bool
dns_acl_isany(dns_acl_t *acl);
/*%<
 * Test whether ACL is set to "{ any; }"
 */

bool
dns_acl_isnone(dns_acl_t *acl);
/*%<
 * Test whether ACL is set to "{ none; }"
 */

isc_result_t
dns_acl_merge(dns_acl_t *dest, dns_acl_t *source, bool pos);
/*%<
 * Merge the contents of one ACL into another.  Call dns_iptable_merge()
 * for the IP tables, then concatenate the element arrays.
 *
 * If pos is set to false, then the nested ACL is to be negated.  This
 * means reverse the sense of each *positive* element or IP table node,
 * but leave negatives alone, so as to prevent a double-negative causing
 * an unexpected positive match in the parent ACL.
 */

void
dns_acl_attach(dns_acl_t *source, dns_acl_t **target);
/*%<
 * Attach to acl 'source'.
 *
 * Requires:
 *\li	'source' to be a valid acl.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_acl_detach(dns_acl_t **aclp);
/*%<
 * Detach the acl. On final detach the acl must not be linked on any
 * list.
 *
 * Requires:
 *\li	'*aclp' to be a valid acl.
 *
 * Insists:
 *\li	'*aclp' is not linked on final detach.
 */

bool
dns_acl_isinsecure(const dns_acl_t *a);
/*%<
 * Return #true iff the acl 'a' is considered insecure, that is,
 * if it contains IP addresses other than those of the local host.
 * This is intended for applications such as printing warning
 * messages for suspect ACLs; it is not intended for making access
 * control decisions.  We make no guarantee that an ACL for which
 * this function returns #false is safe.
 */

bool
dns_acl_allowed(isc_netaddr_t *addr, const dns_name_t *signer, dns_acl_t *acl,
		dns_aclenv_t *aclenv);
/*%<
 * Return #true iff the 'addr', 'signer', or ECS values are
 * permitted by 'acl' in environment 'aclenv'.
 */

isc_result_t
dns_aclenv_create(isc_mem_t *mctx, dns_aclenv_t **envp);
/*%<
 * Create ACL environment, setting up localhost and localnets ACLs
 */

void
dns_aclenv_copy(dns_aclenv_t *t, dns_aclenv_t *s);
/*%<
 * Copy the ACLs from one ACL environment object to another.
 *
 * Requires:
 *\li	both 's' and 't' are valid ACL environments.
 */

void
dns_aclenv_set(dns_aclenv_t *env, dns_acl_t *localhost, dns_acl_t *localnets);
/*%<
 * Attach the 'localhost' and 'localnets' arguments to 'env' ACL environment
 */

void
dns_aclenv_attach(dns_aclenv_t *source, dns_aclenv_t **targetp);
/*%<
 * Attach '*targetp' to ACL environment 'source'.
 *
 * Requires:
 *\li	'source' is a valid ACL environment.
 *\li	'targetp' is not NULL and '*targetp' is NULL.
 */

void
dns_aclenv_detach(dns_aclenv_t **aclenvp);
/*%<
 * Detach an ACL environment; on final detach, destroy it.
 *
 * Requires:
 *\li	'*aclenvp' to be a valid ACL environment
 */

isc_result_t
dns_acl_match(const isc_netaddr_t *reqaddr, const dns_name_t *reqsigner,
	      const dns_acl_t *acl, dns_aclenv_t *env, int *match,
	      const dns_aclelement_t **matchelt);
/*%<
 * General, low-level ACL matching.  This is expected to
 * be useful even for weird stuff like the topology and sortlist statements.
 *
 * Match the address 'reqaddr', and optionally the key name 'reqsigner',
 * against 'acl'.  'reqsigner' may be NULL.
 *
 * If there is a match, '*match' will be set to an integer whose absolute
 * value corresponds to the order in which the matching value was inserted
 * into the ACL.  For a positive match, this value will be positive; for a
 * negative match, it will be negative.
 *
 * If there is no match, *match will be set to zero.
 *
 * If there is a match in the element list (either positive or negative)
 * and 'matchelt' is non-NULL, *matchelt will be pointed to the matching
 * element.
 *
 * 'env' points to the current ACL environment, including the
 * current values of localhost and localnets and (if applicable)
 * the GeoIP context.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		Always succeeds.
 */

bool
dns_aclelement_match(const isc_netaddr_t *reqaddr, const dns_name_t *reqsigner,
		     const dns_aclelement_t *e, dns_aclenv_t *env,
		     const dns_aclelement_t **matchelt);
/*%<
 * Like dns_acl_match, but matches against the single ACL element 'e'
 * rather than a complete ACL, and returns true iff it matched.
 *
 * To determine whether the match was positive or negative, the
 * caller should examine e->negative.  Since the element 'e' may be
 * a reference to a named ACL or a nested ACL, a matching element
 * returned through 'matchelt' is not necessarily 'e' itself.
 */

isc_result_t
dns_acl_match_port_transport(const isc_netaddr_t      *reqaddr,
			     const in_port_t	       local_port,
			     const isc_nmsocket_type_t transport,
			     const bool encrypted, const dns_name_t *reqsigner,
			     const dns_acl_t *acl, dns_aclenv_t *env,
			     int *match, const dns_aclelement_t **matchelt);
/*%<
 * Like dns_acl_match, but able to match the server port and
 * transport, as well as encryption status.
 *
 * Requires:
 *\li		'reqaddr' is not 'NULL';
 *\li		'acl' is a valid ACL object.
 */

void
dns_acl_add_port_transports(dns_acl_t *acl, const in_port_t port,
			    const uint32_t transports, const bool encrypted,
			    const bool negative);
/*%<
 * Adds a "port-transports" entry to the specified ACL. Transports
 * are specified as a bit-set 'transports' consisting of entries
 * defined in the isc_nmsocket_type enumeration.
 *
 * Requires:
 *\li		'acl' is a valid ACL object;
 *\li		either 'port' or 'transports' is not equal to 0.
 */

void
dns_acl_merge_ports_transports(dns_acl_t *dest, dns_acl_t *source, bool pos);
/*%<
 * Merges "port-transports" entries from the 'dest' ACL into
 * the 'source' ACL. The 'pos' parameter works in a way similar to
 * 'dns_acl_merge()'.
 *
 * Requires:
 *\li		'dest' is a valid ACL object;
 *\li		'source' is a valid ACL object.
 */

ISC_LANG_ENDDECLS
