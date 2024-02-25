/*	$NetBSD: a_1.h,v 1.6.2.1 2024/02/25 15:47:00 martin Exp $	*/

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

/* by Bjorn.Victor@it.uu.se, 2005-05-07 */
/* Based on generic/mx_15.h */

#pragma once

typedef uint16_t ch_addr_t;

typedef struct dns_rdata_ch_a {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t ch_addr_dom; /* ch-addr domain for back mapping
				 * */
	ch_addr_t ch_addr;	/* chaos address (16 bit) network
				 * order */
} dns_rdata_ch_a_t;
