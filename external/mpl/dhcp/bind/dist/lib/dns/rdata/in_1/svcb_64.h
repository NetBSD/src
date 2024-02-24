/*	$NetBSD: svcb_64.h,v 1.1.2.2 2024/02/24 13:07:17 martin Exp $	*/

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

#ifndef IN_1_SVCB_64_H
#define IN_1_SVCB_64_H 1

/*!
 *  \brief Per draft-ietf-dnsop-svcb-https-02
 */

typedef struct dns_rdata_in_svcb {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	uint16_t priority;
	dns_name_t svcdomain;
	unsigned char *svc;
	uint16_t svclen;
	uint16_t offset;
} dns_rdata_in_svcb_t;

isc_result_t
dns_rdata_in_svcb_first(dns_rdata_in_svcb_t *);

isc_result_t
dns_rdata_in_svcb_next(dns_rdata_in_svcb_t *);

void
dns_rdata_in_svcb_current(dns_rdata_in_svcb_t *, isc_region_t *);

#endif /* IN_1_SVCB_64_H */
