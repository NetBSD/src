/*	$NetBSD: ipkeylist.h,v 1.6.2.1 2024/02/25 15:46:56 martin Exp $	*/

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

#include <inttypes.h>

#include <isc/types.h>

#include <dns/types.h>

/*%
 * A structure holding a list of addresses and keys.  Used to store
 * primaries for a secondary zone, created by parsing config options.
 */
struct dns_ipkeylist {
	isc_sockaddr_t *addrs;
	dns_name_t    **keys;
	dns_name_t    **tlss;
	dns_name_t    **labels;
	uint32_t	count;
	uint32_t	allocated;
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
 * \li	#ISC_R_SUCCESS if success
 * \li	#ISC_R_NOMEMORY if there's no memory, ipkeylist is left untouched
 */
