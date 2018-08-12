/*	$NetBSD: a_1.h,v 1.1.1.1 2018/08/12 12:08:18 christos Exp $	*/

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


/* by Bjorn.Victor@it.uu.se, 2005-05-07 */
/* Based on generic/mx_15.h */

#ifndef CH_3_A_1_H
#define CH_3_A_1_H 1

typedef isc_uint16_t ch_addr_t;

typedef struct dns_rdata_ch_a {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	dns_name_t		ch_addr_dom; /* ch-addr domain for back mapping */
	ch_addr_t		ch_addr; /* chaos address (16 bit) network order */
} dns_rdata_ch_a_t;

#endif /* CH_3_A_1_H */
