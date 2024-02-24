/*	$NetBSD: dname_39.h,v 1.1.2.2 2024/02/24 13:07:10 martin Exp $	*/

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

#ifndef GENERIC_DNAME_39_H
#define GENERIC_DNAME_39_H 1

/*!
 *  \brief per RFC2672 */

typedef struct dns_rdata_dname {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t dname;
} dns_rdata_dname_t;

#endif /* GENERIC_DNAME_39_H */
