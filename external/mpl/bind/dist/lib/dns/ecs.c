/*	$NetBSD: ecs.c,v 1.6 2023/01/25 21:43:30 christos Exp $	*/

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

#include <string.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/util.h>

#include <dns/ecs.h>
#include <dns/nsec.h>
#include <dns/rbt.h>
#include <dns/rdata.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/types.h>

void
dns_ecs_init(dns_ecs_t *ecs) {
	isc_netaddr_unspec(&ecs->addr);
	ecs->source = 0;
	ecs->scope = 0xff;
}

bool
dns_ecs_equals(const dns_ecs_t *ecs1, const dns_ecs_t *ecs2) {
	const unsigned char *addr1, *addr2;
	uint8_t mask;
	size_t alen;

	REQUIRE(ecs1 != NULL && ecs2 != NULL);

	if (ecs1->source != ecs2->source ||
	    ecs1->addr.family != ecs2->addr.family)
	{
		return (false);
	}

	alen = (ecs1->source + 7) / 8;
	if (alen == 0) {
		return (true);
	}

	switch (ecs1->addr.family) {
	case AF_INET:
		INSIST(alen <= 4);
		addr1 = (const unsigned char *)&ecs1->addr.type.in;
		addr2 = (const unsigned char *)&ecs2->addr.type.in;
		break;
	case AF_INET6:
		INSIST(alen <= 16);
		addr1 = (const unsigned char *)&ecs1->addr.type.in6;
		addr2 = (const unsigned char *)&ecs2->addr.type.in6;
		break;
	default:
		UNREACHABLE();
	}

	/*
	 * Compare all octets except the final octet of the address
	 * prefix.
	 */
	if (alen > 1 && memcmp(addr1, addr2, alen - 1) != 0) {
		return (false);
	}

	/*
	 * It should not be necessary to mask the final octet; all
	 * bits past the source prefix length are supposed to be 0.
	 * However, it seems prudent not to omit them from the
	 * comparison anyway.
	 */
	mask = (~0U << (8 - (ecs1->source % 8))) & 0xff;
	if (mask == 0) {
		mask = 0xff;
	}

	if ((addr1[alen - 1] & mask) != (addr2[alen - 1] & mask)) {
		return (false);
	}

	return (true);
}

void
dns_ecs_format(const dns_ecs_t *ecs, char *buf, size_t size) {
	size_t len;
	char *p;

	REQUIRE(ecs != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(size >= DNS_ECS_FORMATSIZE);

	isc_netaddr_format(&ecs->addr, buf, size);
	len = strlen(buf);
	p = buf + len;
	snprintf(p, size - len, "/%d/%d", ecs->source,
		 ecs->scope == 0xff ? 0 : ecs->scope);
}
