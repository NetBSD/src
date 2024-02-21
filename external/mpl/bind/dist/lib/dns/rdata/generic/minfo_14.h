/*	$NetBSD: minfo_14.h,v 1.1.1.5 2024/02/21 21:54:53 christos Exp $	*/

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

typedef struct dns_rdata_minfo {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t rmailbox;
	dns_name_t emailbox;
} dns_rdata_minfo_t;
