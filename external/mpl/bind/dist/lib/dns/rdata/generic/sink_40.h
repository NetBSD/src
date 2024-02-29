/*	$NetBSD: sink_40.h,v 1.3.4.1 2024/02/29 12:34:45 martin Exp $	*/

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

typedef struct dns_rdata_sink_t {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint8_t meaning;
	uint8_t coding;
	uint8_t subcoding;
	uint16_t datalen;
	unsigned char *data;
} dns_rdata_sink_t;
