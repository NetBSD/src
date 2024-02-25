/*	$NetBSD: a6_38.h,v 1.6.2.1 2024/02/25 15:47:06 martin Exp $	*/

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
 *  \brief Per RFC2874 */

typedef struct dns_rdata_in_a6 {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t prefix;
	uint8_t prefixlen;
	struct in6_addr in6_addr;
} dns_rdata_in_a6_t;
