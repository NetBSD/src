/*	$NetBSD: tsig_250.h,v 1.3.2.2 2019/06/10 22:04:37 christos Exp $	*/

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


#ifndef ANY_255_TSIG_250_H
#define ANY_255_TSIG_250_H 1

/*% RFC2845 */
typedef struct dns_rdata_any_tsig {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	dns_name_t		algorithm;
	uint64_t		timesigned;
	uint16_t		fudge;
	uint16_t		siglen;
	unsigned char *		signature;
	uint16_t		originalid;
	uint16_t		error;
	uint16_t		otherlen;
	unsigned char *		other;
} dns_rdata_any_tsig_t;

#endif /* ANY_255_TSIG_250_H */
