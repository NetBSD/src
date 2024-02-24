/*	$NetBSD: netscope.c,v 1.1.2.2 2024/02/24 13:07:20 martin Exp $	*/

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
#include <stdlib.h>

#include <isc/net.h>
#include <isc/netscope.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

isc_result_t
isc_netscope_pton(int af, char *scopename, void *addr, uint32_t *zoneid) {
	char *ep;
#ifdef HAVE_IF_NAMETOINDEX
	unsigned int ifid;
	struct in6_addr *in6;
#endif /* ifdef HAVE_IF_NAMETOINDEX */
	uint32_t zone = 0;
	uint64_t llz;

#ifndef HAVE_IF_NAMETOINDEX
	UNUSED(addr);
#endif

	/* at this moment, we only support AF_INET6 */
	if (af != AF_INET6) {
		return (ISC_R_FAILURE);
	}

	/*
	 * Basically, "names" are more stable than numeric IDs in terms
	 * of renumbering, and are more preferred.  However, since there
	 * is no standard naming convention and APIs to deal with the
	 * names.  Thus, we only handle the case of link-local
	 * addresses, for which we use interface names as link names,
	 * assuming one to one mapping between interfaces and links.
	 */
#ifdef HAVE_IF_NAMETOINDEX
	in6 = (struct in6_addr *)addr;
	if (IN6_IS_ADDR_LINKLOCAL(in6) &&
	    (ifid = if_nametoindex((const char *)scopename)) != 0)
	{
		zone = (uint32_t)ifid;
	} else {
#endif /* ifdef HAVE_IF_NAMETOINDEX */
		llz = strtoull(scopename, &ep, 10);
		if (ep == scopename) {
			return (ISC_R_FAILURE);
		}

		/* check overflow */
		zone = (uint32_t)(llz & 0xffffffffUL);
		if (zone != llz) {
			return (ISC_R_FAILURE);
		}
#ifdef HAVE_IF_NAMETOINDEX
	}
#endif /* ifdef HAVE_IF_NAMETOINDEX */

	*zoneid = zone;
	return (ISC_R_SUCCESS);
}
