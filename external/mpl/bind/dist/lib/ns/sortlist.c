/*	$NetBSD: sortlist.c,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

/*! \file */

#include <config.h>

#include <isc/mem.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/message.h>
#include <dns/result.h>

#include <ns/server.h>
#include <ns/sortlist.h>

ns_sortlisttype_t
ns_sortlist_setup(dns_acl_t *acl, dns_aclenv_t *env,
		  isc_netaddr_t *clientaddr, const void **argp)
{
	unsigned int i;

	if (acl == NULL)
		goto dont_sort;

	for (i = 0; i < acl->length; i++) {
		/*
		 * 'e' refers to the current 'top level statement'
		 * in the sortlist (see ARM).
		 */
		dns_aclelement_t *e = &acl->elements[i];
		dns_aclelement_t *try_elt;
		dns_aclelement_t *order_elt = NULL;
		const dns_aclelement_t *matched_elt = NULL;

		if (e->type == dns_aclelementtype_nestedacl) {
			dns_acl_t *inner = e->nestedacl;

			if (inner->length == 0)
				try_elt = e;
			else if (inner->length > 2)
				goto dont_sort;
			else if (inner->elements[0].negative)
				goto dont_sort;
			else {
				try_elt = &inner->elements[0];
				if (inner->length == 2)
					order_elt = &inner->elements[1];
			}
		} else {
			/*
			 * BIND 8 allows bare elements at the top level
			 * as an undocumented feature.
			 */
			try_elt = e;
		}

		if (dns_aclelement_match(clientaddr, NULL, try_elt, env,
					 &matched_elt))
		{
			if (order_elt != NULL) {
				if (order_elt->type ==
				    dns_aclelementtype_nestedacl)
				{
					*argp = order_elt->nestedacl;
					return (NS_SORTLISTTYPE_2ELEMENT);
				} else if (order_elt->type ==
					   dns_aclelementtype_localhost &&
					   env->localhost != NULL)
				{
					*argp = env->localhost;
					return (NS_SORTLISTTYPE_2ELEMENT);
				} else if (order_elt->type ==
					   dns_aclelementtype_localnets &&
					   env->localnets != NULL)
				{
					*argp = env->localnets;
					return (NS_SORTLISTTYPE_2ELEMENT);
				} else {
					/*
					 * BIND 8 allows a bare IP prefix as
					 * the 2nd element of a 2-element
					 * sortlist statement.
					 */
					*argp = order_elt;
					return (NS_SORTLISTTYPE_1ELEMENT);
				}
			} else {
				INSIST(matched_elt != NULL);
				*argp = matched_elt;
				return (NS_SORTLISTTYPE_1ELEMENT);
			}
		}
	}

	/* No match; don't sort. */
 dont_sort:
	*argp = NULL;
	return (NS_SORTLISTTYPE_NONE);
}

int
ns_sortlist_addrorder2(const isc_netaddr_t *addr, const void *arg) {
	const dns_sortlist_arg_t *sla = (const dns_sortlist_arg_t *) arg;
	const dns_aclenv_t *env = sla->env;
	const dns_acl_t *sortacl = sla->acl;
	int match;

	(void)dns_acl_match(addr, NULL, sortacl, env, &match, NULL);
	if (match > 0)
		return (match);
	else if (match < 0)
		return (INT_MAX - (-match));
	else
		return (INT_MAX / 2);
}

int
ns_sortlist_addrorder1(const isc_netaddr_t *addr, const void *arg) {
	const dns_sortlist_arg_t *sla = (const dns_sortlist_arg_t *) arg;
	const dns_aclenv_t *env = sla->env;
	const dns_aclelement_t *element = sla->element;

	if (dns_aclelement_match(addr, NULL, element, env, NULL)) {
		return (0);
	}

	return (INT_MAX);
}

void
ns_sortlist_byaddrsetup(dns_acl_t *sortlist_acl, dns_aclenv_t *env,
			isc_netaddr_t *client_addr,
			dns_addressorderfunc_t *orderp, const void **argp)
{
	ns_sortlisttype_t sortlisttype;

	sortlisttype = ns_sortlist_setup(sortlist_acl, env, client_addr, argp);

	switch (sortlisttype) {
	case NS_SORTLISTTYPE_1ELEMENT:
		*orderp = ns_sortlist_addrorder1;
		break;
	case NS_SORTLISTTYPE_2ELEMENT:
		*orderp = ns_sortlist_addrorder2;
		break;
	case NS_SORTLISTTYPE_NONE:
		*orderp = NULL;
		break;
	default:
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "unexpected return from ns_sortlist_setup(): "
				 "%d", sortlisttype);
		break;
	}
}

