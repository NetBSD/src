/*	$NetBSD: snake.h,v 1.13 1999/09/08 21:45:31 jsm Exp $	*/

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
 *
 *	@(#)snake.h	8.1 (Berkeley) 5/31/93
 */

# include <sys/types.h>
# include <sys/ioctl.h>
# include <assert.h>
# include <err.h>
# include <math.h>
# include <signal.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <termcap.h>
# include <termios.h>

#define ESC	'\033'

struct tbuffer {
	long t[4];
} tbuffer;

char	*CL, *UP, *DO, *ND, *BS,
	*HO, *CM,
	*TA, *LL,
	*KL, *KR, *KU, *KD,
	*TI, *TE, *KS, *KE;
int	LINES, COLUMNS;	/* physical screen size. */
int	lcnt, ccnt;	/* user's idea of screen size */
char	PC;
int	AM, BW;
char	tbuf[1024], tcapbuf[128];
int	Klength;	/* length of KX strings */
int	chunk;		/* amount of money given at a time */
short	ospeed;
#ifdef	debug
#define	cashvalue	(loot-penalty)/25
#else
#define cashvalue	chunk*(loot-penalty)/25
#endif

struct point {
	int col, line;
};
struct point cursor;
struct termios orig, new;

#define	same(s1, s2)	((s1)->line == (s2)->line && (s1)->col == (s2)->col)


void		apr __P((const struct point *, const char *, ...));
void		bs __P((void));
void		chase __P((struct point *, struct point *));
int		chk __P((const struct point *));
void		clear __P((void));
void		cook __P((void));
void		cr __P((void));
void		delay __P((int));
void		done __P((void)) __attribute__((__noreturn__));
void		down __P((void));
void		drawbox __P((void));
void		flushi __P((void));
void		getcap __P((void));
void		gto __P((const struct point *));
void		home __P((void));
void		length __P((int));
void		ll __P((void));
void		logit __P((const char *));
void		mainloop __P((void)) __attribute__((__noreturn__));
void		move __P((struct point *));
void		nd __P((void));
void		outch __P((int));
void		pch __P((int));
void		pchar __P((const struct point *, char));
struct point   *point __P((struct point *, int, int));
int		post __P((int, int));
void		pr __P((const char *, ...));
void		pstring __P((const char *));
int		pushsnake __P((void));
void		putpad __P((const char *));
void		raw __P((void));
void		right __P((const struct point *));
void		setup __P((void));
void		snap __P((void));
void		snrand __P((struct point *));
void		spacewarp __P((int));
void		stop __P((int)) __attribute__((__noreturn__));
int		stretch __P((const struct point *));
void		surround __P((struct point *));
void		suspend __P((void));
void		up __P((void));
void		win __P((const struct point *));
void		winnings __P((int));
