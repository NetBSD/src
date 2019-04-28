/*	$NetBSD: key_25.h,v 1.4 2019/04/28 00:01:14 christos Exp $	*/

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

#ifndef GENERIC_KEY_25_H
#define GENERIC_KEY_25_H 1


/*!
 * \brief Per RFC2535 */

typedef struct dns_rdata_key {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	uint16_t		flags;
	dns_secproto_t		protocol;
	dns_secalg_t		algorithm;
	uint16_t		datalen;
	unsigned char *		data;
} dns_rdata_key_t;


#endif /* GENERIC_KEY_25_H */
