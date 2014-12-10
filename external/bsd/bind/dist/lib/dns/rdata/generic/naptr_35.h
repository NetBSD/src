/*	$NetBSD: naptr_35.h,v 1.1.1.4 2014/12/10 03:34:42 christos Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2007, 2011, 2012  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef GENERIC_NAPTR_35_H
#define GENERIC_NAPTR_35_H 1

/* Id */

/*!
 *  \brief Per RFC2915 */

typedef struct dns_rdata_naptr {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint16_t		order;
	isc_uint16_t		preference;
	char			*flags;
	isc_uint8_t		flags_len;
	char			*service;
	isc_uint8_t		service_len;
	char			*regexp;
	isc_uint8_t		regexp_len;
	dns_name_t		replacement;
} dns_rdata_naptr_t;

#endif /* GENERIC_NAPTR_35_H */
