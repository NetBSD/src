/*	$NetBSD: line.c,v 1.3 2001/06/13 10:45:57 wiz Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: line.c,v 1.3 2001/06/13 10:45:57 wiz Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * hline --
 *    Draw a horizontal line of character c on stdscr.
 */
int
hline(chtype ch, int count)
{
	return whline(stdscr, ch, count);
}

/*
 * mvhline --
 *    Move to location (y, x) and draw a horizontal line of character c
 *    on stdscr.
 */
int
mvhline(int y, int x, chtype ch, int count)
{
	return mvwhline(stdscr, y, x, ch, count);
}

/*
 * mvwhline --
 *    Move to location (y, x) and draw a horizontal line of character c
 *    in the given window.
 */
int
mvwhline(WINDOW *win, int y, int x, chtype ch, int count)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return whline(win, ch, count);
}

/*
 * whline --
 *    Draw a horizontal line of character c in the given window moving
 *    towards the rightmost column.  At most count characters are drawn
 *    or until the edge of the screen, whichever comes first.
 */
int
whline(WINDOW *win, chtype ch, int count)
{
	int ocurx, n, i;

	n = min(count, win->maxx - win->curx);
	ocurx = win->curx;
	
	if (!(ch & __CHARTEXT))
		ch |= ACS_HLINE;
	for (i = 0; i < n; i++)
		mvwaddch(win, win->cury, ocurx + i, ch);
		
	wmove(win, win->cury, ocurx);
	return OK;
}

/*
 * vline --
 *    Draw a vertical line of character ch on stdscr.
 */
int
vline(chtype ch, int count)
{
	return wvline(stdscr, ch, count);
}

/*
 * mvvline --
 *   Move to the given location an draw a vertical line of character ch.
 */
int
mvvline(int y, int x, chtype ch, int count)
{
	return mvwvline(stdscr, y, x, ch, count);
}

/*
 * mvwvline --
 *    Move to the given location and draw a vertical line of character ch
 *    on the given window.
 */
int
mvwvline(WINDOW *win, int y, int x, chtype ch, int count)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return wvline(win, ch, count);
}

/*
 * wvline --
 *    Draw a vertical line of character ch in the given window moving
 *    towards the bottom of the screen.  At most count characters are drawn
 *    or until the edge of the screen, whichever comes first.
 */
int
wvline(WINDOW *win, chtype ch, int count)
{
	int ocury, ocurx, n, i;

	n = min(count, win->maxy - win->cury);
	ocury = win->cury;
	ocurx = win->curx;

	if (!(ch & __CHARTEXT))
		ch |= ACS_VLINE;
	for (i = 0; i < n; i++)
		mvwaddch(win, ocury + i, ocurx, ch);

	wmove(win, ocury, ocurx);
	return OK;
}
