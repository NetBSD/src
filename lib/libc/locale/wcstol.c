/*	$NetBSD: wcstol.c,v 1.1.2.2 2001/10/08 20:19:58 nathanw Exp $	*/
/* $Citrus: xpg4dl/FreeBSD/lib/libc/locale/wcstol.c,v 1.2 2001/09/21 16:11:41 yamt Exp $ */

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)strtol.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: wcstol.c,v 1.1.2.2 2001/10/08 20:19:58 nathanw Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "__wctoint.h"

/*
 * Convert a wide-char string to a long integer.
 */
long
wcstol(nptr, endptr, base)
	const wchar_t *nptr;
	wchar_t **endptr;
	int base;
{
	const wchar_t *s;
	long acc, cutoff;
	wint_t wc;
	int i;
	int neg, any, cutlim;

	_DIAGASSERT(nptr != NULL);
	/* endptr may be NULL */

	/* check base value */
	if (base && (base < 2 || base > 36)) {
		errno = EINVAL;
		return 0;
	}

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	s = nptr;
	do {
		wc = (wchar_t) *s++;
	} while (iswspace(wc));
	if (wc == L'-') {
		neg = 1;
		wc = *s++;
	} else {
		neg = 0;
		if (wc == L'+')
			wc = *s++;
	}
	if ((base == 0 || base == 16) &&
	    wc == L'0' && (*s == L'x' || *s == L'X')) {
		wc = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = wc == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? LONG_MIN : LONG_MAX;
	cutlim = (int)(cutoff % base);
	cutoff /= base;
	if (neg) {
		if (cutlim > 0) {
			cutlim -= base;
			cutoff += 1;
		}
		cutlim = -cutlim;
	}
	for (acc = 0, any = 0;; wc = (wchar_t) *s++) {
		i = __wctoint(wc);
		if (i == -1)
			break;
		if (i >= base)
			break;
		if (any < 0)
			continue;
		if (neg) {
			if (acc < cutoff || (acc == cutoff && i > cutlim)) {
				any = -1;
				acc = LONG_MIN;
				errno = ERANGE;
			} else {
				any = 1;
				acc *= base;
				acc -= i;
			}
		} else {
			if (acc > cutoff || (acc == cutoff && i > cutlim)) {
				any = -1;
				acc = LONG_MAX;
				errno = ERANGE;
			} else {
				any = 1;
				acc *= base;
				acc += i;
			}
		}
	}
	if (endptr != 0)
		/* LINTED interface specification */
		*endptr = (wchar_t *)(any ? s - 1 : nptr);
	return (acc);
}
