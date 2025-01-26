/*	$NetBSD: sortlist.h,v 1.7 2025/01/26 16:25:46 christos Exp $	*/

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

/*! \file */

#include <isc/types.h>

#include <dns/acl.h>
#include <dns/types.h>

/*%
 * Type for callback functions that rank addresses.
 */
typedef int (*dns_addressorderfunc_t)(const isc_netaddr_t *address,
				      const void	  *arg);

/*%
 * Return value type for setup_sortlist.
 */
typedef enum {
	NS_SORTLISTTYPE_NONE,
	NS_SORTLISTTYPE_1ELEMENT,
	NS_SORTLISTTYPE_2ELEMENT
} ns_sortlisttype_t;

ns_sortlisttype_t
ns_sortlist_setup(dns_acl_t *acl, dns_aclenv_t *env, isc_netaddr_t *clientaddr,
		  void **argp);
/*%<
 * Find the sortlist statement in 'acl' (for ACL environment 'env')
 * that applies to 'clientaddr', if any.
 *
 * If a 1-element sortlist item applies, return NS_SORTLISTTYPE_1ELEMENT and
 * make '*argp' point to the matching subelement.
 *
 * If a 2-element sortlist item applies, return NS_SORTLISTTYPE_2ELEMENT and
 * make '*argp' point to ACL that forms the second element.
 *
 * If no sortlist item applies, return NS_SORTLISTTYPE_NONE and set '*argp'
 * to NULL.
 */

int
ns_sortlist_addrorder1(const isc_netaddr_t *addr, const void *arg);
/*%<
 * Find the sort order of 'addr' in 'arg', the matching element
 * of a 1-element top-level sortlist statement.
 */

int
ns_sortlist_addrorder2(const isc_netaddr_t *addr, const void *arg);
/*%<
 * Find the sort order of 'addr' in 'arg', a topology-like
 * ACL forming the second element in a 2-element top-level
 * sortlist statement.
 */
