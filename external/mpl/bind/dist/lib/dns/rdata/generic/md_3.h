/*	$NetBSD: md_3.h,v 1.1.1.4 2022/09/23 12:09:20 christos Exp $	*/

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
#ifndef GENERIC_MD_3_H
#define GENERIC_MD_3_H 1

typedef struct dns_rdata_md {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t md;
} dns_rdata_md_t;

#endif /* GENERIC_MD_3_H */
