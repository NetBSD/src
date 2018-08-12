/*	$NetBSD: csync_62.h,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

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

#ifndef GENERIC_CSYNC_62_H
#define GENERIC_CSYNC_62_H 1

/*!
 * \brief Per RFC 7477
 */

typedef struct dns_rdata_csync {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint32_t		serial;
	isc_uint16_t		flags;
	unsigned char		*typebits;
	isc_uint16_t		len;
} dns_rdata_csync_t;

#endif /* GENERIC_CSYNC_62_H */
