/*	$NetBSD: isdn_20.h,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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

#ifndef GENERIC_ISDN_20_H
#define GENERIC_ISDN_20_H 1


/*!
 * \brief Per RFC1183 */

typedef struct dns_rdata_isdn {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	char			*isdn;
	char			*subaddress;
	isc_uint8_t		isdn_len;
	isc_uint8_t		subaddress_len;
} dns_rdata_isdn_t;

#endif /* GENERIC_ISDN_20_H */
