/*	$NetBSD: tlsa_52.h,v 1.1.2.2 2024/02/24 13:07:15 martin Exp $	*/

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

#ifndef GENERIC_TLSA_52_H
#define GENERIC_TLSA_52_H 1

/*!
 *  \brief per rfc6698.txt
 */
typedef struct dns_rdata_tlsa {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint8_t usage;
	uint8_t selector;
	uint8_t match;
	uint16_t length;
	unsigned char *data;
} dns_rdata_tlsa_t;

#endif /* GENERIC_TLSA_52_H */
