/*	$NetBSD: citrus_bcs.c,v 1.2 2003/06/30 12:32:21 tshiozak Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_bcs.c,v 1.2 2003/06/30 12:32:21 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#ifndef HOSTPROG
#include "namespace.h"
#endif
#include <assert.h>
#include <stdlib.h>

#include "citrus_namespace.h"
#include "citrus_bcs.h"

int
_citrus_bcs_strcasecmp(const char * __restrict str1,
		       const char * __restrict str2)
{
	int c1=1, c2=1;

	while (c1 && c2 && c1==c2) {
		c1 = _bcs_toupper(*str1++);
		c2 = _bcs_toupper(*str2++);
	}

	return ((c1==c2) ? 0 : ((c1>c2) ? 1:-1));
}

int
_citrus_bcs_strncasecmp(const char * __restrict str1,
			const char * __restrict str2, size_t sz)
{
	int c1=1, c2=1;

	while (c1 && c2 && c1==c2 && sz!=0) {
		c1 = _bcs_toupper(*str1++);
		c2 = _bcs_toupper(*str2++);
		sz--;
	}

	return ((c1==c2) ? 0 : ((c1>c2) ? 1:-1));
}

const char *
_citrus_bcs_skip_ws(const char *p)
{

	while (*p && _bcs_isspace(*p))
		p++;

	return (p);
}

const char *
_citrus_bcs_skip_nonws(const char *p)
{

	while (*p && !_bcs_isspace(*p))
		p++;

	return (p);
}

const char *
_citrus_bcs_skip_ws_len(const char * __restrict p, size_t * __restrict len)
{

	while (*p && *len>0 && _bcs_isspace(*p)) {
		p++;
		(*len)--;
	}

	return (p);
}

const char *
_citrus_bcs_skip_nonws_len(const char * __restrict p, size_t * __restrict len)
{

	while (*p && *len>0 && !_bcs_isspace(*p)) {
		p++;
		(*len)--;
	}

	return (p);
}

void
_citrus_bcs_trunc_rws_len(const char * __restrict p, size_t * __restrict len)
{

	while (*len>0 && _bcs_isspace(p[*len-1]))
		(*len)--;
}

void
_citrus_bcs_convert_to_lower(char *s)
{
	while (*s) {
		*s = _bcs_tolower(*s);
		s++;
	}
}

void _citrus_bcs_convert_to_upper(char *s)
{
	while (*s) {
		*s = _bcs_toupper(*s);
		s++;
	}
}
