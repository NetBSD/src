/*	$NetBSD: a_1.h,v 1.1.1.4 2022/09/23 12:09:20 christos Exp $	*/

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
#ifndef HS_4_A_1_H
#define HS_4_A_1_H 1

typedef struct dns_rdata_hs_a {
	dns_rdatacommon_t common;
	struct in_addr in_addr;
} dns_rdata_hs_a_t;

#endif /* HS_4_A_1_H */
