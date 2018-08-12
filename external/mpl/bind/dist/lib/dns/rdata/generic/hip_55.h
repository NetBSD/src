/*	$NetBSD: hip_55.h,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


#ifndef GENERIC_HIP_5_H
#define GENERIC_HIP_5_H 1

/* RFC 5205 */

typedef struct dns_rdata_hip {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	unsigned char *		hit;
	unsigned char *		key;
	unsigned char *		servers;
	isc_uint8_t		algorithm;
	isc_uint8_t		hit_len;
	isc_uint16_t		key_len;
	isc_uint16_t		servers_len;
	/* Private */
	isc_uint16_t		offset;
} dns_rdata_hip_t;

isc_result_t
dns_rdata_hip_first(dns_rdata_hip_t *);

isc_result_t
dns_rdata_hip_next(dns_rdata_hip_t *);

void
dns_rdata_hip_current(dns_rdata_hip_t *, dns_name_t *);

#endif /* GENERIC_HIP_5_H */
