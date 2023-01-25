/*	$NetBSD: string.c,v 1.7 2023/01/25 21:43:31 christos Exp $	*/

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

/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2001 Mike Barcroft <mike@FreeBSD.org>
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*! \file */

#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif /* ifdef _GNU_SOURCE */
#include <string.h>

#include <isc/string.h> /* IWYU pragma: keep */
#include <isc/util.h>

#if !defined(HAVE_STRLCPY)
size_t
strlcpy(char *dst, const char *src, size_t size) {
	char *d = dst;
	const char *s = src;
	size_t n = size;

	/* Copy as many bytes as will fit */
	if (n != 0U && --n != 0U) {
		do {
			if ((*d++ = *s++) == 0) {
				break;
			}
		} while (--n != 0U);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0U) {
		if (size != 0U) {
			*d = '\0'; /* NUL-terminate dst */
		}
		while (*s++) {
		}
	}

	return (s - src - 1); /* count does not include NUL */
}
#endif /* !defined(HAVE_STRLCPY) */

#if !defined(HAVE_STRLCAT)
size_t
strlcat(char *dst, const char *src, size_t size) {
	char *d = dst;
	const char *s = src;
	size_t n = size;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0U && *d != '\0') {
		d++;
	}
	dlen = d - dst;
	n = size - dlen;

	if (n == 0U) {
		return (dlen + strlen(s));
	}
	while (*s != '\0') {
		if (n != 1U) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return (dlen + (s - src)); /* count does not include NUL */
}
#endif /* !defined(HAVE_STRLCAT) */

#if !defined(HAVE_STRNSTR)
char *
strnstr(const char *s, const char *find, size_t slen) {
	char c, sc, *r;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	DE_CONST(s, r);
	return (r);
}
#endif

int
isc_string_strerror_r(int errnum, char *buf, size_t buflen) {
#if defined(_WIN32) || defined(_WIN64)
	return (strerror_s(buf, buflen, errnum));
#else  /* if defined(_WIN32) || defined(_WIN64) */
	return (strerror_r(errnum, buf, buflen));
#endif /* if defined(_WIN32) || defined(_WIN64) */
}
