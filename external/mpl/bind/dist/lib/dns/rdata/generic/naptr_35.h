/*	$NetBSD: naptr_35.h,v 1.3.2.2 2019/06/10 22:04:38 christos Exp $	*/

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

#ifndef GENERIC_NAPTR_35_H
#define GENERIC_NAPTR_35_H 1


/*!
 *  \brief Per RFC2915 */

typedef struct dns_rdata_naptr {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	uint16_t		order;
	uint16_t		preference;
	char			*flags;
	uint8_t		flags_len;
	char			*service;
	uint8_t		service_len;
	char			*regexp;
	uint8_t		regexp_len;
	dns_name_t		replacement;
} dns_rdata_naptr_t;

#endif /* GENERIC_NAPTR_35_H */
