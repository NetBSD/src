/*	$NetBSD: null_10.h,v 1.2.2.3 2019/01/18 08:49:55 pgoyette Exp $	*/

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

/* */
#ifndef GENERIC_NULL_10_H
#define GENERIC_NULL_10_H 1


typedef struct dns_rdata_null {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	uint16_t		length;
	unsigned char		*data;
} dns_rdata_null_t;


#endif /* GENERIC_NULL_10_H */
