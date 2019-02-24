/*	$NetBSD: zonemd_63.h,v 1.1.1.1 2019/02/24 18:56:52 christos Exp $	*/

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

#ifndef GENERIC_ZONEMD_63_H
#define GENERIC_ZONEMD_63_H 1

/* Digest type(s). Currently only SHA-384 is defined. */
#define DNS_ZONEMD_DIGEST_SHA384 (1)

/*
 *  \brief per draft-wessels-zone-digest-05
 */
typedef struct dns_rdata_zonemd {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	uint32_t		serial;
	uint8_t			digest_type;
	uint8_t			reserved;
	unsigned char		*digest;
	uint16_t		length;
} dns_rdata_zonemd_t;

#endif /* GENERIC_ZONEMD_63_H */
