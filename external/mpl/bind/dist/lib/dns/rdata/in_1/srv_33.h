/*	$NetBSD: srv_33.h,v 1.3.2.2 2019/06/10 22:04:39 christos Exp $	*/

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

#ifndef IN_1_SRV_33_H
#define IN_1_SRV_33_H 1

/*!
 *  \brief Per RFC2782 */

typedef struct dns_rdata_in_srv {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	uint16_t		priority;
	uint16_t		weight;
	uint16_t		port;
	dns_name_t		target;
} dns_rdata_in_srv_t;

#endif /* IN_1_SRV_33_H */
