/*	$NetBSD: keydata_65533.h,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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

#ifndef GENERIC_KEYDATA_65533_H
#define GENERIC_KEYDATA_65533_H 1


typedef struct dns_rdata_keydata {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	isc_uint32_t		refresh;      /* Timer for refreshing data */
	isc_uint32_t		addhd;	      /* Hold-down timer for adding */
	isc_uint32_t		removehd;     /* Hold-down timer for removing */
	isc_uint16_t		flags;	      /* Copy of DNSKEY_48 */
	isc_uint8_t		protocol;
	isc_uint8_t		algorithm;
	isc_uint16_t		datalen;
	unsigned char *		data;
} dns_rdata_keydata_t;

#endif /* GENERIC_KEYDATA_65533_H */
