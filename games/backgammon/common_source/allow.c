/*	$NetBSD: allow.c,v 1.6.42.1 2012/10/30 18:58:17 yamt Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
static char sccsid[] = "@(#)allow.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: allow.c,v 1.6.42.1 2012/10/30 18:58:17 yamt Exp $");
#endif
#endif /* not lint */

#include "back.h"

int
movallow(struct move *mm)
{
	int     i, m, iold;
	int     r;

	if (mm->d0)
		mswap(mm);
	m = (mm->D0 == mm->D1 ? 4 : 2);
	for (i = 0; i < 4; i++)
		mm->p[i] = bar;
	i = iold = 0;
	while (i < m) {
		if (*offptr == 15)
			break;
		mm->h[i] = 0;
		if (board[bar]) {
			if (i == 1 || m == 4)
				mm->g[i] = bar + cturn * mm->D1;
			else
				mm->g[i] = bar + cturn * mm->D0;
			if ((r = makmove(mm, i)) != 0) {
				if (mm->d0 || m == 4)
					break;
				mswap(mm);
				movback(mm, i);
				if (i > iold)
					iold = i;
				for (i = 0; i < 4; i++)
					mm->p[i] = bar;
				i = 0;
			} else
				i++;
			continue;
		}
		if ((mm->p[i] += cturn) == home) {
			if (i > iold)
				iold = i;
			if (m == 2 && i) {
				movback(mm, i);
				mm->p[i--] = bar;
				if (mm->p[i] != bar)
					continue;
				else
					break;
			}
			if (mm->d0 || m == 4)
				break;
			mswap(mm);
			movback(mm, i);
			for (i = 0; i < 4; i++)
				mm->p[i] = bar;
			i = 0;
			continue;
		}
		if (i == 1 || m == 4)
			mm->g[i] = mm->p[i] + cturn * mm->D1;
		else
			mm->g[i] = mm->p[i] + cturn * mm->D0;
		if (mm->g[i] * cturn > home) {
			if (*offptr >= 0)
				mm->g[i] = home;
			else
				continue;
		}
		if (board[mm->p[i]] * cturn > 0 && (r = makmove(mm, i)) == 0)
			i++;
	}
	movback(mm, i);
	return (iold > i ? iold : i);
}
