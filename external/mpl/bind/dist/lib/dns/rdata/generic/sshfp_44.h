/*	$NetBSD: sshfp_44.h,v 1.2.2.3 2019/01/18 08:49:55 pgoyette Exp $	*/

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


/*!
 *  \brief Per RFC 4255 */

#ifndef GENERIC_SSHFP_44_H
#define GENERIC_SSHFP_44_H 1

typedef struct dns_rdata_sshfp {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	uint8_t		algorithm;
	uint8_t		digest_type;
	uint16_t		length;
	unsigned char		*digest;
} dns_rdata_sshfp_t;

#endif /* GENERIC_SSHFP_44_H */
