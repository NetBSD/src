/*	$NetBSD: ipseckey_45.h,v 1.6.2.1 2024/02/25 15:47:02 martin Exp $	*/

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

typedef struct dns_rdata_ipseckey {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint8_t precedence;
	uint8_t gateway_type;
	uint8_t algorithm;
	struct in_addr in_addr;	  /* gateway type 1 */
	struct in6_addr in6_addr; /* gateway type 2 */
	dns_name_t gateway;	  /* gateway type 3 */
	unsigned char *key;
	uint16_t keylength;
} dns_rdata_ipseckey_t;
