/*	$NetBSD: soa_6.h,v 1.6.2.1 2024/02/25 15:47:05 martin Exp $	*/

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

/* */
#pragma once

typedef struct dns_rdata_soa {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t origin;
	dns_name_t contact;
	uint32_t serial;  /*%< host order */
	uint32_t refresh; /*%< host order */
	uint32_t retry;	  /*%< host order */
	uint32_t expire;  /*%< host order */
	uint32_t minimum; /*%< host order */
} dns_rdata_soa_t;
