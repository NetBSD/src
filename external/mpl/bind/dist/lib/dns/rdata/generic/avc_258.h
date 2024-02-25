/*	$NetBSD: avc_258.h,v 1.6.2.1 2024/02/25 15:47:01 martin Exp $	*/

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

typedef dns_rdata_txt_string_t dns_rdata_avc_string_t;

typedef struct dns_rdata_avc {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	unsigned char *data;
	uint16_t length;
	/* private */
	uint16_t offset;
} dns_rdata_avc_t;

/*
 * ISC_LANG_BEGINDECLS and ISC_LANG_ENDDECLS are already done
 * via rdatastructpre.h and rdatastructsuf.h.
 */
