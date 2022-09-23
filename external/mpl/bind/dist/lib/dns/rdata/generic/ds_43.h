/*	$NetBSD: ds_43.h,v 1.7 2022/09/23 12:15:31 christos Exp $	*/

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

#ifndef GENERIC_DS_43_H
#define GENERIC_DS_43_H 1

/*!
 *  \brief per draft-ietf-dnsext-delegation-signer-05.txt */
typedef struct dns_rdata_ds {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t key_tag;
	dns_secalg_t algorithm;
	dns_dsdigest_t digest_type;
	uint16_t length;
	unsigned char *digest;
} dns_rdata_ds_t;

#endif /* GENERIC_DS_43_H */
