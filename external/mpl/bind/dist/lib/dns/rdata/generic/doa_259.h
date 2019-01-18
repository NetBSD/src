/*	$NetBSD: doa_259.h,v 1.2.2.3 2019/01/18 08:49:55 pgoyette Exp $	*/

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

#ifndef GENERIC_DOA_259_H
#define GENERIC_DOA_259_H 1

typedef struct dns_rdata_doa {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	unsigned char *		mediatype;
	unsigned char *		data;
	uint32_t		enterprise;
	uint32_t		type;
	uint16_t		data_len;
	uint8_t		location;
	uint8_t		mediatype_len;
} dns_rdata_doa_t;

#endif /* GENERIC_DOA_259_H */
