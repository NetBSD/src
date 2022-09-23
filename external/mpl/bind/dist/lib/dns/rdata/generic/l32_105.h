/*	$NetBSD: l32_105.h,v 1.6 2022/09/23 12:15:31 christos Exp $	*/

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
#ifndef GENERIC_L32_105_H
#define GENERIC_L32_105_H 1

typedef struct dns_rdata_l32 {
	dns_rdatacommon_t common;
	uint16_t pref;
	struct in_addr l32;
} dns_rdata_l32_t;

#endif /* GENERIC_L32_105_H */
