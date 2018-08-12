/*	$NetBSD: loc_29.h,v 1.1.1.1 2018/08/12 12:08:17 christos Exp $	*/

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

#ifndef GENERIC_LOC_29_H
#define GENERIC_LOC_29_H 1


/*!
 * \brief Per RFC1876 */

typedef struct dns_rdata_loc_0 {
	isc_uint8_t	version;	/* must be first and zero */
	isc_uint8_t	size;
	isc_uint8_t	horizontal;
	isc_uint8_t	vertical;
	isc_uint32_t	latitude;
	isc_uint32_t	longitude;
	isc_uint32_t	altitude;
} dns_rdata_loc_0_t;

typedef struct dns_rdata_loc {
	dns_rdatacommon_t	common;
	union {
		dns_rdata_loc_0_t v0;
	} v;
} dns_rdata_loc_t;

#endif /* GENERIC_LOC_29_H */
