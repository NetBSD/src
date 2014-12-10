/*	$NetBSD: safe.c,v 1.3 2014/12/10 04:37:59 christos Exp $	*/

/*
 * Copyright (C) 2013  Internet Systems Consortium, Inc. ("ISC")
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

/* Id */

/*! \file */

#include <config.h>

#include <isc/safe.h>
#include <isc/util.h>

#ifdef _MSC_VER
#pragma optimize("", off)
#endif

isc_boolean_t
isc_safe_memcmp(const void *s1, const void *s2, size_t n) {
	isc_uint8_t acc = 0;

	if (n != 0U) {
		const isc_uint8_t *p1 = s1, *p2 = s2;

		do {
			acc |= *p1++ ^ *p2++;
		} while (--n != 0U);
	}
	return (ISC_TF(acc == 0));
}
