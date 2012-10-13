/*	$NetBSD: one.c,v 1.9 2012/10/13 19:19:39 dholland Exp $	*/

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
static char sccsid[] = "@(#)one.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: one.c,v 1.9 2012/10/13 19:19:39 dholland Exp $");
#endif
#endif /* not lint */

#include "back.h"

static int checkd(struct move *, int);
static int last(void);

int
makmove(struct move *mm, int i)
{
	int     n, d;
	int     max;

	d = mm->d0;
	n = abs(mm->g[i] - mm->p[i]);
	max = (*offptr < 0 ? 7 : last());
	if (board[mm->p[i]] * cturn <= 0)
		return (checkd(mm, d) + 2);
	if (mm->g[i] != home && board[mm->g[i]] * cturn < -1)
		return (checkd(mm, d) + 3);
	if (i || mm->D0 == mm->D1) {
		if (n == max ? mm->D1 < n : mm->D1 != n)
			return (checkd(mm, d) + 1);
	} else {
		if (n == max ? mm->D0 < n && mm->D1 < n : mm->D0 != n && mm->D1 != n)
			return (checkd(mm, d) + 1);
		if (n == max ? mm->D0 < n : mm->D0 != n) {
			if (mm->d0)
				return (checkd(mm, d) + 1);
			mswap(mm);
		}
	}
	if (mm->g[i] == home && *offptr < 0)
		return (checkd(mm, d) + 4);
	mm->h[i] = 0;
	board[mm->p[i]] -= cturn;
	if (mm->g[i] != home) {
		if (board[mm->g[i]] == -cturn) {
			board[home] -= cturn;
			board[mm->g[i]] = 0;
			mm->h[i] = 1;
			if (abs(bar - mm->g[i]) < 7) {
				(*inopp)--;
				if (*offopp >= 0)
					*offopp -= 15;
			}
		}
		board[mm->g[i]] += cturn;
		if (abs(home - mm->g[i]) < 7 && abs(home - mm->p[i]) > 6) {
			(*inptr)++;
			if (*inptr + *offptr == 0)
				*offptr += 15;
		}
	} else {
		(*offptr)++;
		(*inptr)--;
	}
	return (0);
}

void
moverr(struct move *mm, int i)
{
	int     j;

	if (tflag)
		curmove(20, 0);
	else
		writec('\n');
	writel("Error:  ");
	for (j = 0; j <= i; j++) {
		wrint(mm->p[j]);
		writec('-');
		wrint(mm->g[j]);
		if (j < i)
			writec(',');
	}
	writel("... ");
	movback(mm, i);
}


static int
checkd(struct move *mm, int d)
{
	if (mm->d0 != d)
		mswap(mm);
	return (0);
}

static int
last(void)
{
	int     i;

	for (i = home - 6 * cturn; i != home; i += cturn)
		if (board[i] * cturn > 0)
			return (abs(home - i));
	return (-1);
}

void
movback(struct move *mm, int i)
{
	int     j;

	for (j = i - 1; j >= 0; j--)
		backone(mm, j);
}

void
backone(struct move *mm, int i)
{
	board[mm->p[i]] += cturn;
	if (mm->g[i] != home) {
		board[mm->g[i]] -= cturn;
		if (abs(mm->g[i] - home) < 7 && abs(mm->p[i] - home) > 6) {
			(*inptr)--;
			if (*inptr + *offptr < 15 && *offptr >= 0)
				*offptr -= 15;
		}
	} else {
		(*offptr)--;
		(*inptr)++;
	}
	if (mm->h[i]) {
		board[home] += cturn;
		board[mm->g[i]] = -cturn;
		if (abs(bar - mm->g[i]) < 7) {
			(*inopp)++;
			if (*inopp + *offopp == 0)
				*offopp += 15;
		}
	}
}
