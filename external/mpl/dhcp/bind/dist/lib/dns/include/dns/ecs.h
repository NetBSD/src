/*	$NetBSD: ecs.h,v 1.1.2.2 2024/02/24 13:07:04 martin Exp $	*/

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

#ifndef DNS_ECS_H
#define DNS_ECS_H 1

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/netaddr.h>
#include <isc/types.h>

#include <dns/rdatatype.h>
#include <dns/types.h>

/*%
 * Maximum scope values for IPv4 and IPv6.
 */
#ifndef ECS_MAX_V4_SCOPE
#define ECS_MAX_V4_SCOPE 24
#endif

#ifndef ECS_MAX_V6_SCOPE
#define ECS_MAX_V6_SCOPE 56
#endif

/*
 * Any updates to this structure should also be applied in
 * contrib/modules/dlz/dlz_minmal.h.
 */
struct dns_ecs {
	isc_netaddr_t addr;
	uint8_t	      source;
	uint8_t	      scope;
};

/* <address>/NNN/NNN */
#define DNS_ECS_FORMATSIZE (ISC_NETADDR_FORMATSIZE + 9)

ISC_LANG_BEGINDECLS

void
dns_ecs_init(dns_ecs_t *ecs);
/*%<
 * Initialize a DNS ECS structure.
 *
 * Requires:
 * \li 'ecs' is not NULL and points to a valid dns_ecs structure.
 */

bool
dns_ecs_equals(const dns_ecs_t *ecs1, const dns_ecs_t *ecs2);
/*%<
 * Determine whether two ECS address prefixes are equal (except the
 * scope prefix-length field).
 *
 * 'ecs1->source' must exactly match 'ecs2->source'; the address families
 * must match; and the first 'ecs1->source' bits of the addresses must
 * match. Subsequent address bits and the 'scope' values are ignored.
 */

void
dns_ecs_format(const dns_ecs_t *ecs, char *buf, size_t size);
/*%<
 * Format an ECS record as text. Result is guaranteed to be null-terminated.
 *
 * Requires:
 * \li  'ecs' is not NULL.
 * \li  'buf' is not NULL.
 * \li  'size' is at least DNS_ECS_FORMATSIZE
 */
ISC_LANG_ENDDECLS

#endif /* DNS_ECS_H */
