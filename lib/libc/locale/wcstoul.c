/*	$NetBSD: wcstoul.c,v 1.1.2.2 2001/10/08 20:19:59 nathanw Exp $	*/
/* $Citrus: xpg4dl/FreeBSD/lib/libc/locale/wcstoul.c,v 1.2 2001/09/21 16:11:41 yamt Exp $ */

/*
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
static char sccsid[] = "@(#)strtoul.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: wcstoul.c,v 1.1.2.2 2001/10/08 20:19:59 nathanw Exp $");
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
 * Convert a wide-char string to an unsigned long integer.
 */
unsigned long
wcstoul(nptr, endptr, base)
	const wchar_t *nptr;
	wchar_t **endptr;
	int base;
{
	const wchar_t *s;
	unsigned long acc, cutoff;
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
	 * See strtol for comments as to the logic used.
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
		base = wc == L'0' ? 8 : 10;

	cutoff = ULONG_MAX / (unsigned long)base;
	cutlim = (int)(ULONG_MAX % (unsigned long)base);
	for (acc = 0, any = 0;; wc = (wchar_t) *s++) {
		i = __wctoint(wc);
		if (i == (wint_t)-1)
			break;
		if (i >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || (acc == cutoff && i > cutlim)) {
			any = -1;
			acc = ULONG_MAX;
			errno = ERANGE;
		} else {
			any = 1;
			acc *= (unsigned long)base;
			acc += i;
		}
	}
	if (neg && any > 0)
		acc = -acc;
	if (endptr != 0)
		/* LINTED interface specification */
		*endptr = (wchar_t *)(any ? s - 1 : nptr);
	return (acc);
}
