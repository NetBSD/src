/*	$NetBSD: ipseckey_45.h,v 1.1.1.1 2018/08/12 12:08:17 christos Exp $	*/

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


#ifndef GENERIC_IPSECKEY_45_H
#define GENERIC_IPSECKEY_45_H 1

typedef struct dns_rdata_ipseckey {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint8_t		precedence;
	isc_uint8_t		gateway_type;
	isc_uint8_t		algorithm;
	struct in_addr		in_addr;	/* gateway type 1 */
	struct in6_addr		in6_addr;	/* gateway type 2 */
	dns_name_t		gateway;	/* gateway type 3 */
	unsigned char		*key;
	isc_uint16_t		keylength;
} dns_rdata_ipseckey_t;

#endif /* GENERIC_IPSECKEY_45_H */
