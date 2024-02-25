/*	$NetBSD: amtrelay_260.h,v 1.4.2.1 2024/02/25 15:47:01 martin Exp $	*/

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

typedef struct dns_rdata_amtrelay {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint8_t precedence;
	bool discovery;
	uint8_t gateway_type;
	struct in_addr in_addr;	  /* gateway type 1 */
	struct in6_addr in6_addr; /* gateway type 2 */
	dns_name_t gateway;	  /* gateway type 3 */
	unsigned char *data;	  /* gateway type > 3 */
	uint16_t length;
} dns_rdata_amtrelay_t;
