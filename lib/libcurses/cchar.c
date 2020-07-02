/*   $NetBSD: cchar.c,v 1.12 2020/07/02 23:43:01 uwe Exp $ */

/*
 * Copyright (c) 2005 The NetBSD Foundation Inc.
 * All rights reserved.
 *
 * This code is derived from code donated to the NetBSD Foundation
 * by Ruibiao Qiu <ruibiao@arl.wustl.edu,ruibiao@gmail.com>.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the NetBSD Foundation nor the names of its
 *	contributors may be used to endorse or promote products derived
 *	from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
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
#ifndef lint
__RCSID("$NetBSD: cchar.c,v 1.12 2020/07/02 23:43:01 uwe Exp $");
#endif						  /* not lint */

#include <string.h>

#include "curses.h"
#include "curses_private.h"

/*
 * getcchar --
 *	get a wide-character string and rendition from a cchar_t
 */
int
getcchar(const cchar_t *wcval, wchar_t *wch, attr_t *attrs,
         short *color_pair, void *opts)
{
	wchar_t *wp;
	size_t len;

	if (__predict_false(opts != NULL))
		return ERR;

	wp = wmemchr(wcval->vals, L'\0', CCHARW_MAX);
	len = wp ? wp - wcval->vals : CCHARW_MAX;

	if (wch == NULL)
		return (int)len;

	if (attrs == NULL || color_pair == NULL)
		return ERR;

	if (len > 0) {
		*attrs = wcval->attributes;
		if (__using_color)
			*color_pair = PAIR_NUMBER(wcval->attributes);
		else
			*color_pair = 0;
		wmemcpy(wch, wcval->vals, len);
		wch[len] = L'\0';
	}
	return OK;
}

/*
 * setcchar --
 *	set cchar_t from a wide-character string and rendition
 */
int
setcchar(cchar_t *wcval, const wchar_t *wch, const attr_t attrs,
	 short color_pair, const void *opts)
{
	int i;
	size_t len;

	if (__predict_false(opts != NULL))
		return ERR;

	len = wcslen(wch);
	if (len > CCHARW_MAX || (len > 1 && wcwidth(wch[0]) < 0))
		return ERR;

	/*
	 * If we have a following spacing-character, stop at that point.  We
	 * are only interested in adding non-spacing characters.
	 */
	for (i = 1; i < len; ++i) {
		if (wcwidth(wch[i]) != 0) {
			len = i;
			break;
		}
	}

	memset(wcval, 0, sizeof(*wcval));
	if (len != 0) {
		wcval->attributes = attrs & ~__COLOR;
		if (__using_color && color_pair)
			wcval->attributes |= COLOR_PAIR(color_pair);
		wcval->elements = len;
		memcpy(&wcval->vals, wch, len * sizeof(wchar_t));
	}

	return OK;
}

void
__cursesi_chtype_to_cchar(chtype in, cchar_t *out)
{
	unsigned int idx;

	if (in & __ACS_IS_WACS) {
		idx = in & __CHARTEXT;
		if (idx < NUM_ACS) {
			memcpy(out, &_wacs_char[idx], sizeof(cchar_t));
			out->attributes |= in & __ATTRIBUTES;
			return;
		}
	}
	out->vals[0] = in & __CHARTEXT;
	out->attributes = in & __ATTRIBUTES;
	out->elements = 1;
}
