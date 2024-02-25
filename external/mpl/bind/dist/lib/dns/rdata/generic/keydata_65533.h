/*	$NetBSD: keydata_65533.h,v 1.7.2.1 2024/02/25 15:47:02 martin Exp $	*/

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

typedef struct dns_rdata_keydata {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint32_t refresh;  /* Timer for refreshing data */
	uint32_t addhd;	   /* Hold-down timer for adding */
	uint32_t removehd; /* Hold-down timer for removing */
	uint16_t flags;	   /* Copy of DNSKEY_48 */
	dns_secproto_t protocol;
	dns_secalg_t algorithm;
	uint16_t datalen;
	unsigned char *data;
} dns_rdata_keydata_t;
