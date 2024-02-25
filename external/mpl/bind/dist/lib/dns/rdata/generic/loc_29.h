/*	$NetBSD: loc_29.h,v 1.6.2.1 2024/02/25 15:47:03 martin Exp $	*/

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

/*!
 * \brief Per RFC1876 */

typedef struct dns_rdata_loc_0 {
	uint8_t version; /* must be first and zero */
	uint8_t size;
	uint8_t horizontal;
	uint8_t vertical;
	uint32_t latitude;
	uint32_t longitude;
	uint32_t altitude;
} dns_rdata_loc_0_t;

typedef struct dns_rdata_loc {
	dns_rdatacommon_t common;
	union {
		dns_rdata_loc_0_t v0;
	} v;
} dns_rdata_loc_t;
