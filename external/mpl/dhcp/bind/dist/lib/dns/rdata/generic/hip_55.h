/*	$NetBSD: hip_55.h,v 1.1.2.2 2024/02/24 13:07:11 martin Exp $	*/

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

#ifndef GENERIC_HIP_5_H
#define GENERIC_HIP_5_H 1

/* RFC 5205 */

typedef struct dns_rdata_hip {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	unsigned char *hit;
	unsigned char *key;
	unsigned char *servers;
	uint8_t algorithm;
	uint8_t hit_len;
	uint16_t key_len;
	uint16_t servers_len;
	/* Private */
	uint16_t offset;
} dns_rdata_hip_t;

isc_result_t
dns_rdata_hip_first(dns_rdata_hip_t *);

isc_result_t
dns_rdata_hip_next(dns_rdata_hip_t *);

void
dns_rdata_hip_current(dns_rdata_hip_t *, dns_name_t *);

#endif /* GENERIC_HIP_5_H */
