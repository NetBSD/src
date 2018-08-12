/*	$NetBSD: rp_17.h,v 1.1.1.1 2018/08/12 12:08:17 christos Exp $	*/

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

#ifndef GENERIC_RP_17_H
#define GENERIC_RP_17_H 1


/*!
 *  \brief Per RFC1183 */

typedef struct dns_rdata_rp {
	dns_rdatacommon_t       common;
	isc_mem_t               *mctx;
	dns_name_t              mail;
	dns_name_t              text;
} dns_rdata_rp_t;


#endif /* GENERIC_RP_17_H */
