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
 *	$Id: wctype.h,v 1.1.2.1 2000/05/28 22:41:02 minoura Exp $
 */

#ifndef _WCTYPE_H_
#ifndef __IN_WCHAR_H
#define	_WCTYPE_H_
#endif  /* !__IN_WCHAR_H */

#include <sys/cdefs.h>
#include <machine/ansi.h>
#if defined(__FreeBSD__)
#include <runetype.h>
#endif

#ifdef  _BSD_WINT_T_
typedef _BSD_WINT_T_    wint_t;
#undef  _BSD_WINT_T_
#endif

#ifndef WEOF
#define WEOF ((wint_t)-1)
#endif


__BEGIN_DECLS
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
#if 0 /* XXX: not implemented */
/*
int		iswctype	__P((wint_t, wctype_t));
*/
#endif /* XXX: not implemented */
#ifndef __IN_WCHAR_H
#if 0 /* XXX: not implemented */
/*
wint_t		towctrans	__P((wint_t, wctrans_t));
*/
#endif /* XXX: not implemented */
#endif /* !__IN_WCHAR_H */
wint_t		towlower	__P((wint_t));
wint_t		towupper	__P((wint_t));

#ifndef __IN_WCHAR_H
#if 0 /* XXX: not implemented */
/*
wctrans_t	wctrans		__P((const char *));
*/
#endif /* XXX: not implemented */
#endif /* !__IN_WCHAR_H */
#if 0 /* XXX: not implemented */
/*
wctype_t	wctype		__P((const char *));
*/
#endif /* XXX: not implemented */

__END_DECLS

#if defined(__FreeBSD__)
#define	iswalnum(c)	(__istype_w((c), _A)|__isctype_w((c), _D))
#define	iswalpha(c)	__istype_w((c), _A)
#define	iswcntrl(c)	__istype_w((c), _C)
#define	iswdigit(c)	__isctype_w((c), _D)
#define	iswgraph(c)	__istype_w((c), _G)
#define	iswlower(c)	__istype_w((c), _L)
#define	iswprint(c)	__istype_w((c), _R)
#define	iswpunct(c)	__istype_w((c), _P)
#define	iswspace(c)	__istype_w((c), _S)
#define	iswupper(c)	__istype_w((c), _U)
#define	iswxdigit(c)	__isctype_w((c), _X)
#define	towlower(c)	__tolower_w(c)
#define	towupper(c)	__toupper_w(c)
#endif

#endif		/* _WCTYPE_H_ */
