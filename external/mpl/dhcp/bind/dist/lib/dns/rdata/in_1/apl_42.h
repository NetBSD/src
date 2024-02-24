/*	$NetBSD: apl_42.h,v 1.1.2.2 2024/02/24 13:07:16 martin Exp $	*/

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
#ifndef IN_1_APL_42_H
#define IN_1_APL_42_H 1

typedef struct dns_rdata_apl_ent {
	bool negative;
	uint16_t family;
	uint8_t prefix;
	uint8_t length;
	unsigned char *data;
} dns_rdata_apl_ent_t;

typedef struct dns_rdata_in_apl {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	/* type & class specific elements */
	unsigned char *apl;
	uint16_t apl_len;
	/* private */
	uint16_t offset;
} dns_rdata_in_apl_t;

/*
 * ISC_LANG_BEGINDECLS and ISC_LANG_ENDDECLS are already done
 * via rdatastructpre.h and rdatastructsuf.h.
 */

isc_result_t
dns_rdata_apl_first(dns_rdata_in_apl_t *);

isc_result_t
dns_rdata_apl_next(dns_rdata_in_apl_t *);

isc_result_t
dns_rdata_apl_current(dns_rdata_in_apl_t *, dns_rdata_apl_ent_t *);

unsigned int
dns_rdata_apl_count(const dns_rdata_in_apl_t *apl);

#endif /* IN_1_APL_42_H */
