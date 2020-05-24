/*	$NetBSD: afsdb_18.h,v 1.4 2020/05/24 19:46:24 christos Exp $	*/

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

#ifndef GENERIC_AFSDB_18_H
#define GENERIC_AFSDB_18_H 1

/*!
 *  \brief Per RFC1183 */

typedef struct dns_rdata_afsdb {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t subtype;
	dns_name_t server;
} dns_rdata_afsdb_t;

#endif /* GENERIC_AFSDB_18_H */
