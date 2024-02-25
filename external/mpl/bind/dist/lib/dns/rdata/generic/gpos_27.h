/*	$NetBSD: gpos_27.h,v 1.6.2.1 2024/02/25 15:47:02 martin Exp $	*/

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
