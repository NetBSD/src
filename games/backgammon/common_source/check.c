/*	$NetBSD: check.c,v 1.9 2019/02/18 19:35:44 christos Exp $	*/

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
static char sccsid[] = "@(#)check.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: check.c,v 1.9 2019/02/18 19:35:44 christos Exp $");
#endif
#endif /* not lint */

#include "back.h"

void
getmove(struct move *mm)
{
	int     i, c;

	c = 0;
	for (;;) {
		i = checkmove(mm, c);

		switch (i) {
		case -1:
			if (movokay(mm, mm->mvlim)) {
				if (tflag)
					curmove(20, 0);
				else
					writec('\n');
				for (i = 0; i < mm->mvlim; i++)
					if (mm->h[i])
						wrhit(mm->g[i]);
				nexturn();
				if (*offopp == 15)
					cturn *= -2;
				if (tflag && pnum)
					bflag = pnum;
				return;
			}
			/*FALLTHROUGH*/
		case -4:
		case 0:
			if (tflag)
				refresh();
			if (i != 0 && i != -4)
				break;
			if (tflag)
				curmove(20, 0);
			else
				writec('\n');
			writel(*Colorptr);
			if (i == -4)
				writel(" must make ");
			else
				writel(" can only make ");
			writec(mm->mvlim + '0');
			writel(" move");
			if (mm->mvlim > 1)
				writec('s');
			writec('.');
			writec('\n');
			break;

		case -3:
			if (quit(mm))
				return;
		}

		if (!tflag)
			proll(mm);
		else {
			curmove(cturn == -1 ? 18 : 19, 39);
			cline();
			c = -1;
		}
	}
}

int
movokay(struct move *mm, int mv)
{
	int     i, m;

	if (mm->d0)
		mswap(mm);

	for (i = 0; i < mv; i++) {
		if (mm->p[i] == mm->g[i]) {
			moverr(mm, i);
			curmove(20, 0);
			writel("Attempt to move to same location.\n");
			return (0);
		}
		if (cturn * (mm->g[i] - mm->p[i]) < 0) {
			moverr(mm, i);
			curmove(20, 0);
			writel("Backwards move.\n");
			return (0);
		}
		if (abs(board[bar]) && mm->p[i] != bar) {
			moverr(mm, i);
			curmove(20, 0);
			writel("Men still on bar.\n");
			return (0);
		}
		if ((m = makmove(mm, i))) {
			moverr(mm, i);
			switch (m) {

			case 1:
				writel("Move not rolled.\n");
				break;

			case 2:
				writel("Bad starting position.\n");
				break;

			case 3:
				writel("Destination occupied.\n");
				break;

			case 4:
				writel("Can't remove men yet.\n");
			}
			return (0);
		}
	}
	return (1);
}
