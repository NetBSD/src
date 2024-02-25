/*	$NetBSD: zonemd_63.h,v 1.5.2.1 2024/02/25 15:47:06 martin Exp $	*/

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

/* Known digest type(s). */
#define DNS_ZONEMD_DIGEST_SHA384 (1)
#define DNS_ZONEMD_DIGEST_SHA512 (2)

/*
 *  \brief per RFC 8976
 */
typedef struct dns_rdata_zonemd {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint32_t serial;
	uint8_t scheme;
	uint8_t digest_type;
	unsigned char *digest;
	uint16_t length;
} dns_rdata_zonemd_t;
