/*	$NetBSD: euc.c,v 1.2 2000/12/21 12:17:35 itojun Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
static char sccsid[] = "@(#)euc.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: euc.c,v 1.2 2000/12/21 12:17:35 itojun Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef lint
#define inline
#endif

int _EUC_init __P((_RuneLocale *));
static inline int _euc_set __P((u_int));
rune_t	_EUC_sgetrune __P((_RuneLocale *, const char *, size_t, char const **, void *));
int	_EUC_sputrune __P((_RuneLocale *, rune_t, char *, size_t, char **, void *));

static _RuneState _EUC_RuneState = {
	0,		/* sizestate */
	NULL,		/* initstate */
	NULL,		/* packstate */
	NULL		/* unpackstate */
};

typedef struct {
	int	count[4];
	rune_t	bits[4];
	rune_t	mask;
} _EucInfo;

int
_EUC_init(rl)
	_RuneLocale *rl;
{
	_EucInfo *ei;
	int x;
	char *v, *e;

	rl->__rune_sgetrune = _EUC_sgetrune;
	rl->__rune_sputrune = _EUC_sputrune;

	if (!rl->__rune_variable) {
		free(rl);
		return (EFTYPE);
	}
	v = (char *) rl->__rune_variable;

	while (*v == ' ' || *v == '\t')
		++v;

	if ((ei = malloc(sizeof(_EucInfo))) == NULL) {
		free(rl);
		return (ENOMEM);
	}
	for (x = 0; x < 4; ++x) {
		ei->count[x] = (int) strtol(v, &e, 0);
		if (v == e || !(v = e)) {
			free(rl);
			free(ei);
			return (EFTYPE);
		}
		while (*v == ' ' || *v == '\t')
			++v;
		ei->bits[x] = (int) strtol(v, &e, 0);
		if (v == e || !(v = e)) {
			free(rl);
			free(ei);
			return (EFTYPE);
		}
		while (*v == ' ' || *v == '\t')
			++v;
	}
	ei->mask = (int)strtol(v, &e, 0);
	if (v == e || !(v = e)) {
		free(rl);
		free(ei);
		return (EFTYPE);
	}
	if (sizeof(_EucInfo) <= rl->__variable_len) {
		memcpy(rl->__rune_variable, ei, sizeof(_EucInfo));
		free(ei);
	} else {
		rl->__rune_variable = &ei;
	}
	rl->__variable_len = sizeof(_EucInfo);

	rl->__rune_RuneState = &_EUC_RuneState;
	rl->__rune_mb_cur_max = 3;

	return (0);
}

#define	CEI(rl)	((_EucInfo *)(rl->__rune_variable))

#define	_SS2	0x008e
#define	_SS3	0x008f

static inline int
_euc_set(c)
	u_int c;
{
	c &= 0xff;

	return ((c & 0x80) ? c == _SS3 ? 3 : c == _SS2 ? 2 : 1 : 0);
}
rune_t
_EUC_sgetrune(rl, string, n, result, state)
	_RuneLocale *rl;
	const char *string;
	size_t n;
	char const **result;
	void *state;
{
	rune_t rune = 0;
	int len, set;

	if (n < 1 || (len = CEI(rl)->count[set = _euc_set(*string)]) > n) {
		if (result)
			*result = string;
		return (___INVALID_RUNE(rl));
	}
	switch (set) {
	case 3:
	case 2:
		--len;
		++string;
		/* FALLTHROUGH */
	case 1:
	case 0:
		while (len-- > 0)
			rune = (rune << 8) | ((u_int)(*string++) & 0xff);
		break;
	}
	if (result)
		*result = string;
	return ((rune & ~CEI(rl)->mask) | CEI(rl)->bits[set]);
}

int
_EUC_sputrune(rl, c, string, n, result, state)
	_RuneLocale *rl;
	rune_t c;
	char *string, **result;
	size_t n;
	void *state;
{
	rune_t m = c & CEI(rl)->mask;
	rune_t nm = c & ~m;
	int i, len;

	if (m == CEI(rl)->bits[1]) {
CodeSet1:
		/* Codeset 1: The first byte must have 0x80 in it. */
		i = len = CEI(rl)->count[1];
		if (n >= len) {
			if (result)
				*result = string + len;
			while (i-- > 0)
				*string++ = (nm >> (i << 3)) | 0x80;
		} else
			if (result)
				*result = (char *) 0;
	} else {
		if (m == CEI(rl)->bits[0]) {
			i = len = CEI(rl)->count[0];
			if (n < len) {
				if (result)
					*result = NULL;
				return (len);
			}
		} else
			if (m == CEI(rl)->bits[2]) {
				i = len = CEI(rl)->count[2];
				if (n < len) {
					if (result)
						*result = NULL;
					return (len);
				}
				*string++ = _SS2;
				--i;
			} else
				if (m == CEI(rl)->bits[3]) {
					i = len = CEI(rl)->count[3];
					if (n < len) {
						if (result)
							*result = NULL;
						return (len);
					}
					*string++ = _SS3;
					--i;
				} else
					goto CodeSet1;	/* Bletch */
		while (i-- > 0)
			*string++ = (nm >> (i << 3)) & 0xff;
		if (result)
			*result = string;
	}
	return (len);
}
