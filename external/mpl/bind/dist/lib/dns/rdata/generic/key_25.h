/*	$NetBSD: key_25.h,v 1.9 2025/01/26 16:25:31 christos Exp $	*/

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
 * \brief Per RFC2535 */

typedef struct dns_rdata_key {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t flags;
	dns_secproto_t protocol;
	dns_secalg_t algorithm;
	uint16_t datalen;
	unsigned char *data;
} dns_rdata_key_t;
