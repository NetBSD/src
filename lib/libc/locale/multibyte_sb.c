/*	$NetBSD: multibyte_sb.c,v 1.1 2000/12/25 23:30:58 itojun Exp $	*/

/*
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
static char *sccsid = "from: @(#)multibyte.c	5.1 (Berkeley) 2/18/91";
#else
__RCSID("$NetBSD: multibyte_sb.c,v 1.1 2000/12/25 23:30:58 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <wchar.h>

/*
 * Stub multibyte character functions.
 * This cheezy implementation is fixed to the native single-byte
 * character set.
 */

/*ARGSUSED*/
int
mbsinit(ps)
	const mbstate_t *ps;
{

	return 1;
}

/*ARGSUSED*/
size_t
mbrlen(s, n, ps)
	const char *s;
	size_t n;
	mbstate_t *ps;
{

	if (s == NULL || *s == '\0')
		return 0;
	if (n == 0)
		return -1;
	return 1;
}

int
mblen(s, n)
	const char *s;
	size_t n;
{

	return mbrlen(s, n, NULL);
}

/*ARGSUSED*/
size_t
mbrtowc(pwc, s, n, ps)
	wchar_t *pwc;
	const char *s;
	size_t n;
	mbstate_t *ps;
{

	if (s == NULL)
		return 0;
	if (n == 0)
		return -1;
	if (pwc)
		*pwc = (wchar_t) *s;
	return (*s != '\0');
}

int
mbtowc(pwc, s, n)
	wchar_t *pwc;
	const char *s;
	size_t n;
{

	return mbrtowc(pwc, s, n, NULL);
}

/*ARGSUSED*/
size_t
wcrtomb(s, wchar, ps)
	char *s;
	wchar_t wchar;
	mbstate_t *ps;
{

	if (s == NULL)
		return 0;

	*s = (char) wchar;
	return 1;
}

int
wctomb(s, wchar)
	char *s;
	wchar_t wchar;
{

	return wcrtomb(s, wchar, NULL);
}

/*ARGSUSED*/
size_t
mbsrtowcs(pwcs, s, n, ps)
	wchar_t *pwcs;
	const char **s;
	size_t n;
	mbstate_t *ps;
{
	int count = 0;

	if (!s || !*s)
		return 0;

	if (n != 0) {
		if (pwcs != NULL) {
			do {
				if ((*pwcs++ = (wchar_t) *(*s)++) == 0)
					break;
				count++;
			} while (--n != 0);
		} else {
			do {
				if (((wchar_t)*(*s)++) == 0)
					break;
				count++;
			} while (--n != 0);
		}
	}
	
	return count;
}

size_t
mbstowcs(pwcs, s, n)
	wchar_t *pwcs;
	const char *s;
	size_t n;
{

	return mbsrtowcs(pwcs, &s, n, NULL);
}

/*ARGSUSED*/
size_t
wcsrtombs(s, pwcs, n, ps)
	char *s;
	const wchar_t **pwcs;
	size_t n;
	mbstate_t *ps;
{
	int count = 0;

	_DIAGASSERT(pwcs != NULL);
	if (pwcs == NULL || *pwcs == NULL)
		return (0);

	if (s == NULL) {
		while (*(*pwcs)++ != 0)
			count++;
		return(count);
	}

	if (n != 0) {
		do {
			if ((*s++ = (char) *(*pwcs)++) == 0)
				break;
			count++;
		} while (--n != 0);
	}

	return count;
}

size_t
wcstombs(s, pwcs, n)
	char *s;
	const wchar_t *pwcs;
	size_t n;
{

	return wcsrtombs(s, &pwcs, n, NULL);
}
