/*	$NetBSD: init.c,v 1.8.60.1 2012/11/20 02:58:45 tls Exp $	*/

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
static char sccsid[] = "@(#)init.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: init.c,v 1.8.60.1 2012/11/20 02:58:45 tls Exp $");
#endif
#endif /* not lint */

#include <termios.h>

#include "back.h"

/*
 * variable initialization.
 */

 /* name of executable object programs */
const char    EXEC[] = "/usr/games/backgammon";
const char    TEACH[] = "/usr/games/teachgammon";

int     pnum = 2;		/* color of player: -1 = white 1 = red 0 =
				 * both 2 = not yet init'ed */
int     acnt = 0;		/* length of args */
int     aflag = 1;		/* flag to ask for rules or instructions */
int     bflag = 0;		/* flag for automatic board printing */
int     cflag = 0;		/* case conversion flag */
int     hflag = 1;		/* flag for cleaning screen */
int     mflag = 0;		/* backgammon flag */
int     raflag = 0;		/* 'roll again' flag for recovered game */
int     rflag = 0;		/* recovered game flag */
int     tflag = 0;		/* cursor addressing flag */
int     iroll = 0;		/* special flag for inputting rolls */
int     rfl = 0;

const char   *const color[] = {"White", "Red", "white", "red"};


struct move gm;
const char	*const *Colorptr;
const char	*const *colorptr;
int	*inopp;
int	*inptr;
int	*offopp;
int	*offptr;
char	args[100];
int	bar;
int	begscr;
int	board[26];
char	cin[100];
int	colen;
int	cturn;
int	curc;
int	curr;
int	dlast;
int	gvalue;
int	home;
int	in[2];
int	ncin;
int	off[2];
int	rscore;
int	table[6][6];
int	wscore;
struct termios	old, noech, raw;

void
move_init(struct move *mm)
{
	mm->D0 = 0;
	mm->D1 = 0;
	mm->mvlim = 0;
	mm->p[0] = mm->p[1] = mm->p[2] = mm->p[3] = mm->p[4] = 0;
	mm->g[0] = mm->g[1] = mm->g[2] = mm->g[3] = mm->g[4] = 0;
	mm->h[0] = mm->h[1] = mm->h[2] = mm->h[3] = 0;
	mm->d0 = 0;
}
