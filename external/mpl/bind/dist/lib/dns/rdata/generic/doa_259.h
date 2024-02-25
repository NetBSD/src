/*	$NetBSD: doa_259.h,v 1.6.2.1 2024/02/25 15:47:01 martin Exp $	*/

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

#pragma once

typedef struct dns_rdata_doa {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	unsigned char *mediatype;
	unsigned char *data;
	uint32_t enterprise;
	uint32_t type;
	uint16_t data_len;
	uint8_t location;
	uint8_t mediatype_len;
} dns_rdata_doa_t;
