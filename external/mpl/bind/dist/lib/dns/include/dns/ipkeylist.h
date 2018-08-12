/*	$NetBSD: ipkeylist.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#ifndef DNS_IPKEYLIST_H
#define DNS_IPKEYLIST_H 1

#include <isc/types.h>
#include <dns/types.h>

/*%
 * A structure holding a list of addresses, dscps and keys.  Used to
 * store masters for a slave zone, created by parsing config options.
 */
struct dns_ipkeylist {
	isc_sockaddr_t		*addrs;
	isc_dscp_t		*dscps;
	dns_name_t		**keys;
	dns_name_t		**labels;
	isc_uint32_t		count;
	isc_uint32_t		allocated;
};

void
dns_ipkeylist_init(dns_ipkeylist_t *ipkl);
/*%<
 * Reset ipkl to empty state
 *
 * Requires:
 *\li	'ipkl' to be non NULL.
 */

void
dns_ipkeylist_clear(isc_mem_t *mctx, dns_ipkeylist_t *ipkl);
/*%<
 * Free `ipkl` contents using `mctx`.
 *
 * After this call, `ipkl` is a freshly cleared structure with all
 * pointers set to `NULL` and count set to 0.
 *
 * Requires:
 *\li	'mctx' to be a valid memory context.
 *\li	'ipkl' to be non NULL.
 */

isc_result_t
dns_ipkeylist_copy(isc_mem_t *mctx, const dns_ipkeylist_t *src,
		   dns_ipkeylist_t *dst);
/*%<
 * Deep copy `src` into empty `dst`, allocating `dst`'s contents.
 *
 * Requires:
 *\li	'mctx' to be a valid memory context.
 *\li	'src' to be non NULL
 *\li	'dst' to be non NULL and point to an empty \ref dns_ipkeylist_t
 *       with all pointers set to `NULL` and count set to 0.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS	-- success
 *\li	any other value -- failure
 */
isc_result_t
dns_ipkeylist_resize(isc_mem_t *mctx, dns_ipkeylist_t *ipkl, unsigned int n);
/*%<
 * Resize ipkl to contain n elements. Size (count) is not changed, and the
 * added space is zeroed.
 *
 * Requires:
 * \li	'mctx' to be a valid memory context.
 * \li	'ipk' to be non NULL
 * \li	'n' >= ipkl->count
 *
 * Returns:
 * \li	#ISC_R_SUCCESS if successs
 * \li	#ISC_R_NOMEMORY if there's no memory, ipkeylist is left untoched
 */

#endif
