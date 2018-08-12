/*	$NetBSD: tlsa_52.h,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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


#ifndef GENERIC_TLSA_52_H
#define GENERIC_TLSA_52_H 1

/*!
 *  \brief per rfc6698.txt
 */
typedef struct dns_rdata_tlsa {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint8_t		usage;
	isc_uint8_t		selector;
	isc_uint8_t		match;
	isc_uint16_t		length;
	unsigned char		*data;
} dns_rdata_tlsa_t;

#endif /* GENERIC_TLSA_52_H */
