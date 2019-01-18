/*	$NetBSD: caa_257.h,v 1.2.2.3 2019/01/18 08:49:55 pgoyette Exp $	*/

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

#ifndef GENERIC_CAA_257_H
#define GENERIC_CAA_257_H 1


typedef struct dns_rdata_caa {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	uint8_t		flags;
	unsigned char *		tag;
	uint8_t		tag_len;
	unsigned char		*value;
	uint16_t		value_len;
} dns_rdata_caa_t;

#endif /* GENERIC_CAA_257_H */
