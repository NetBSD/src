/*	$NetBSD: iswctype.c,v 1.12 2003/03/21 13:48:53 scw Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
__RCSID("$NetBSD: iswctype.c,v 1.12 2003/03/21 13:48:53 scw Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "rune.h"
#include "runetype.h"
#include "rune_local.h"
#include "_wctrans_local.h"

#ifdef lint
#define __inline
#endif

static __inline _RuneType __runetype_w __P((wint_t));
static __inline int __isctype_w __P((wint_t, _RuneType));
static __inline wint_t __toupper_w __P((wint_t));
static __inline wint_t __tolower_w __P((wint_t));

static __inline _RuneType
__runetype_w(c)
	wint_t c;
{
	_RuneLocale *rl = _CurrentRuneLocale;

	return (_RUNE_ISCACHED(c) ? rl->rl_runetype[c] : ___runetype_mb(c));
}

static __inline int
__isctype_w(c, f)
	wint_t c;
	_RuneType f;
{
	return (!!(__runetype_w(c) & f));
}

static __inline wint_t
__toupper_w(c)
	wint_t c;
{
	return (_towctrans(c, _wctrans_upper(_CurrentRuneLocale)));
}

static __inline wint_t
__tolower_w(c)
	wint_t c;
{
	return (_towctrans(c, _wctrans_lower(_CurrentRuneLocale)));
}

#undef iswalnum
int
iswalnum(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_A|_CTYPE_D));
}

#undef iswalpha
int
iswalpha(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_A));
}

#undef iswblank
int
iswblank(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_B));
}

#undef iswcntrl
int
iswcntrl(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_C));
}

#undef iswdigit
int
iswdigit(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_D));
}

#undef iswgraph
int
iswgraph(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_G));
}

#undef iswlower
int
iswlower(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_L));
}

#undef iswprint
int
iswprint(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_R));
}

#undef iswpunct
int
iswpunct(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_P));
}

#undef iswspace
int
iswspace(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_S));
}

#undef iswupper
int
iswupper(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_U));
}

#undef iswxdigit
int
iswxdigit(c)
	wint_t c;
{
	return (__isctype_w((c), _CTYPE_X));
}

#undef towupper
wint_t
towupper(c)
	wint_t c;
{
	return (__toupper_w(c));
}

#undef towlower
wint_t
towlower(c)
	wint_t c;
{
	return (__tolower_w(c));
}

#undef wcwidth
int
wcwidth(c)
	wchar_t c;
{
        return (((unsigned)__runetype_w(c) & _CTYPE_SWM) >> _CTYPE_SWS);
}

#undef wctrans
wctrans_t
wctrans(charclass)
	const char *charclass;
{
	int i;
	_RuneLocale *rl = _CurrentRuneLocale;

	if (rl->rl_wctrans[_WCTRANS_INDEX_LOWER].te_name==NULL)
		_wctrans_init(rl);

	for (i=0; i<_WCTRANS_NINDEXES; i++)
		if (!strcmp(rl->rl_wctrans[i].te_name, charclass))
			return ((wctrans_t)&rl->rl_wctrans[i]);

	return ((wctrans_t)NULL);
}

#undef towctrans
wint_t
towctrans(c, desc)
	wint_t c;
	wctrans_t desc;
{
	if (desc==NULL) {
		errno = EINVAL;
		return (c);
	}
	return (_towctrans(c, (_WCTransEntry *)desc));
}

#undef wctype
wctype_t
wctype(const char *property)
{
	int i;
	_RuneLocale *rl = _CurrentRuneLocale;

	for (i=0; i<_WCTYPE_NINDEXES; i++)
		if (!strcmp(rl->rl_wctype[i].te_name, property))
			return ((wctype_t)&rl->rl_wctype[i]);
	return ((wctype_t)NULL);
}

#undef iswctype
int
iswctype(wint_t c, wctype_t charclass)
{

	/*
	 * SUSv3: If charclass is 0, iswctype() shall return 0.
	 */
	if (charclass == (wctype_t)0) {
		return 0;
	}

	return (__isctype_w(c, ((_WCTypeEntry *)charclass)->te_mask));
}
