/*	$NetBSD: region.h,v 1.1.2.2 2024/02/24 13:07:26 martin Exp $	*/

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

#ifndef ISC_REGION_H
#define ISC_REGION_H 1

/*! \file isc/region.h */

#include <isc/lang.h>
#include <isc/types.h>

struct isc_region {
	unsigned char *base;
	unsigned int   length;
};

struct isc_textregion {
	char	    *base;
	unsigned int length;
};

/* XXXDCL questionable ... bears discussion.  we have been putting off
 * discussing the region api.
 */
struct isc_constregion {
	const void  *base;
	unsigned int length;
};

struct isc_consttextregion {
	const char  *base;
	unsigned int length;
};

/*@{*/
/*!
 * The region structure is not opaque, and is usually directly manipulated.
 * Some macros are defined below for convenience.
 */

#define isc_region_consume(r, l)          \
	do {                              \
		isc_region_t *_r = (r);   \
		unsigned int  _l = (l);   \
		INSIST(_r->length >= _l); \
		_r->base += _l;           \
		_r->length -= _l;         \
	} while (0)

#define isc_textregion_consume(r, l)        \
	do {                                \
		isc_textregion_t *_r = (r); \
		unsigned int	  _l = (l); \
		INSIST(_r->length >= _l);   \
		_r->base += _l;             \
		_r->length -= _l;           \
	} while (0)

#define isc_constregion_consume(r, l)        \
	do {                                 \
		isc_constregion_t *_r = (r); \
		unsigned int	   _l = (l); \
		INSIST(_r->length >= _l);    \
		_r->base += _l;              \
		_r->length -= _l;            \
	} while (0)
/*@}*/

ISC_LANG_BEGINDECLS

int
isc_region_compare(isc_region_t *r1, isc_region_t *r2);
/*%<
 * Compares the contents of two regions
 *
 * Requires:
 *\li	'r1' is a valid region
 *\li	'r2' is a valid region
 *
 * Returns:
 *\li	 < 0 if r1 is lexicographically less than r2
 *\li	 = 0 if r1 is lexicographically identical to r2
 *\li	 > 0 if r1 is lexicographically greater than r2
 */

ISC_LANG_ENDDECLS

#endif /* ISC_REGION_H */
