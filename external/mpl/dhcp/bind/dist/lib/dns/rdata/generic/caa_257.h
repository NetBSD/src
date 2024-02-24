/*	$NetBSD: caa_257.h,v 1.1.2.2 2024/02/24 13:07:09 martin Exp $	*/

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

#ifndef GENERIC_CAA_257_H
#define GENERIC_CAA_257_H 1

typedef struct dns_rdata_caa {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint8_t flags;
	unsigned char *tag;
	uint8_t tag_len;
	unsigned char *value;
	uint16_t value_len;
} dns_rdata_caa_t;

#endif /* GENERIC_CAA_257_H */
