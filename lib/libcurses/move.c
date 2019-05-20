/*	$NetBSD: move.c,v 1.20 2019/05/20 22:17:41 blymn Exp $	*/

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
static char sccsid[] = "@(#)move.c	8.2 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: move.c,v 1.20 2019/05/20 22:17:41 blymn Exp $");
#endif
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS
/*
 * move --
 *	Moves the cursor to the given point on stdscr.
 */
int
move(int y, int x)
{

	return _cursesi_wmove(stdscr, y, x, 1);
}

#endif

/*
 * wmove --
 *	Moves the cursor to the given point.
 */
int
wmove(WINDOW *win, int y, int x)
{
	return _cursesi_wmove(win, y, x, 1);
}


/*
 * _cursesi_wmove:
 *	Moves the cursor to the given point, if keep_old == 0 then
 * update the old cursor position.
 */
int
_cursesi_wmove(WINDOW *win, int y, int x, int keep_old)
{

#ifdef DEBUG
	__CTRACE(__CTRACE_MISC, "_cursesi_wmove: (%d, %d), keep_old: %d\n", y, x, keep_old);
#endif
	if (x < 0 || y < 0)
		return ERR;
	if (x >= win->maxx || y >= win->maxy)
		return ERR;

	win->curx = x;
	win->cury = y;
	if (keep_old == 0) {
		win->ocurx = x;
		win->ocury = y;
	}

	return OK;
}

void
wcursyncup(WINDOW *win)
{

	while (win->orig) {
		_cursesi_wmove(win->orig, win->cury + win->begy - win->orig->begy,
		      win->curx + win->begx - win->orig->begx, 0);
		win = win->orig;
	}
}
