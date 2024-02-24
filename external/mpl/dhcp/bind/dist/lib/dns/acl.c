/*	$NetBSD: acl.c,v 1.1.2.2 2024/02/24 13:06:55 martin Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/mem.h>
#include <isc/once.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/iptable.h>

/*
 * Create a new ACL, including an IP table and an array with room
 * for 'n' ACL elements.  The elements are uninitialized and the
 * length is 0.
 */
isc_result_t
dns_acl_create(isc_mem_t *mctx, int n, dns_acl_t **target) {
	isc_result_t result;
	dns_acl_t *acl;

	/*
	 * Work around silly limitation of isc_mem_get().
	 */
	if (n == 0) {
		n = 1;
	}

	acl = isc_mem_get(mctx, sizeof(*acl));

	acl->mctx = NULL;
	isc_mem_attach(mctx, &acl->mctx);

	acl->name = NULL;

	isc_refcount_init(&acl->refcount, 1);

	result = dns_iptable_create(mctx, &acl->iptable);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, acl, sizeof(*acl));
		return (result);
	}

	acl->elements = NULL;
	acl->alloc = 0;
	acl->length = 0;
	acl->has_negatives = false;

	ISC_LINK_INIT(acl, nextincache);
	/*
	 * Must set magic early because we use dns_acl_detach() to clean up.
	 */
	acl->magic = DNS_ACL_MAGIC;

	acl->elements = isc_mem_get(mctx, n * sizeof(dns_aclelement_t));
	acl->alloc = n;
	memset(acl->elements, 0, n * sizeof(dns_aclelement_t));
	*target = acl;
	return (ISC_R_SUCCESS);
}

/*
 * Create a new ACL and initialize it with the value "any" or "none",
 * depending on the value of the "neg" parameter.
 * "any" is a positive iptable entry with bit length 0.
 * "none" is the same as "!any".
 */
static isc_result_t
dns_acl_anyornone(isc_mem_t *mctx, bool neg, dns_acl_t **target) {
	isc_result_t result;
	dns_acl_t *acl = NULL;

	result = dns_acl_create(mctx, 0, &acl);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_iptable_addprefix(acl->iptable, NULL, 0, !neg);
	if (result != ISC_R_SUCCESS) {
		dns_acl_detach(&acl);
		return (result);
	}

	*target = acl;
	return (result);
}

/*
 * Create a new ACL that matches everything.
 */
isc_result_t
dns_acl_any(isc_mem_t *mctx, dns_acl_t **target) {
	return (dns_acl_anyornone(mctx, false, target));
}

/*
 * Create a new ACL that matches nothing.
 */
isc_result_t
dns_acl_none(isc_mem_t *mctx, dns_acl_t **target) {
	return (dns_acl_anyornone(mctx, true, target));
}

/*
 * If pos is true, test whether acl is set to "{ any; }"
 * If pos is false, test whether acl is set to "{ none; }"
 */
static bool
dns_acl_isanyornone(dns_acl_t *acl, bool pos) {
	/* Should never happen but let's be safe */
	if (acl == NULL || acl->iptable == NULL ||
	    acl->iptable->radix == NULL || acl->iptable->radix->head == NULL ||
	    acl->iptable->radix->head->prefix == NULL)
	{
		return (false);
	}

	if (acl->length != 0 || dns_acl_node_count(acl) != 1) {
		return (false);
	}

	if (acl->iptable->radix->head->prefix->bitlen == 0 &&
	    acl->iptable->radix->head->data[0] != NULL &&
	    acl->iptable->radix->head->data[0] ==
		    acl->iptable->radix->head->data[1] &&
	    *(bool *)(acl->iptable->radix->head->data[0]) == pos)
	{
		return (true);
	}

	return (false); /* All others */
}

/*
 * Test whether acl is set to "{ any; }"
 */
bool
dns_acl_isany(dns_acl_t *acl) {
	return (dns_acl_isanyornone(acl, true));
}

/*
 * Test whether acl is set to "{ none; }"
 */
bool
dns_acl_isnone(dns_acl_t *acl) {
	return (dns_acl_isanyornone(acl, false));
}

/*
 * Determine whether a given address or signer matches a given ACL.
 * For a match with a positive ACL element or iptable radix entry,
 * return with a positive value in match; for a match with a negated ACL
 * element or radix entry, return with a negative value in match.
 */

isc_result_t
dns_acl_match(const isc_netaddr_t *reqaddr, const dns_name_t *reqsigner,
	      const dns_acl_t *acl, const dns_aclenv_t *env, int *match,
	      const dns_aclelement_t **matchelt) {
	uint16_t bitlen;
	isc_prefix_t pfx;
	isc_radix_node_t *node = NULL;
	const isc_netaddr_t *addr = reqaddr;
	isc_netaddr_t v4addr;
	isc_result_t result;
	int match_num = -1;
	unsigned int i;

	REQUIRE(reqaddr != NULL);
	REQUIRE(matchelt == NULL || *matchelt == NULL);

	if (env != NULL && env->match_mapped && addr->family == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&addr->type.in6))
	{
		isc_netaddr_fromv4mapped(&v4addr, addr);
		addr = &v4addr;
	}

	/* Always match with host addresses. */
	bitlen = (addr->family == AF_INET6) ? 128 : 32;
	NETADDR_TO_PREFIX_T(addr, pfx, bitlen);

	/* Assume no match. */
	*match = 0;

	/* Search radix. */
	result = isc_radix_search(acl->iptable->radix, &node, &pfx);

	/* Found a match. */
	if (result == ISC_R_SUCCESS && node != NULL) {
		int fam = ISC_RADIX_FAMILY(&pfx);
		match_num = node->node_num[fam];
		if (*(bool *)node->data[fam]) {
			*match = match_num;
		} else {
			*match = -match_num;
		}
	}

	isc_refcount_destroy(&pfx.refcount);

	/* Now search non-radix elements for a match with a lower node_num. */
	for (i = 0; i < acl->length; i++) {
		dns_aclelement_t *e = &acl->elements[i];

		/* Already found a better match? */
		if (match_num != -1 && match_num < e->node_num) {
			break;
		}

		if (dns_aclelement_match(reqaddr, reqsigner, e, env, matchelt))
		{
			if (match_num == -1 || e->node_num < match_num) {
				if (e->negative) {
					*match = -e->node_num;
				} else {
					*match = e->node_num;
				}
			}
			break;
		}
	}

	return (ISC_R_SUCCESS);
}

/*
 * Merge the contents of one ACL into another.  Call dns_iptable_merge()
 * for the IP tables, then concatenate the element arrays.
 *
 * If pos is set to false, then the nested ACL is to be negated.  This
 * means reverse the sense of each *positive* element or IP table node,
 * but leave negatives alone, so as to prevent a double-negative causing
 * an unexpected positive match in the parent ACL.
 */
isc_result_t
dns_acl_merge(dns_acl_t *dest, dns_acl_t *source, bool pos) {
	isc_result_t result;
	unsigned int newalloc, nelem, i;
	int max_node = 0, nodes;

	/* Resize the element array if needed. */
	if (dest->length + source->length > dest->alloc) {
		void *newmem;

		newalloc = dest->alloc + source->alloc;
		if (newalloc < 4) {
			newalloc = 4;
		}

		newmem = isc_mem_get(dest->mctx,
				     newalloc * sizeof(dns_aclelement_t));

		/* Zero. */
		memset(newmem, 0, newalloc * sizeof(dns_aclelement_t));

		/* Copy in the original elements */
		memmove(newmem, dest->elements,
			dest->length * sizeof(dns_aclelement_t));

		/* Release the memory for the old elements array */
		isc_mem_put(dest->mctx, dest->elements,
			    dest->alloc * sizeof(dns_aclelement_t));
		dest->elements = newmem;
		dest->alloc = newalloc;
	}

	/*
	 * Now copy in the new elements, increasing their node_num
	 * values so as to keep the new ACL consistent.  If we're
	 * negating, then negate positive elements, but keep negative
	 * elements the same for security reasons.
	 */
	nelem = dest->length;
	dest->length += source->length;
	for (i = 0; i < source->length; i++) {
		if (source->elements[i].node_num > max_node) {
			max_node = source->elements[i].node_num;
		}

		/* Copy type. */
		dest->elements[nelem + i].type = source->elements[i].type;

		/* Adjust node numbering. */
		dest->elements[nelem + i].node_num =
			source->elements[i].node_num + dns_acl_node_count(dest);

		/* Duplicate nested acl. */
		if (source->elements[i].type == dns_aclelementtype_nestedacl &&
		    source->elements[i].nestedacl != NULL)
		{
			dns_acl_attach(source->elements[i].nestedacl,
				       &dest->elements[nelem + i].nestedacl);
		}

		/* Duplicate key name. */
		if (source->elements[i].type == dns_aclelementtype_keyname) {
			dns_name_init(&dest->elements[nelem + i].keyname, NULL);
			dns_name_dup(&source->elements[i].keyname, dest->mctx,
				     &dest->elements[nelem + i].keyname);
		}

#if defined(HAVE_GEOIP2)
		/* Duplicate GeoIP data */
		if (source->elements[i].type == dns_aclelementtype_geoip) {
			dest->elements[nelem + i].geoip_elem =
				source->elements[i].geoip_elem;
		}
#endif /* if defined(HAVE_GEOIP2) */

		/* reverse sense of positives if this is a negative acl */
		if (!pos && !source->elements[i].negative) {
			dest->elements[nelem + i].negative = true;
		} else {
			dest->elements[nelem + i].negative =
				source->elements[i].negative;
		}
	}

	/*
	 * Merge the iptables.  Make sure the destination ACL's
	 * node_count value is set correctly afterward.
	 */
	nodes = max_node + dns_acl_node_count(dest);
	result = dns_iptable_merge(dest->iptable, source->iptable, pos);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	if (nodes > dns_acl_node_count(dest)) {
		dns_acl_node_count(dest) = nodes;
	}

	return (ISC_R_SUCCESS);
}

/*
 * Like dns_acl_match, but matches against the single ACL element 'e'
 * rather than a complete ACL, and returns true iff it matched.
 *
 * To determine whether the match was positive or negative, the
 * caller should examine e->negative.  Since the element 'e' may be
 * a reference to a named ACL or a nested ACL, a matching element
 * returned through 'matchelt' is not necessarily 'e' itself.
 */

bool
dns_aclelement_match(const isc_netaddr_t *reqaddr, const dns_name_t *reqsigner,
		     const dns_aclelement_t *e, const dns_aclenv_t *env,
		     const dns_aclelement_t **matchelt) {
	dns_acl_t *inner = NULL;
	int indirectmatch;
	isc_result_t result;

	switch (e->type) {
	case dns_aclelementtype_keyname:
		if (reqsigner != NULL && dns_name_equal(reqsigner, &e->keyname))
		{
			if (matchelt != NULL) {
				*matchelt = e;
			}
			return (true);
		} else {
			return (false);
		}

	case dns_aclelementtype_nestedacl:
		inner = e->nestedacl;
		break;

	case dns_aclelementtype_localhost:
		if (env == NULL || env->localhost == NULL) {
			return (false);
		}
		inner = env->localhost;
		break;

	case dns_aclelementtype_localnets:
		if (env == NULL || env->localnets == NULL) {
			return (false);
		}
		inner = env->localnets;
		break;

#if defined(HAVE_GEOIP2)
	case dns_aclelementtype_geoip:
		if (env == NULL || env->geoip == NULL) {
			return (false);
		}
		return (dns_geoip_match(reqaddr, env->geoip, &e->geoip_elem));
#endif /* if defined(HAVE_GEOIP2) */
	default:
		UNREACHABLE();
	}

	result = dns_acl_match(reqaddr, reqsigner, inner, env, &indirectmatch,
			       matchelt);
	INSIST(result == ISC_R_SUCCESS);

	/*
	 * Treat negative matches in indirect ACLs as "no match".
	 * That way, a negated indirect ACL will never become a
	 * surprise positive match through double negation.
	 * XXXDCL this should be documented.
	 */
	if (indirectmatch > 0) {
		if (matchelt != NULL) {
			*matchelt = e;
		}
		return (true);
	}

	/*
	 * A negative indirect match may have set *matchelt, but we don't
	 * want it set when we return.
	 */
	if (matchelt != NULL) {
		*matchelt = NULL;
	}

	return (false);
}

void
dns_acl_attach(dns_acl_t *source, dns_acl_t **target) {
	REQUIRE(DNS_ACL_VALID(source));

	isc_refcount_increment(&source->refcount);
	*target = source;
}

static void
destroy(dns_acl_t *dacl) {
	unsigned int i;

	INSIST(!ISC_LINK_LINKED(dacl, nextincache));

	for (i = 0; i < dacl->length; i++) {
		dns_aclelement_t *de = &dacl->elements[i];
		if (de->type == dns_aclelementtype_keyname) {
			dns_name_free(&de->keyname, dacl->mctx);
		} else if (de->type == dns_aclelementtype_nestedacl) {
			dns_acl_detach(&de->nestedacl);
		}
	}
	if (dacl->elements != NULL) {
		isc_mem_put(dacl->mctx, dacl->elements,
			    dacl->alloc * sizeof(dns_aclelement_t));
	}
	if (dacl->name != NULL) {
		isc_mem_free(dacl->mctx, dacl->name);
	}
	if (dacl->iptable != NULL) {
		dns_iptable_detach(&dacl->iptable);
	}
	isc_refcount_destroy(&dacl->refcount);
	dacl->magic = 0;
	isc_mem_putanddetach(&dacl->mctx, dacl, sizeof(*dacl));
}

void
dns_acl_detach(dns_acl_t **aclp) {
	REQUIRE(aclp != NULL && DNS_ACL_VALID(*aclp));
	dns_acl_t *acl = *aclp;
	*aclp = NULL;

	if (isc_refcount_decrement(&acl->refcount) == 1) {
		destroy(acl);
	}
}

static isc_once_t insecure_prefix_once = ISC_ONCE_INIT;
static isc_mutex_t insecure_prefix_lock;
static bool insecure_prefix_found;

static void
initialize_action(void) {
	isc_mutex_init(&insecure_prefix_lock);
}

/*
 * Called via isc_radix_process() to find IP table nodes that are
 * insecure.
 */
static void
is_insecure(isc_prefix_t *prefix, void **data) {
	/*
	 * If all nonexistent or negative then this node is secure.
	 */
	if ((data[0] == NULL || !*(bool *)data[0]) &&
	    (data[1] == NULL || !*(bool *)data[1]))
	{
		return;
	}

	/*
	 * If a loopback address found and the other family
	 * entry doesn't exist or is negative, return.
	 */
	if (prefix->bitlen == 32 &&
	    htonl(prefix->add.sin.s_addr) == INADDR_LOOPBACK &&
	    (data[1] == NULL || !*(bool *)data[1]))
	{
		return;
	}

	if (prefix->bitlen == 128 && IN6_IS_ADDR_LOOPBACK(&prefix->add.sin6) &&
	    (data[0] == NULL || !*(bool *)data[0]))
	{
		return;
	}

	/* Non-negated, non-loopback */
	insecure_prefix_found = true; /* LOCKED */
	return;
}

/*
 * Return true iff the acl 'a' is considered insecure, that is,
 * if it contains IP addresses other than those of the local host.
 * This is intended for applications such as printing warning
 * messages for suspect ACLs; it is not intended for making access
 * control decisions.  We make no guarantee that an ACL for which
 * this function returns false is safe.
 */
bool
dns_acl_isinsecure(const dns_acl_t *a) {
	unsigned int i;
	bool insecure;

	RUNTIME_CHECK(isc_once_do(&insecure_prefix_once, initialize_action) ==
		      ISC_R_SUCCESS);

	/*
	 * Walk radix tree to find out if there are any non-negated,
	 * non-loopback prefixes.
	 */
	LOCK(&insecure_prefix_lock);
	insecure_prefix_found = false;
	isc_radix_process(a->iptable->radix, is_insecure);
	insecure = insecure_prefix_found;
	UNLOCK(&insecure_prefix_lock);
	if (insecure) {
		return (true);
	}

	/* Now check non-radix elements */
	for (i = 0; i < a->length; i++) {
		dns_aclelement_t *e = &a->elements[i];

		/* A negated match can never be insecure. */
		if (e->negative) {
			continue;
		}

		switch (e->type) {
		case dns_aclelementtype_keyname:
		case dns_aclelementtype_localhost:
			continue;

		case dns_aclelementtype_nestedacl:
			if (dns_acl_isinsecure(e->nestedacl)) {
				return (true);
			}
			continue;

#if defined(HAVE_GEOIP2)
		case dns_aclelementtype_geoip:
#endif /* if defined(HAVE_GEOIP2) */
		case dns_aclelementtype_localnets:
			return (true);

		default:
			UNREACHABLE();
		}
	}

	/* No insecure elements were found. */
	return (false);
}

/*%
 * Check whether an address/signer is allowed by a given acl/aclenv.
 */
bool
dns_acl_allowed(isc_netaddr_t *addr, const dns_name_t *signer, dns_acl_t *acl,
		dns_aclenv_t *aclenv) {
	int match;
	isc_result_t result;

	if (acl == NULL) {
		return (true);
	}
	result = dns_acl_match(addr, signer, acl, aclenv, &match, NULL);
	if (result == ISC_R_SUCCESS && match > 0) {
		return (true);
	}
	return (false);
}

/*
 * Initialize ACL environment, setting up localhost and localnets ACLs
 */
isc_result_t
dns_aclenv_init(isc_mem_t *mctx, dns_aclenv_t *env) {
	isc_result_t result;

	env->localhost = NULL;
	env->localnets = NULL;
	result = dns_acl_create(mctx, 0, &env->localhost);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_nothing;
	}
	result = dns_acl_create(mctx, 0, &env->localnets);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_localhost;
	}
	env->match_mapped = false;
#if defined(HAVE_GEOIP2)
	env->geoip = NULL;
#endif /* if defined(HAVE_GEOIP2) */
	return (ISC_R_SUCCESS);

cleanup_localhost:
	dns_acl_detach(&env->localhost);
cleanup_nothing:
	return (result);
}

void
dns_aclenv_copy(dns_aclenv_t *t, dns_aclenv_t *s) {
	dns_acl_detach(&t->localhost);
	dns_acl_attach(s->localhost, &t->localhost);
	dns_acl_detach(&t->localnets);
	dns_acl_attach(s->localnets, &t->localnets);
	t->match_mapped = s->match_mapped;
#if defined(HAVE_GEOIP2)
	t->geoip = s->geoip;
#endif /* if defined(HAVE_GEOIP2) */
}

void
dns_aclenv_destroy(dns_aclenv_t *env) {
	if (env->localhost != NULL) {
		dns_acl_detach(&env->localhost);
	}
	if (env->localnets != NULL) {
		dns_acl_detach(&env->localnets);
	}
}
