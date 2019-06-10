/*	$NetBSD: gpos_27.h,v 1.3.2.2 2019/06/10 22:04:38 christos Exp $	*/

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

#ifndef GENERIC_GPOS_27_H
#define GENERIC_GPOS_27_H 1


/*!
 *  \brief per RFC1712 */

typedef struct dns_rdata_gpos {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	char			*longitude;
	char			*latitude;
	char			*altitude;
	uint8_t		long_len;
	uint8_t		lat_len;
	uint8_t		alt_len;
} dns_rdata_gpos_t;

#endif /* GENERIC_GPOS_27_H */
