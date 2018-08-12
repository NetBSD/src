/*	$NetBSD: eui64_109.h,v 1.1.1.1 2018/08/12 12:08:17 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/* */
#ifndef GENERIC_EUI64_109_H
#define GENERIC_EUI64_109_H 1

typedef struct dns_rdata_eui64 {
	dns_rdatacommon_t	common;
	unsigned char		eui64[8];
} dns_rdata_eui64_t;

#endif /* GENERIC_EUI64_10k_H */
