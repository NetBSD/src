/*	$NetBSD: nsec_47.h,v 1.1.4.2 2024/02/29 11:38:54 martin Exp $	*/

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

#ifndef GENERIC_NSEC_47_H
#define GENERIC_NSEC_47_H 1

/*!
 * \brief Per RFC 3845 */

typedef struct dns_rdata_nsec {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	dns_name_t next;
	unsigned char *typebits;
	uint16_t len;
} dns_rdata_nsec_t;

#endif /* GENERIC_NSEC_47_H */
