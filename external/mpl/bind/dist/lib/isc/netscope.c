/*	$NetBSD: netscope.c,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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

#include <isc/string.h>
#include <isc/net.h>
#include <isc/netscope.h>
#include <isc/result.h>

isc_result_t
isc_netscope_pton(int af, char *scopename, void *addr, isc_uint32_t *zoneid) {
	char *ep;
#ifdef ISC_PLATFORM_HAVEIFNAMETOINDEX
	unsigned int ifid;
	struct in6_addr *in6;
#endif
	isc_uint32_t zone;
	isc_uint64_t llz;

	/* at this moment, we only support AF_INET6 */
	if (af != AF_INET6)
		return (ISC_R_FAILURE);

	/*
	 * Basically, "names" are more stable than numeric IDs in terms of
	 * renumbering, and are more preferred.  However, since there is no
	 * standard naming convention and APIs to deal with the names.  Thus,
	 * we only handle the case of link-local addresses, for which we use
	 * interface names as link names, assuming one to one mapping between
	 * interfaces and links.
	 */
#ifdef ISC_PLATFORM_HAVEIFNAMETOINDEX
	in6 = (struct in6_addr *)addr;
	if (IN6_IS_ADDR_LINKLOCAL(in6) &&
	    (ifid = if_nametoindex((const char *)scopename)) != 0)
		zone = (isc_uint32_t)ifid;
	else {
#endif
		llz = isc_string_touint64(scopename, &ep, 10);
		if (ep == scopename)
			return (ISC_R_FAILURE);

		/* check overflow */
		zone = (isc_uint32_t)(llz & 0xffffffffUL);
		if (zone != llz)
			return (ISC_R_FAILURE);
#ifdef ISC_PLATFORM_HAVEIFNAMETOINDEX
	}
#endif

	*zoneid = zone;
	return (ISC_R_SUCCESS);
}
