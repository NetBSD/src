/*-
 * Copyright (c)1999 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: wchar.h,v 1.1.2.1 2000/05/28 22:41:02 minoura Exp $
 */

#ifndef _WCHAR_H_
#define	_WCHAR_H_

#undef  __IN_WCHAR_H
#define __IN_WCHAR_H
#include <wctype.h>
#undef  __IN_WCHAR_H

#ifdef  _BSD_SIZE_T_
typedef _BSD_SIZE_T_    size_t;
#undef  _BSD_SIZE_T_
#endif

#ifdef  _BSD_WCHAR_T_
typedef _BSD_WCHAR_T_   wchar_t;
#undef  _BSD_WCHAR_T_
#endif

#ifdef  _BSD_MBSTATE_T_
typedef _BSD_MBSTATE_T_ mbstate_t;
#undef  _BSD_MBSTATE_T_
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX (2147483647)
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN (0)
#endif

#ifndef NULL
#define NULL (0)
#endif

__BEGIN_DECLS
#if 0 /* XXX: not implemented */
/*
wint_t		btowc		__P((int));
int		fwprintf	__P((FILE *, const wchar_t *, ...));
int		fwscanf		__P((FILE *, const wchar_t *, ...));
*/
#endif /* XXX: not implemented */
#if 0
/* defined in wctype.h commonly */
/*
int		iswalnum	__P((wint_t));
int		iswalpha	__P((wint_t));
int		iswcntrl	__P((wint_t));
int		iswdigit	__P((wint_t));
int		iswgraph	__P((wint_t));
int		iswlower	__P((wint_t));
int		iswprint	__P((wint_t));
int		iswpunct	__P((wint_t));
int		iswspace	__P((wint_t));
int		iswupper	__P((wint_t));
int		iswxdigit	__P((wint_t));
*/
#endif
#if 0 /* XXX: not implemented */
/*
int		iswctype	__P((wint_t, wctype_t));
wint_t		fgetwc		__P((FILE *));
wchar_t		*fgetws		__P((wchar_t *, int, FILE *));
wint_t		fputwc		__P((wchar_t, FILE *));
int		fputws		__P((const wchar_t *, FILE *));
int		fwide		__P((FILE *, int));
wint_t		getwc		__P((FILE *));
wint_t		getwchar	__P((void));
*/
#endif /* XXX: not implemented */
int		mbsinit		__P((const mbstate_t *));
size_t		mbrlen		__P((const char *, size_t, mbstate_t *));
size_t		mbrtowc		__P((wchar_t *, const char *, size_t,
				     mbstate_t *));
size_t		mbsrtowcs	__P((wchar_t *, const char **, size_t,
				     mbstate_t *));
#if 0 /* XXX: not implemented */
/*
wint_t		putwc		__P((wchar_t, FILE *));
wint_t		putwchar	__P((wchar_t));
int		swprintf	__P((wchar_t *, size_t, const wchar_t *, ...));
int		swscanf		__P((const wchar_t *, const wchar_t *, ...));
*/
#endif /* XXX: not implemented */
#if 0 /* defined in wctype.h commonly */
/*
wint_t		towlower	__P((wint_t));
wint_t		towupper	__P((wint_t));
*/
#endif
#if 0 /* XXX: not implemented */
/*
wint_t		ungetwc		__P((wint_t, FILE *));
int		vfwprintf	__P((FILE *, const wchar_t *, va_list));
int		vwprintf	__P((const wchar_t *, va_list));
int		vswprintf	__P((wchar_t *, size_t, const wchar_t *,
				     va_list));
*/
#endif /* XXX: not implemented */
size_t		wcrtomb		__P((char *, wchar_t, mbstate_t *));
wchar_t		*wcscat		__P((wchar_t *, const wchar_t *));
wchar_t		*wcschr		__P((const wchar_t *, wchar_t));
int		wcscmp		__P((const wchar_t *, const wchar_t *));
#if 0 /* XXX: not implemented */
/*
int		wcscoll		__P((const wchar_t *, const wchar_t *));
*/
#endif /* XXX: not implemented */
wchar_t		*wcscpy		__P((wchar_t *, const wchar_t *));
size_t		wcscspn		__P((const wchar_t *, const wchar_t *));
#if 0 /* XXX: not implemented */
/*
size_t		wcsftime	__P((wchar_t *, size_t, const wchar_t *,
				     const struct tm *));
*/
#endif /* XXX: not implemented */
size_t		wcslen		__P((const wchar_t *));
wchar_t		*wcsncat	__P((wchar_t *, const wchar_t *, size_t));
int		wcsncmp		__P((const wchar_t *, const wchar_t *,
				     size_t));
wchar_t		*wcsncpy	__P((wchar_t *, const wchar_t *, size_t));
wchar_t		*wcspbrk	__P((const wchar_t *, const wchar_t *));
wchar_t		*wcsrchr	__P((const wchar_t *, wchar_t));
size_t		wcsrtombs	__P((char *, const wchar_t **, size_t,
				     mbstate_t *));
size_t		wcsspn		__P((const wchar_t *, const wchar_t *));
wchar_t		*wcsstr		__P((const wchar_t *, const wchar_t *));
#if 0 /* XXX: not implemented */
/*
double		wcstod		__P((const wchar_t *, wchar_t **));
wchar_t		*wcstok		__P((wchar_t *, const wchar_t *, wchar_t **));
long int	wcstol		__P((const wchar_t *, wchar_t **, int));
unsigned long int wcstoul	__P((const wchar_t *, wchar_t **, int));
wchar_t		*wcswcs		__P((const wchar_t *, const wchar_t *));
*/
#endif /* XXX: not implemented */
int		wcswidth	__P((const wchar_t *, size_t));
#if 0 /* XXX: not implemented */
/*
size_t		wcsxfrm		__P((wchar_t *, const wchar_t *, size_t));
int		wctob		__P((wint_t));
wctype_t	wctype		__P((const char *));
*/
#endif /* XXX: not implemented */
int		wcwidth		__P((wchar_t));
wchar_t		*wmemchr	__P((const wchar_t *, wchar_t, size_t));
int		wmemcmp		__P((const wchar_t *, const wchar_t *,
				     size_t));
wchar_t		*wmemcpy	__P((wchar_t *, const wchar_t *, size_t));
wchar_t		*wmemmove	__P((wchar_t *, const wchar_t *, size_t));
wchar_t		*wmemset	__P((wchar_t *, wchar_t, size_t));
#if 0 /* XXX: not implemented */
/*
int		wprintf		__P((const wchar_t *, ...));
int		wscanf		__P((const wchar_t *, ...));
*/
#endif /* XXX: not implemented */
__END_DECLS

#if defined(__FreeBSD__)
#define wcwidth(c)	((unsigned)__maskrune_w((c), _SWM)>>_SWS)
#endif

#endif
