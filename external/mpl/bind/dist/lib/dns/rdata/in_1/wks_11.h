/*	$NetBSD: wks_11.h,v 1.6 2022/09/23 12:15:31 christos Exp $	*/

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

#ifndef IN_1_WKS_11_H
#define IN_1_WKS_11_H 1

typedef struct dns_rdata_in_wks {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	struct in_addr in_addr;
	uint16_t protocol;
	unsigned char *map;
	uint16_t map_len;
} dns_rdata_in_wks_t;

#endif /* IN_1_WKS_11_H */
