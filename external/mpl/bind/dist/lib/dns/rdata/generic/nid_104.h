/*	$NetBSD: nid_104.h,v 1.2.2.3 2019/01/18 08:49:55 pgoyette Exp $	*/

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
#ifndef GENERIC_NID_104_H
#define GENERIC_NID_104_H 1

typedef struct dns_rdata_nid {
	dns_rdatacommon_t	common;
	uint16_t		pref;
	unsigned char		nid[8];
} dns_rdata_nid_t;

#endif /* GENERIC_NID_104_H */
