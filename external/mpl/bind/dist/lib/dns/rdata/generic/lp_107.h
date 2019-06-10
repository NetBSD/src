/*	$NetBSD: lp_107.h,v 1.3.2.2 2019/06/10 22:04:38 christos Exp $	*/

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
#ifndef GENERIC_LP_107_H
#define GENERIC_LP_107_H 1

typedef struct dns_rdata_lp {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	uint16_t		pref;
	dns_name_t		lp;
} dns_rdata_lp_t;

#endif /* GENERIC_LP_107_H */
