/*	$NetBSD: loc_29.h,v 1.3.2.2 2019/06/10 22:04:38 christos Exp $	*/

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
	uint8_t	version;	/* must be first and zero */
	uint8_t	size;
	uint8_t	horizontal;
	uint8_t	vertical;
	uint32_t	latitude;
	uint32_t	longitude;
	uint32_t	altitude;
} dns_rdata_loc_0_t;

typedef struct dns_rdata_loc {
	dns_rdatacommon_t	common;
	union {
		dns_rdata_loc_0_t v0;
	} v;
} dns_rdata_loc_t;

#endif /* GENERIC_LOC_29_H */
