/*	$NetBSD: ecs.c,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#include <string.h>

#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/util.h>

#include <dns/ecs.h>
#include <dns/types.h>

void
dns_ecs_init(dns_ecs_t *ecs) {
	isc_netaddr_unspec(&ecs->addr);
	ecs->source = 0;
	/*
	 * XXXMUKS: Fix me when resolver ECS gets merged where scope
	 * gets initialized to 0xff.
	 */
	ecs->scope = 0;
}

void
dns_ecs_format(dns_ecs_t *ecs, char *buf, size_t size) {
	size_t len;

	REQUIRE(ecs != NULL);
	REQUIRE(buf != NULL);
	REQUIRE(size >= DNS_ECS_FORMATSIZE);

	isc_netaddr_format(&ecs->addr, buf, (unsigned int)size);
	len = strlen(buf);
	INSIST(size >= len);
	buf += len;
	size -= len;
	snprintf(buf, size, "/%u/%u", ecs->source, ecs->scope);
}
