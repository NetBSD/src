/*	$NetBSD: cert_37.h,v 1.1.2.2 2024/02/24 13:07:10 martin Exp $	*/

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

#ifndef GENERIC_CERT_37_H
#define GENERIC_CERT_37_H 1

/*% RFC2538 */
typedef struct dns_rdata_cert {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t type;
	uint16_t key_tag;
	uint8_t algorithm;
	uint16_t length;
	unsigned char *certificate;
} dns_rdata_cert_t;

#endif /* GENERIC_CERT_37_H */
