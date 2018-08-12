/*	$NetBSD: txt_16.h,v 1.1.1.1 2018/08/12 12:08:18 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/* */
#ifndef GENERIC_TXT_16_H
#define GENERIC_TXT_16_H 1


typedef struct dns_rdata_txt_string {
		isc_uint8_t    length;
		unsigned char   *data;
} dns_rdata_txt_string_t;

typedef struct dns_rdata_txt {
	dns_rdatacommon_t       common;
	isc_mem_t               *mctx;
	unsigned char           *txt;
	isc_uint16_t            txt_len;
	/* private */
	isc_uint16_t            offset;
} dns_rdata_txt_t;

/*
 * ISC_LANG_BEGINDECLS and ISC_LANG_ENDDECLS are already done
 * via rdatastructpre.h and rdatastructsuf.h.
 */

isc_result_t
dns_rdata_txt_first(dns_rdata_txt_t *);

isc_result_t
dns_rdata_txt_next(dns_rdata_txt_t *);

isc_result_t
dns_rdata_txt_current(dns_rdata_txt_t *, dns_rdata_txt_string_t *);

#endif /* GENERIC_TXT_16_H */
