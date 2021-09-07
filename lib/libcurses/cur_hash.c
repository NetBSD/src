/*	$NetBSD: cur_hash.c,v 1.14 2021/09/07 01:23:09 rin Exp $	*/

/*
 * Copyright (c) 1992, 1993
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)cur_hash.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: cur_hash.c,v 1.14 2021/09/07 01:23:09 rin Exp $");
#endif
#endif				/* not lint */

#include <sys/types.h>

#include "curses.h"
#include "curses_private.h"

/*
 * __hash_more() is "hashpjw" from the Dragon Book, Aho, Sethi & Ullman, p.436.
 */
unsigned int
__hash_more(const void  *v_s, size_t len, unsigned int h)
{
	unsigned int g;
	size_t i = 0;
	const char *s = v_s;

	while (i < len) {
		h = (h << 4) + s[i];
		if ((g = h & 0xf0000000) != 0) {
			h = h ^ (g >> 24);
			h = h ^ g;
		}
		i++;
	}
	return h;
}

unsigned int
__hash_line(const __LDATA *cp, int ncols)
{
#ifdef HAVE_WCHAR
	unsigned int h;
	const nschar_t *np;
	int x;

	h = 0;
	for (x = 0; x < ncols; x++) {
		h = __hash_more(&cp->ch, sizeof(cp->ch), h);
		h = __hash_more(&cp->attr, sizeof(cp->attr), h);
		for (np = cp->nsp; np != NULL; np = np->next)
			h = __hash_more(&np->ch, sizeof(np->ch), h);
		cp++;
	}
	return h;
#else
	return __hash(cp, (size_t)(ncols * __LDATASIZE));
#endif
}
