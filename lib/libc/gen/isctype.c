/* $NetBSD: isctype.c,v 1.16.38.1 2009/01/15 03:24:06 snj Exp $ */

/*-
 * Copyright (c)2008 Citrus Project,
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: isctype.c,v 1.16.38.1 2009/01/15 03:24:06 snj Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <ctype.h>
#include <langinfo.h>
#define __SETLOCALE_SOURCE__
#include <locale.h>
#include <stdio.h>
#if EOF != -1
#error "EOF != -1"
#endif

#include "setlocale_local.h"

#define _CTYPE_TAB(table, i) \
    (((*_current_locale())->cache.table + 1)[i])

#undef isalnum
#undef isalpha
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit

#define _ISCTYPE_FUNC(name, bit) \
int \
is##name(int c) \
{ \
	return (int)(_CTYPE_TAB(ctype_tab, c) & (bit)); \
}

_ISCTYPE_FUNC(alnum,  _U|_L|_N      )
_ISCTYPE_FUNC(alpha,  _U|_L         )
_ISCTYPE_FUNC(cntrl,  _C            )
_ISCTYPE_FUNC(digit,  _N            )
_ISCTYPE_FUNC(graph,  _P|_U|_L|_N   )
_ISCTYPE_FUNC(lower,  _L            )
_ISCTYPE_FUNC(print,  _P|_U|_L|_N|_B)
_ISCTYPE_FUNC(punct,  _P            )
_ISCTYPE_FUNC(space,  _S            )
_ISCTYPE_FUNC(upper,  _U            )
_ISCTYPE_FUNC(xdigit, _N|_X         )

#undef isblank
int
isblank(int c)
{
	/* XXX: FIXME */
        return c == ' ' || c == '\t';
}

#undef toupper
int
toupper(int c)
{
	return (int)_CTYPE_TAB(toupper_tab, c);
}

#undef tolower
int
tolower(int c)
{
	return (int)_CTYPE_TAB(tolower_tab, c);
}

#undef _toupper
int
_toupper(int c)
{
	return (c - 'a' + 'A');
}

#undef _tolower
int
_tolower(int c)
{
	return (c - 'A' + 'a');
}
