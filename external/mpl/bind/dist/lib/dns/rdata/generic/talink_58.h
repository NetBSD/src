/*	$NetBSD: talink_58.h,v 1.5.2.1 2024/02/25 15:47:05 martin Exp $	*/

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

/*
 * http://www.iana.org/assignments/dns-parameters/TALINK/talink-completed-template
 */

#pragma once

typedef struct dns_rdata_talink {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t prev;
	dns_name_t next;
} dns_rdata_talink_t;
