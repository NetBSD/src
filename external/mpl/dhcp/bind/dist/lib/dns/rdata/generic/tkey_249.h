/*	$NetBSD: tkey_249.h,v 1.1.2.2 2024/02/24 13:07:15 martin Exp $	*/

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

#ifndef GENERIC_TKEY_249_H
#define GENERIC_TKEY_249_H 1

/*!
 *  \brief Per draft-ietf-dnsind-tkey-00.txt */

typedef struct dns_rdata_tkey {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t algorithm;
	uint32_t inception;
	uint32_t expire;
	uint16_t mode;
	uint16_t error;
	uint16_t keylen;
	unsigned char *key;
	uint16_t otherlen;
	unsigned char *other;
} dns_rdata_tkey_t;

#endif /* GENERIC_TKEY_249_H */
