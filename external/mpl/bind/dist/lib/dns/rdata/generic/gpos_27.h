/*	$NetBSD: gpos_27.h,v 1.1.1.5 2022/09/23 12:09:20 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef GENERIC_GPOS_27_H
#define GENERIC_GPOS_27_H 1

/*!
 *  \brief per RFC1712 */

typedef struct dns_rdata_gpos {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	char *longitude;
	char *latitude;
	char *altitude;
	uint8_t long_len;
	uint8_t lat_len;
	uint8_t alt_len;
} dns_rdata_gpos_t;

#endif /* GENERIC_GPOS_27_H */
