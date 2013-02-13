/*	$NetBSD: vis.c,v 1.48 2013/02/13 12:15:09 pooka Exp $	*/

/*-
 * Copyright (c) 1989, 1993
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

/*-
 * Copyright (c) 1999, 2005 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: vis.c,v 1.48 2013/02/13 12:15:09 pooka Exp $");
#endif /* LIBC_SCCS and not lint */
#ifdef __FBSDID
__FBSDID("$FreeBSD$");
#define	_DIAGASSERT(x)	assert(x)
#endif

#include "namespace.h"
#include <sys/types.h>

#include <assert.h>
#include <vis.h>
#include <errno.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#ifdef __weak_alias
__weak_alias(strvisx,_strvisx)
#endif

#if !HAVE_VIS || !HAVE_SVIS
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
 * The reason for going through the trouble to deal with character encodings
 * in vis(3), is that we use this to safe encode output of commands. This
 * safe encoding varies depending on the character set. For example if we
 * display ps output in French, we don't want to display French characters
 * as M-foo.
 */

static wchar_t *do_svis(wchar_t *, wint_t, int, wint_t, const wchar_t *);

#undef BELL
#define BELL L'\a'

#define iswoctal(c)	(((u_char)(c)) >= L'0' && ((u_char)(c)) <= L'7')
#define iswwhite(c)	(c == L' ' || c == L'\t' || c == L'\n')
#define iswsafe(c)	(c == L'\b' || c == BELL || c == L'\r')
#define xtoa(c)		L"0123456789abcdef"[c]
#define XTOA(c)		L"0123456789ABCDEF"[c]

#define MAXEXTRAS	9

#define MAKEEXTRALIST(flag, extra, orig_str)				      \
do {									      \
	const wchar_t *orig = orig_str;					      \
	const wchar_t *o = orig;					      \
	wchar_t *e;							      \
	while (*o++)							      \
		continue;						      \
	extra = calloc((size_t)((o - orig) + MAXEXTRAS), sizeof(*extra));    \
	if (!extra) break;						      \
	for (o = orig, e = extra; (*e++ = *o++) != L'\0';)		      \
		continue;						      \
	e--;								      \
	if (flag & VIS_GLOB) {						      \
		*e++ = L'*';						      \
		*e++ = L'?';						      \
		*e++ = L'[';						      \
		*e++ = L'#';						      \
	}								      \
	if (flag & VIS_SP) *e++ = L' ';					      \
	if (flag & VIS_TAB) *e++ = L'\t';				      \
	if (flag & VIS_NL) *e++ = L'\n';				      \
	if ((flag & VIS_NOSLASH) == 0) *e++ = L'\\';			      \
	*e = L'\0';							      \
} while (/*CONSTCOND*/0)

/*
 * This is do_hvis, for HTTP style (RFC 1808)
 */
static wchar_t *
do_hvis(wchar_t *dst, wint_t c, int flag, wint_t nextc, const wchar_t *extra)
{
	if (iswalnum(c)
	    /* safe */
	    || c == L'$' || c == L'-' || c == L'_' || c == L'.' || c == L'+'
	    /* extra */
	    || c == L'!' || c == L'*' || c == L'\'' || c == L'(' || c == L')'
	    || c == L',')
		dst = do_svis(dst, c, flag, nextc, extra);
	else {
		*dst++ = L'%';
		*dst++ = xtoa(((unsigned int)c >> 4) & 0xf);
		*dst++ = xtoa((unsigned int)c & 0xf);
	}

	return dst;
}

/*
 * This is do_mvis, for Quoted-Printable MIME (RFC 2045)
 * NB: No handling of long lines or CRLF.
 */
static wchar_t *
do_mvis(wchar_t *dst, wint_t c, int flag, wint_t nextc, const wchar_t *extra)
{
	if ((c != L'\n') &&
	    /* Space at the end of the line */
	    ((iswspace(c) && (nextc == L'\r' || nextc == L'\n')) ||
	    /* Out of range */
	    (!iswspace(c) && (c < 33 || (c > 60 && c < 62) || c > 126)) ||
	    /* Specific char to be escaped */ 
	    wcschr(L"#$@[\\]^`{|}~", c) != NULL)) {
		*dst++ = L'=';
		*dst++ = XTOA(((unsigned int)c >> 4) & 0xf);
		*dst++ = XTOA((unsigned int)c & 0xf);
	} else
		dst = do_svis(dst, c, flag, nextc, extra);
	return dst;
}

/*
 * This is do_vis, the central code of vis.
 * dst:	      Pointer to the destination buffer
 * c:	      Character to encode
 * flag:      Flag word
 * nextc:     The character following 'c'
 * extra:     Pointer to the list of extra characters to be
 *	      backslash-protected.
 */
static wchar_t *
do_svis(wchar_t *dst, wint_t c, int flag, wint_t nextc, const wchar_t *extra)
{
	int iswextra;

	iswextra = wcschr(extra, c) != NULL;
	if (!iswextra && (iswgraph(c) || iswwhite(c) ||
	    ((flag & VIS_SAFE) && iswsafe(c)))) {
		*dst++ = c;
		return dst;
	}
	if (flag & VIS_CSTYLE) {
		switch (c) {
		case L'\n':
			*dst++ = L'\\'; *dst++ = L'n';
			return dst;
		case L'\r':
			*dst++ = L'\\'; *dst++ = L'r';
			return dst;
		case L'\b':
			*dst++ = L'\\'; *dst++ = L'b';
			return dst;
		case BELL:
			*dst++ = L'\\'; *dst++ = L'a';
			return dst;
		case L'\v':
			*dst++ = L'\\'; *dst++ = L'v';
			return dst;
		case L'\t':
			*dst++ = L'\\'; *dst++ = L't';
			return dst;
		case L'\f':
			*dst++ = L'\\'; *dst++ = L'f';
			return dst;
		case L' ':
			*dst++ = L'\\'; *dst++ = L's';
			return dst;
		case L'\0':
			*dst++ = L'\\'; *dst++ = L'0';
			if (iswoctal(nextc)) {
				*dst++ = L'0';
				*dst++ = L'0';
			}
			return dst;
		default:
			if (iswgraph(c)) {
				*dst++ = L'\\';
				*dst++ = c;
				return dst;
			}
		}
	}
	if (iswextra || ((c & 0177) == L' ') || (flag & VIS_OCTAL)) {
		*dst++ = L'\\';
		*dst++ = (u_char)(((u_int32_t)(u_char)c >> 6) & 03) + L'0';
		*dst++ = (u_char)(((u_int32_t)(u_char)c >> 3) & 07) + L'0';
		*dst++ =			     (c	      & 07) + L'0';
	} else {
		if ((flag & VIS_NOSLASH) == 0)
			*dst++ = L'\\';

		if (c & 0200) {
			c &= 0177;
			*dst++ = L'M';
		}

		if (iswcntrl(c)) {
			*dst++ = L'^';
			if (c == 0177)
				*dst++ = L'?';
			else
				*dst++ = c + L'@';
		} else {
			*dst++ = L'-';
			*dst++ = c;
		}
	}
	return dst;
}

typedef wchar_t *(*visfun_t)(wchar_t *, wint_t, int, wint_t, const wchar_t *);

/*
 * Return the appropriate encoding function depending on the flags given.
 */
static visfun_t
getvisfun(int flag)
{
	if (flag & VIS_HTTPSTYLE)
		return do_hvis;
	if (flag & VIS_MIMESTYLE)
		return do_mvis;
	return do_svis;
}

/*
 * istrsnvisx()
 * 	The main internal function.
 *	All user-visible functions call this one.
 */
static int
istrsnvisx(char *mbdst, size_t *dlen, const char *mbsrc, size_t mblength,
    int flag, const char *mbextra)
{
	wchar_t *dst, *src, *pdst, *psrc, *start, *extra, *nextra;
	size_t len, olen, mbslength;
	wint_t c;
	visfun_t f;
	int clen, error = -1;

	_DIAGASSERT(mbdst != NULL);
	_DIAGASSERT(mbsrc != NULL);
	_DIAGASSERT(mbextra != NULL);

	psrc = pdst = extra = nextra = NULL;
	if (!mblength)
		mblength = strlen(mbsrc);

	if ((psrc = calloc(mblength + 1, sizeof(*psrc))) == NULL)
		return -1;
	if ((pdst = calloc((4 * mblength) + 1, sizeof(*pdst))) == NULL)
		goto out;
	if ((extra = calloc((strlen(mbextra) + 1), sizeof(*extra))) == NULL)
		goto out;

	dst = pdst;
	src = psrc;

	if (mblength > 1) {
		mbslength = mblength;
		while (mbslength) {
			clen = mbtowc(src, mbsrc, mbslength);
			if (clen < 0)
				break;
			if (clen == 0)
				clen = 1;
			src++;
			mbsrc += clen;
			mbslength -= clen;
		}
		len = src - psrc;	
		src = psrc;
	} else {
		len = mblength;
		src[0] = (wint_t)(u_char)mbsrc[0];
		src[1] = (wint_t)(u_char)mbsrc[1];
	}
	if (mblength < len)
		len = mblength;

	mbstowcs(extra, mbextra, strlen(mbextra));
	MAKEEXTRALIST(flag, nextra, extra);
	if (!nextra) {
		if (dlen && *dlen == 0) {
			errno = ENOSPC;
			goto out;
		}
		*mbdst = '\0';		/* can't create nextra, return "" */
		error = 0;
		goto out;
	}

	f = getvisfun(flag);

	for (start = dst; len > 0; len--) {
		c = *src++;
		dst = (*f)(dst, c, flag, len >= 1 ? *src : L'\0', nextra);
		if (dst == NULL) {
			errno = ENOSPC;
			goto out;
		}
	}

	*dst = L'\0';

	len = dlen ? *dlen : ((wcslen(start) + 1) * MB_LEN_MAX);
	olen = wcstombs(mbdst, start, len * sizeof(*mbdst));

	free(nextra);
	free(extra);
	free(pdst);
	free(psrc);

	return (int)olen;
out:
	free(nextra);
	free(extra);
	free(pdst);
	free(psrc);
	return error;
}
#endif

#if !HAVE_SVIS
/*
 *	The "svis" variants all take an "extra" arg that is a pointer
 *	to a NUL-terminated list of characters to be encoded, too.
 *	These functions are useful e. g. to encode strings in such a
 *	way so that they are not interpreted by a shell.
 */

char *
svis(char *mbdst, int c, int flag, int nextc, const char *mbextra)
{
	char cc[2];
	int ret;

	cc[0] = c;
	cc[1] = nextc;

	ret = istrsnvisx(mbdst, NULL, cc, 1, flag, mbextra);
	if (ret < 0)
		return NULL;
	return mbdst + ret;
}

char *
snvis(char *mbdst, size_t dlen, int c, int flag, int nextc, const char *mbextra)
{
	char cc[2];
	int ret;

	cc[0] = c;
	cc[1] = nextc;

	ret = istrsnvisx(mbdst, &dlen, cc, 1, flag, mbextra);
	if (ret < 0)
		return NULL;
	return mbdst + ret;
}

int
strsvis(char *mbdst, const char *mbsrc, int flag, const char *mbextra)
{
	return istrsnvisx(mbdst, NULL, mbsrc, 0, flag, mbextra);
}

int
strsnvis(char *mbdst, size_t dlen, const char *mbsrc, int flag, const char *mbextra)
{
	return istrsnvisx(mbdst, &dlen, mbsrc, 0, flag, mbextra);
}

int
strsvisx(char *mbdst, const char *mbsrc, size_t len, int flag, const char *mbextra)
{
	return istrsnvisx(mbdst, NULL, mbsrc, len, flag, mbextra);
}

int
strsnvisx(char *mbdst, size_t dlen, const char *mbsrc, size_t len, int flag,
    const char *mbextra)
{
	return istrsnvisx(mbdst, &dlen, mbsrc, len, flag, mbextra);
}
#endif

#if !HAVE_VIS
/*
 * vis - visually encode characters
 */
char *
vis(char *mbdst, int c, int flag, int nextc)
{
	char cc[2];
	int ret;

	cc[0] = c;
	cc[1] = nextc;

	ret = istrsnvisx(mbdst, NULL, cc, 1, flag, "");
	if (ret < 0)
		return NULL;
	return mbdst + ret;
}

char *
nvis(char *mbdst, size_t dlen, int c, int flag, int nextc)
{
	char cc[2];
	int ret;

	cc[0] = c;
	cc[1] = nextc;

	ret = istrsnvisx(mbdst, &dlen, cc, 1, flag, "");
	if (ret < 0)
		return NULL;
	return mbdst + ret;
}

/*
 * strvis - visually encode characters from src into dst
 *
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned.
 */

int
strvis(char *mbdst, const char *mbsrc, int flag)
{
	return istrsnvisx(mbdst, NULL, mbsrc, 0, flag, "");
}

int
strnvis(char *mbdst, size_t dlen, const char *mbsrc, int flag)
{
	return istrsnvisx(mbdst, &dlen, mbsrc, 0, flag, "");
}

/*
 * strvisx - visually encode characters from src into dst
 *
 *	Dst must be 4 times the size of src to account for possible
 *	expansion.  The length of dst, not including the trailing NULL,
 *	is returned.
 *
 *	Strvisx encodes exactly len characters from src into dst.
 *	This is useful for encoding a block of data.
 */

int
strvisx(char *mbdst, const char *mbsrc, size_t len, int flag)
{
	return istrsnvisx(mbdst, NULL, mbsrc, len, flag, "");
}

int
strnvisx(char *mbdst, size_t dlen, const char *mbsrc, size_t len, int flag)
{
	return istrsnvisx(mbdst, &dlen, mbsrc, len, flag, "");
}
#endif
