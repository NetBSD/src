/*	$NetBSD: scroll.c,v 1.14 2000/12/19 21:34:24 jdc Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)scroll.c	8.3 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: scroll.c,v 1.14 2000/12/19 21:34:24 jdc Exp $");
#endif
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * scroll --
 *	Scroll the window up a line.
 */
int
scroll(WINDOW *win)
{
	return(wscrl(win, 1));
}

#ifndef _CURSES_USE_MACROS

/*
 * scrl --
 *	Scroll stdscr n lines - up if n is positive, down if n is negative.
 */
int
scrl(int lines)
{
	return wscrl(stdscr, lines);
}

#endif

/*
 * wscrl --
 *	Scroll a window n lines - up if n is positive, down if n is negative.
 */
int
wscrl(WINDOW *win, int lines)
{
	int     oy, ox;

#ifdef DEBUG
	__CTRACE("wscrl: (%0.2o) lines=%d\n", win, lines);
#endif

	if (!(win->flags & __SCROLLOK))
		return (ERR);
	if (!lines)
		return (OK);

	getyx(win, oy, ox);
	wmove(win, 0, 0);
	winsdelln(win, 0 - lines);
	wmove(win, oy, ox);

	if (win == curscr) {
		__cputchar('\n');
		if (!__NONL)
			win->curx = 0;
#ifdef DEBUG
		__CTRACE("scroll: win == curscr\n");
#endif
	}
	return (OK);
}
