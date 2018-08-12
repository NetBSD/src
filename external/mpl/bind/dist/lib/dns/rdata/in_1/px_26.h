/*	$NetBSD: px_26.h,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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

#ifndef IN_1_PX_26_H
#define IN_1_PX_26_H 1


/*!
 *  \brief Per RFC2163 */

typedef struct dns_rdata_in_px {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint16_t		preference;
	dns_name_t		map822;
	dns_name_t		mapx400;
} dns_rdata_in_px_t;

#endif /* IN_1_PX_26_H */
