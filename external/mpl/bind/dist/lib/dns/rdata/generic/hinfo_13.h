/*	$NetBSD: hinfo_13.h,v 1.6 2022/09/23 12:15:31 christos Exp $	*/

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

#ifndef GENERIC_HINFO_13_H
#define GENERIC_HINFO_13_H 1

typedef struct dns_rdata_hinfo {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	char *cpu;
	char *os;
	uint8_t cpu_len;
	uint8_t os_len;
} dns_rdata_hinfo_t;

#endif /* GENERIC_HINFO_13_H */
