/*	$NetBSD: dsync_66.h,v 1.1.1.1 2026/01/29 18:19:55 christos Exp $	*/

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

/* */
#pragma once

typedef struct dns_rdata_dsync {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t type;
	uint8_t scheme;
	uint16_t port;
	dns_name_t target;
} dns_rdata_dsync_t;
