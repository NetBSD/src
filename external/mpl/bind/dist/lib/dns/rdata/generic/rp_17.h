/*	$NetBSD: rp_17.h,v 1.5.2.1 2024/02/25 15:47:04 martin Exp $	*/

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

typedef struct dns_rdata_rp {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t mail;
	dns_name_t text;
} dns_rdata_rp_t;
