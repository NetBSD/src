/*	$NetBSD: opt_41.h,v 1.6.2.1 2024/02/25 15:47:04 martin Exp $	*/

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
 *  \brief Per RFC2671 */

typedef struct dns_rdata_opt_opcode {
	uint16_t opcode;
	uint16_t length;
	unsigned char *data;
} dns_rdata_opt_opcode_t;

typedef struct dns_rdata_opt {
	dns_rdatacommon_t common;
	isc_mem_t *mctx;
	unsigned char *options;
	uint16_t length;
	/* private */
	uint16_t offset;
} dns_rdata_opt_t;

/*
 * ISC_LANG_BEGINDECLS and ISC_LANG_ENDDECLS are already done
 * via rdatastructpre.h and rdatastructsuf.h.
 */

isc_result_t
dns_rdata_opt_first(dns_rdata_opt_t *);

isc_result_t
dns_rdata_opt_next(dns_rdata_opt_t *);

isc_result_t
dns_rdata_opt_current(dns_rdata_opt_t *, dns_rdata_opt_opcode_t *);
