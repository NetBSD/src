/*	$NetBSD: afsdb_18.h,v 1.7 2024/02/21 22:52:12 christos Exp $	*/

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
 *  \brief Per RFC1183 */

typedef struct dns_rdata_afsdb {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t subtype;
	dns_name_t server;
} dns_rdata_afsdb_t;
