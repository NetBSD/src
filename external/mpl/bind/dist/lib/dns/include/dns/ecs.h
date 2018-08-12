/*	$NetBSD: ecs.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#ifndef DNS_ECS_H
#define DNS_ECS_H 1

#include <isc/netaddr.h>
#include <isc/types.h>
#include <dns/types.h>

struct dns_ecs {
	isc_netaddr_t addr;
	isc_uint8_t source;
	isc_uint8_t scope;
};

#define DNS_ECS_FORMATSIZE (ISC_NETADDR_FORMATSIZE + 8) /* <address>/NNN/NNN */

ISC_LANG_BEGINDECLS

void
dns_ecs_init(dns_ecs_t *ecs);
/*%<
 * Initialize a DNS ECS structure.
 *
 * Requires:
 * \li 'ecs' is not NULL and points to a valid dns_ecs structure.
 */

void
dns_ecs_format(dns_ecs_t *ecs, char *buf, size_t size);
/*%<
 * Format an ECS record as text. Result is guaranteed to be null-terminated.
 *
 * Requires:
 * \li  'ecs' is not NULL.
 * \li  'buf' is not NULL.
 * \li  'size' is at least DNS_ECS_FORMATSIZE
 */

#endif /* DNS_ECS_H */
