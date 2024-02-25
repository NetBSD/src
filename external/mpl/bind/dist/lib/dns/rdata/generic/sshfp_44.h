/*	$NetBSD: sshfp_44.h,v 1.6.2.1 2024/02/25 15:47:05 martin Exp $	*/

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

#pragma once

typedef struct dns_rdata_sshfp {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint8_t algorithm;
	uint8_t digest_type;
	uint16_t length;
	unsigned char *digest;
} dns_rdata_sshfp_t;
