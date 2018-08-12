/*	$NetBSD: mx_15.h,v 1.1.1.1 2018/08/12 12:08:17 christos Exp $	*/

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
#ifndef GENERIC_MX_15_H
#define GENERIC_MX_15_H 1


typedef struct dns_rdata_mx {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint16_t		pref;
	dns_name_t		mx;
} dns_rdata_mx_t;

#endif /* GENERIC_MX_15_H */
