/*	$NetBSD: insch.c,v 1.15 2000/05/20 15:12:15 mycroft Exp $	*/

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
static char sccsid[] = "@(#)insch.c	8.2 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: insch.c,v 1.15 2000/05/20 15:12:15 mycroft Exp $");
#endif
#endif				/* not lint */

#include <string.h>

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS

/*
 * insch --
 *	Do an insert-char on the line, leaving (cury, curx) unchanged.
 */
int
insch(chtype ch)
{
	return winsch(stdscr, ch);
}

/*
 * mvinsch --
 *      Do an insert-char on the line at (y, x).
 */
int
mvinsch(int y, int x, chtype ch)
{
	return mvwinsch(stdscr, y, x, ch);
}

/*
 * mvwinsch --
 *      Do an insert-char on the line at (y, x) in the given window.
 */
int
mvwinsch(WINDOW *win, int y, int x, chtype ch)
{
	if (wmove(win, y, x) == ERR)
		return ERR;
	
	return winsch(stdscr, ch);
}

#endif

/*
 * winsch --
 *	Do an insert-char on the line, leaving (cury, curx) unchanged.
 */
int
winsch(WINDOW *win, chtype ch)
{

	__LDATA	*end, *temp1, *temp2;

	end = &win->lines[win->cury]->line[win->curx];
	temp1 = &win->lines[win->cury]->line[win->maxx - 1];
	temp2 = temp1 - 1;
	while (temp1 > end) {
		(void) memcpy(temp1, temp2, sizeof(__LDATA));
		temp1--, temp2--;
	}
	temp1->ch = (wchar_t) ch & __CHARTEXT;
	temp1->bch = win->bch;
	temp1->attr = (attr_t) ch & __ATTRIBUTES;
	temp1->battr = win->battr;
	__touchline(win, (int) win->cury, (int) win->curx, (int) win->maxx - 1);
	if (win->cury == LINES - 1 &&
	    (win->lines[LINES - 1]->line[COLS - 1].ch != ' ' ||
	        win->lines[LINES - 1]->line[COLS - 1].bch != ' ' ||
		win->lines[LINES - 1]->line[COLS - 1].attr != 0 ||
		win->lines[LINES - 1]->line[COLS - 1].battr != 0)) {
		if (win->flags & __SCROLLOK) {
			wrefresh(win);
			scroll(win);
			win->cury--;
		} else
			return (ERR);
	}
	return (OK);
}
