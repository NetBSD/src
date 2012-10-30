/*	$NetBSD: tutor.c,v 1.9.6.1 2012/10/30 18:58:18 yamt Exp $	*/

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
static char sccsid[] = "@(#)tutor.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: tutor.c,v 1.9.6.1 2012/10/30 18:58:18 yamt Exp $");
#endif
#endif				/* not lint */

#include "back.h"
#include "tutor.h"

static const char better[] = 
	"That is a legal move, but there is a better one.\n";

static int brdeq(const int *, const int *);
static void clrest(void);

void
tutor(struct move *mm)
{
	int     i, j;

	i = 0;
	begscr = 18;
	cturn = -1;
	home = 0;
	bar = 25;
	inptr = &in[0];
	inopp = &in[1];
	offptr = &off[0];
	offopp = &off[1];
	Colorptr = &color[0];
	colorptr = &color[2];
	colen = 5;
	wrboard();

	while (1) {
		if (!brdeq(test[i].brd, board)) {
			if (tflag && curr == 23)
				curmove(18, 0);
			writel(better);
			nexturn();
			movback(mm, mm->mvlim);
			if (tflag) {
				refresh();
				clrest();
			}
			if ((!tflag) || curr == 19) {
				proll(mm);
				writec('\t');
			} else
				curmove(curr > 19 ? curr - 2 : curr + 4, 25);
			getmove(mm);
			if (cturn == 0)
				leave();
			continue;
		}
		if (tflag)
			curmove(18, 0);
		wrtext(*test[i].com);
		if (!tflag)
			writec('\n');
		if (i == maxmoves)
			break;
		mm->D0 = test[i].roll1;
		mm->D1 = test[i].roll2;
		mm->d0 = 0;
		mm->mvlim = 0;
		for (j = 0; j < 4; j++) {
			if (test[i].mp[j] == test[i].mg[j])
				break;
			mm->p[j] = test[i].mp[j];
			mm->g[j] = test[i].mg[j];
			mm->mvlim++;
		}
		if (mm->mvlim)
			for (j = 0; j < mm->mvlim; j++)
				if (makmove(mm, j))
					writel("AARGH!!!\n");
		if (tflag)
			refresh();
		nexturn();
		mm->D0 = test[i].new1;
		mm->D1 = test[i].new2;
		mm->d0 = 0;
		i++;
		mm->mvlim = movallow(mm);
		if (mm->mvlim) {
			if (tflag)
				clrest();
			proll(mm);
			writec('\t');
			getmove(mm);
			if (tflag)
				refresh();
			if (cturn == 0)
				leave();
		}
	}
	leave();
}

static void
clrest(void)
{
	int     r, c, j;

	r = curr;
	c = curc;
	for (j = r + 1; j < 24; j++) {
		curmove(j, 0);
		cline();
	}
	curmove(r, c);
}

static int
brdeq(const int *b1, const int *b2)
{
	const int    *e;

	e = b1 + 26;
	while (b1 < e)
		if (*b1++ != *b2++)
			return (0);
	return (1);
}
