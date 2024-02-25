/*	$NetBSD: wks_11.h,v 1.6.2.1 2024/02/25 15:47:07 martin Exp $	*/

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

typedef struct dns_rdata_in_wks {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	struct in_addr in_addr;
	uint16_t protocol;
	unsigned char *map;
	uint16_t map_len;
} dns_rdata_in_wks_t;
