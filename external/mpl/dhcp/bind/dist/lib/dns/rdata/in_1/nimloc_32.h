/*	$NetBSD: nimloc_32.h,v 1.1.2.2 2024/02/24 13:07:17 martin Exp $	*/

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

#ifndef IN_1_NIMLOC_32_H
#define IN_1_NIMLOC_32_H 1

/*!
 *  \brief http://ana-3.lcs.mit.edu/~jnc/nimrod/dns.txt
 */

typedef struct dns_rdata_in_nimloc {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	unsigned char *nimloc;
	uint16_t nimloc_len;
} dns_rdata_in_nimloc_t;

#endif /* IN_1_NIMLOC_32_H */
