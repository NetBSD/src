/*	$NetBSD: sshfp_44.h,v 1.1.2.2 2024/02/24 13:07:14 martin Exp $	*/

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

/*!
 *  \brief Per RFC 4255 */

#ifndef GENERIC_SSHFP_44_H
#define GENERIC_SSHFP_44_H 1

typedef struct dns_rdata_sshfp {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint8_t algorithm;
	uint8_t digest_type;
	uint16_t length;
	unsigned char *digest;
} dns_rdata_sshfp_t;

#endif /* GENERIC_SSHFP_44_H */
