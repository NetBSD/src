/*	$NetBSD: lp_107.h,v 1.1.1.6 2024/02/21 21:54:54 christos Exp $	*/

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

typedef struct dns_rdata_lp {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t pref;
	dns_name_t lp;
} dns_rdata_lp_t;
