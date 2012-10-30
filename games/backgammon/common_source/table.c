/*	$NetBSD: table.c,v 1.11.6.1 2012/10/30 18:58:17 yamt Exp $	*/

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
static char sccsid[] = "@(#)table.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: table.c,v 1.11.6.1 2012/10/30 18:58:17 yamt Exp $");
#endif
#endif /* not lint */

#include "back.h"

static const char *const help2[] = {
	"   Enter moves as <s>-<f> or <s>/<r> where <s> is the starting",
	"position, <f> is the finishing position, and <r> is the roll.",
	"Remember, each die roll must be moved separately.",
	0
};

struct state {
	char    ch;
	int     fcode;
	int     newst;
};

static int mvl;			/* working copy of move->mvlim */

static const struct state atmata[] = {

	{'R', 1, 0},	{'?', 7, 0},	{'Q', 0, -3},	{'B', 8, 25},
	{'9', 2, 25},	{'8', 2, 25},	{'7', 2, 25},	{'6', 2, 25},
	{'5', 2, 25},	{'4', 2, 25},	{'3', 2, 25},	{'2', 2, 19},
	{'1', 2, 15},	{'0', 2, 25},	{'.', 0, 0},	{'9', 2, 25},
	{'8', 2, 25},	{'7', 2, 25},	{'6', 2, 25},	{'5', 2, 25},

	{'4', 2, 25},	{'3', 2, 25},	{'2', 2, 25},	{'1', 2, 25},
	{'0', 2, 25},	{'/', 0, 32},	{'-', 0, 39},	{'.', 0, 0},
	{'/', 5, 32},	{' ', 6, 3},	{',', 6, 3},	{'\n', 0, -1},
	{'6', 3, 28},	{'5', 3, 28},	{'4', 3, 28},	{'3', 3, 28},
	{'2', 3, 28},	{'1', 3, 28},	{'.', 0, 0},	{'H', 9, 61},

	{'9', 4, 61},	{'8', 4, 61},	{'7', 4, 61},	{'6', 4, 61},
	{'5', 4, 61},	{'4', 4, 61},	{'3', 4, 61},	{'2', 4, 53},
	{'1', 4, 51},	{'0', 4, 61},	{'.', 0, 0},	{'9', 4, 61},
	{'8', 4, 61},	{'7', 4, 61},	{'6', 4, 61},	{'5', 4, 61},
	{'4', 4, 61},	{'3', 4, 61},	{'2', 4, 61},	{'1', 4, 61},

	{'0', 4, 61},	{' ', 6, 3},	{',', 6, 3},	{'-', 5, 39},
	{'\n', 0, -1},	{'.', 0, 0}
};

static int dotable(struct move *, int, int);
static int rsetbrd(struct move *);

int
checkmove(struct move *mm, int ist)
{
	int     j, n;
	char    c;

domove:
	if (ist == 0) {
		if (tflag)
			curmove(curr, 32);
		else
			writel("\t\t");
		writel("Move:  ");
	}
	ist = mvl = ncin = 0;
	for (j = 0; j < 5; j++)
		mm->p[j] = mm->g[j] = -1;

dochar:
	c = readc();

	if (c == 'S') {
		raflag = 0;
		save(mm, 1);
		if (tflag) {
			curmove(cturn == -1 ? 18 : 19, 39);
			ist = -1;
			goto domove;
		} else {
			proll(mm);
			ist = 0;
			goto domove;
		}
	}
	if (c == old.c_cc[VERASE] && ncin > 0) {
		if (tflag)
			curmove(curr, curc - 1);
		else {
			if (old.c_cc[VERASE] == '\010')
				writel("\010 \010");
			else
				writec(cin[ncin - 1]);
		}
		ncin--;
		n = rsetbrd(mm);
		if (n == 0) {
			n = -1;
			if (tflag)
				refresh();
		}
		if ((ist = n) > 0)
			goto dochar;
		goto domove;
	}
	if (c == old.c_cc[VKILL] && ncin > 0) {
		if (tflag) {
			refresh();
			curmove(curr, 39);
			ist = -1;
			goto domove;
		} else
			if (old.c_cc[VERASE] == '\010') {
				for (j = 0; j < ncin; j++)
					writel("\010 \010");
				ist = -1;
				goto domove;
			} else {
				writec('\\');
				writec('\n');
				proll(mm);
				ist = 0;
				goto domove;
			}
	}
	n = dotable(mm, c, ist);
	if (n >= 0) {
		cin[ncin++] = c;
		if (n > 2)
			if ((!tflag) || c != '\n')
				writec(c);
		ist = n;
		if (n)
			goto dochar;
		else
			goto domove;
	}
	if (n == -1 && mvl >= mm->mvlim)
		return (0);
	if (n == -1 && mvl < mm->mvlim - 1)
		return (-4);

	if (n == -6) {
		if (!tflag) {
			if (movokay(mm, mvl + 1)) {
				wrboard();
				movback(mm, mvl + 1);
			}
			proll(mm);
			writel("\t\tMove:  ");
			for (j = 0; j < ncin;)
				writec(cin[j++]);
		} else {
			if (movokay(mm, mvl + 1)) {
				refresh();
				movback(mm, mvl + 1);
			} else
				curmove(cturn == -1 ? 18 : 19, ncin + 39);
		}
		ist = n = rsetbrd(mm);
		goto dochar;
	}
	if (n != -5)
		return (n);
	writec('\007');
	goto dochar;
}

static int
dotable(struct move *mm, int c, int i)
{
	int     a;
	int     test;

	test = (c == 'R');

	while ((a = atmata[i].ch) != '.') {
		if (a == c || (test && a == '\n')) {
			switch (atmata[i].fcode) {

			case 1:
				wrboard();
				if (tflag) {
					curmove(cturn == -1 ? 18 : 19, 0);
					proll(mm);
					writel("\t\t");
				} else
					proll(mm);
				break;

			case 2:
				if (mm->p[mvl] == -1)
					mm->p[mvl] = c - '0';
				else
					mm->p[mvl] = mm->p[mvl] * 10 + c - '0';
				break;

			case 3:
				if (mm->g[mvl] != -1) {
					if (mvl < mm->mvlim)
						mvl++;
					mm->p[mvl] = mm->p[mvl - 1];
				}
				mm->g[mvl] = mm->p[mvl] + cturn * (c - '0');
				if (mm->g[mvl] < 0)
					mm->g[mvl] = 0;
				if (mm->g[mvl] > 25)
					mm->g[mvl] = 25;
				break;

			case 4:
				if (mm->g[mvl] == -1)
					mm->g[mvl] = c - '0';
				else
					mm->g[mvl] = mm->g[mvl] * 10 + c - '0';
				break;

			case 5:
				if (mvl < mm->mvlim)
					mvl++;
				mm->p[mvl] = mm->g[mvl - 1];
				break;

			case 6:
				if (mvl < mm->mvlim)
					mvl++;
				break;

			case 7:
				if (tflag)
					curmove(20, 0);
				else
					writec('\n');
				(void) wrtext(help2);
				if (tflag) {
					curmove(cturn == -1 ? 18 : 19, 39);
				} else {
					writec('\n');
					proll(mm);
					writel("\t\tMove:  ");
				}
				break;

			case 8:
				mm->p[mvl] = bar;
				break;

			case 9:
				mm->g[mvl] = home;
			}

			if (!test || a != '\n')
				return (atmata[i].newst);
			else
				return (-6);
		}
		i++;
	}

	return (-5);
}

static int
rsetbrd(struct move *mm)
{
	int     i, j, n;

	n = 0;
	mvl = 0;
	for (i = 0; i < 4; i++)
		mm->p[i] = mm->g[i] = -1;
	for (j = 0; j < ncin; j++)
		if ((n = dotable(mm, cin[j], n)) < 0)
			return n;
	return (n);
}
