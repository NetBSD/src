/*	$NetBSD: sortlist.c,v 1.9 2025/01/26 16:25:46 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/message.h>

#include <ns/server.h>
#include <ns/sortlist.h>

ns_sortlisttype_t
ns_sortlist_setup(dns_acl_t *acl, dns_aclenv_t *env, isc_netaddr_t *clientaddr,
		  void **argp) {
	if (acl == NULL) {
		goto dont_sort;
	}

	for (size_t i = 0; i < acl->length; i++) {
		/*
		 * 'e' refers to the current 'top level statement'
		 * in the sortlist (see ARM).
		 */
		dns_aclelement_t *e = &acl->elements[i];
		dns_aclelement_t *try_elt;
		dns_aclelement_t *order_elt = NULL;
		dns_aclelement_t *matched_elt = NULL;

		if (e->type == dns_aclelementtype_nestedacl) {
			dns_acl_t *inner = e->nestedacl;

			if (inner->length == 0) {
				try_elt = e;
			} else if (inner->length > 2) {
				goto dont_sort;
			} else if (inner->elements[0].negative) {
				goto dont_sort;
			} else {
				try_elt = &inner->elements[0];
				if (inner->length == 2) {
					order_elt = &inner->elements[1];
				}
			}
		} else {
			/*
			 * BIND 8 allows bare elements at the top level
			 * as an undocumented feature.
			 */
			try_elt = e;
		}

		if (!dns_aclelement_match(
			    clientaddr, NULL, try_elt, env,
			    (const dns_aclelement_t **)&matched_elt))
		{
			continue;
		}

		if (order_elt == NULL) {
			INSIST(matched_elt != NULL);
			*argp = matched_elt;
			return NS_SORTLISTTYPE_1ELEMENT;
		}

		if (order_elt->type == dns_aclelementtype_nestedacl) {
			dns_acl_t *inner = NULL;
			dns_acl_attach(order_elt->nestedacl, &inner);
			*argp = inner;
			return NS_SORTLISTTYPE_2ELEMENT;
		}

		if (order_elt->type == dns_aclelementtype_localhost) {
			rcu_read_lock();
			dns_acl_t *inner = rcu_dereference(env->localhost);
			if (inner != NULL) {
				*argp = dns_acl_ref(inner);
				rcu_read_unlock();
				return NS_SORTLISTTYPE_2ELEMENT;
			}
			rcu_read_unlock();
		}

		if (order_elt->type == dns_aclelementtype_localnets) {
			rcu_read_lock();
			dns_acl_t *inner = rcu_dereference(env->localhost);
			if (inner != NULL) {
				*argp = dns_acl_ref(inner);
				rcu_read_unlock();
				return NS_SORTLISTTYPE_2ELEMENT;
			}
			rcu_read_unlock();
		}

		/*
		 * BIND 8 allows a bare IP prefix as
		 * the 2nd element of a 2-element
		 * sortlist statement.
		 */
		*argp = order_elt;
		return NS_SORTLISTTYPE_1ELEMENT;
	}

dont_sort:
	*argp = NULL;
	return NS_SORTLISTTYPE_NONE;
}

int
ns_sortlist_addrorder2(const isc_netaddr_t *addr, const void *arg) {
	const dns_sortlist_arg_t *sla = (const dns_sortlist_arg_t *)arg;
	dns_aclenv_t *env = sla->env;
	const dns_acl_t *sortacl = sla->acl;
	int match;

	(void)dns_acl_match(addr, NULL, sortacl, env, &match, NULL);
	if (match > 0) {
		return match;
	} else if (match < 0) {
		return INT_MAX - (-match);
	} else {
		return INT_MAX / 2;
	}
}

int
ns_sortlist_addrorder1(const isc_netaddr_t *addr, const void *arg) {
	const dns_sortlist_arg_t *sla = (const dns_sortlist_arg_t *)arg;
	dns_aclenv_t *env = sla->env;
	const dns_aclelement_t *element = sla->element;

	if (dns_aclelement_match(addr, NULL, element, env, NULL)) {
		return 0;
	}

	return INT_MAX;
}
