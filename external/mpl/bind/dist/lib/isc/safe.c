/*	$NetBSD: safe.c,v 1.1.1.1 2018/08/12 12:08:23 christos Exp $	*/

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
