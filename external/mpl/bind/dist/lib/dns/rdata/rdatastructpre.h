/*	$NetBSD: rdatastructpre.h,v 1.6.2.1 2024/02/25 15:47:00 martin Exp $	*/

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

#include <isc/lang.h>
#include <isc/sockaddr.h>

#include <dns/name.h>
#include <dns/types.h>

ISC_LANG_BEGINDECLS

typedef struct dns_rdatacommon {
	dns_rdataclass_t rdclass;
	dns_rdatatype_t rdtype;
	ISC_LINK(struct dns_rdatacommon) link;
} dns_rdatacommon_t;

#define DNS_RDATACOMMON_INIT(_data, _rdtype, _rdclass) \
	do {                                           \
		(_data)->common.rdtype = (_rdtype);    \
		(_data)->common.rdclass = (_rdclass);  \
		ISC_LINK_INIT(&(_data)->common, link); \
	} while (0)
