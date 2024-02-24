/*	$NetBSD: rrsig_46.h,v 1.1.2.2 2024/02/24 13:07:14 martin Exp $	*/

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

#ifndef GENERIC_DNSSIG_46_H
#define GENERIC_DNSSIG_46_H 1

/*!
 *  \brief Per RFC2535 */
typedef struct dns_rdata_rrsig {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_rdatatype_t covered;
	dns_secalg_t algorithm;
	uint8_t labels;
	uint32_t originalttl;
	uint32_t timeexpire;
	uint32_t timesigned;
	uint16_t keyid;
	dns_name_t signer;
	uint16_t siglen;
	unsigned char *signature;
} dns_rdata_rrsig_t;

#endif /* GENERIC_DNSSIG_46_H */
