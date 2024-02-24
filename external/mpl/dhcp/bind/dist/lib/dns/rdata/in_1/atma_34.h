/*	$NetBSD: atma_34.h,v 1.1.2.2 2024/02/24 13:07:16 martin Exp $	*/

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

#ifndef IN_1_ATMA_22_H
#define IN_1_ATMA_22_H 1

/*!
 *  \brief Per RFC1706 */

typedef struct dns_rdata_in_atma {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	unsigned char format;
	unsigned char *atma;
	uint16_t atma_len;
} dns_rdata_in_atma_t;

#endif /* IN_1_ATMA_22_H */
