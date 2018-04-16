/*	$NetBSD: safe.c,v 1.4.14.1 2018/04/16 01:57:58 pgoyette Exp $	*/

/*
 * Copyright (C) 2013, 2015, 2017  Internet Systems Consortium, Inc. ("ISC")
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

/*! \file */

#include <config.h>

#include <isc/safe.h>
#include <isc/string.h>
#include <isc/util.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef _MSC_VER
#pragma optimize("", off)
#endif

isc_boolean_t
isc_safe_memequal(const void *s1, const void *s2, size_t n) {
	isc_uint8_t acc = 0;

	if (n != 0U) {
		const isc_uint8_t *p1 = s1, *p2 = s2;

		do {
			acc |= *p1++ ^ *p2++;
		} while (--n != 0U);
	}
	return (ISC_TF(acc == 0));
}


int
isc_safe_memcompare(const void *b1, const void *b2, size_t len) {
	const unsigned char *p1 = b1, *p2 = b2;
	size_t i;
	int res = 0, done = 0;

	for (i = 0; i < len; i++) {
		/* lt is -1 if p1[i] < p2[i]; else 0. */
		int lt = (p1[i] - p2[i]) >> CHAR_BIT;

		/* gt is -1 if p1[i] > p2[i]; else 0. */
		int gt = (p2[i] - p1[i]) >> CHAR_BIT;

		/* cmp is 1 if p1[i] > p2[i]; -1 if p1[i] < p2[i]; else 0. */
		int cmp = lt - gt;

		/* set res = cmp if !done. */
		res |= cmp & ~done;

		/* set done if p1[i] != p2[i]. */
		done |= lt | gt;
	}

	return (res);
}

void
isc_safe_memwipe(void *ptr, size_t len) {
	if (ISC_UNLIKELY(ptr == NULL || len == 0))
		return;

#ifdef WIN32
	SecureZeroMemory(ptr, len);
#elif HAVE_EXPLICIT_BZERO
	explicit_bzero(ptr, len);
#else
	memset(ptr, 0, len);
#endif
}
