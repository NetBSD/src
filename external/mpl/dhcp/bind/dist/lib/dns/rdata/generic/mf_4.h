/*	$NetBSD: mf_4.h,v 1.1.4.2 2024/02/29 11:38:53 martin Exp $	*/

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
#ifndef GENERIC_MF_4_H
#define GENERIC_MF_4_H 1

typedef struct dns_rdata_mf {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t mf;
} dns_rdata_mf_t;

#endif /* GENERIC_MF_4_H */
