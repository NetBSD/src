/*	$NetBSD: addbytes.c,v 1.19 2000/04/18 22:45:23 jdc Exp $	*/

/*
 * Copyright (c) 1987, 1993, 1994
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
static char sccsid[] = "@(#)addbytes.c	8.4 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: addbytes.c,v 1.19 2000/04/18 22:45:23 jdc Exp $");
#endif
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

#define	SYNCH_IN	{y = win->cury; x = win->curx;}
#define	SYNCH_OUT	{win->cury = y; win->curx = x;}

#ifndef _CURSES_USE_MACROS

/*
 * addbytes --
 *      Add the character to the current position in stdscr.
 */
int
addbytes(const char *bytes, int count)
{
	return __waddbytes(stdscr, bytes, count, 0);
}

/*
 * waddbytes --
 *      Add the character to the current position in the given window.
 */
int
waddbytes(WINDOW *win, const char *bytes, int count)
{
	return __waddbytes(win, bytes, count, 0);
}

/*
 * mvaddbytes --
 *      Add the characters to stdscr at the location given.
 */
int
mvaddbytes(int y, int x, const char *bytes, int count)
{
	return mvwaddbytes(stdscr, y, x, bytes, count);
}

/*
 * mvwaddbytes --
 *      Add the characters to the given window at the location given.
 */
int
mvwaddbytes(WINDOW *win, int y, int x, const char *bytes, int count)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return __waddbytes(win, bytes, count, 0);
}

#endif

/*
 * waddbytes --
 *	Add the character to the current position in the given window.
 */
int
__waddbytes(WINDOW *win, const char *bytes, int count, attr_t attr)
{
	static char	 blanks[] = "        ";
	int		 c, newx, x, y;
	attr_t		 attributes;
	__LINE		*lp;

	SYNCH_IN;

	while (count--) {
		c = *bytes++;
#ifdef DEBUG
		__CTRACE("ADDBYTES('%c', %x) at (%d, %d)\n", c, attr, y, x);
#endif
		switch (c) {
		case '\t':
			SYNCH_OUT;
			if (waddbytes(win, blanks, 8 - (x % 8)) == ERR)
				return (ERR);
			SYNCH_IN;
			break;

		default:
#ifdef DEBUG
			__CTRACE("ADDBYTES(%0.2o, %d, %d)\n", win, y, x);
#endif

			lp = win->lines[y];
			if (lp->flags & __ISPASTEOL) {
				lp->flags &= ~__ISPASTEOL;
		newline:	if (y == win->maxy - 1) {
					if (win->flags & __SCROLLOK) {
						SYNCH_OUT;
						scroll(win);
						SYNCH_IN;
						lp = win->lines[y];
						x = 0;
					} else
						return (ERR);
				} else {
					y++;
					lp = win->lines[y];
					x = 0;
				}
				if (c == '\n')
					break;
			}

			attributes = '\0';
			if (win->wattr & __STANDOUT || attr & __STANDOUT)
				attributes |= __STANDOUT;
			if (win->wattr & __UNDERSCORE || attr & __UNDERSCORE)
				attributes |= __UNDERSCORE;
			if (win->wattr & __REVERSE || attr & __REVERSE)
				attributes |= __REVERSE;
			if (win->wattr & __BLINK || attr & __BLINK)
				attributes |= __BLINK;
			if (win->wattr & __DIM || attr & __DIM)
				attributes |= __DIM;
			if (win->wattr & __BOLD || attr & __BOLD)
				attributes |= __BOLD;
			if (win->wattr & __BLANK || attr & __BLANK)
				attributes |= __BLANK;
			if (win->wattr & __PROTECT || attr & __PROTECT)
				attributes |= __PROTECT;
			if (win->wattr & __ALTCHARSET || attr & __ALTCHARSET)
				attributes |= __ALTCHARSET;
			if (attr & __COLOR)
				attributes |= attr & __COLOR;
			else if (win->wattr & __COLOR)
				attributes |= win->wattr & __COLOR;
#ifdef DEBUG
			__CTRACE("ADDBYTES: 1: y = %d, x = %d, firstch = %d, lastch = %d\n",
			    y, x, *win->lines[y]->firstchp,
			    *win->lines[y]->lastchp);
#endif
			if (lp->line[x].ch != c ||
			    lp->line[x].attr != attributes ||
			    lp->line[x].bch != win->bch ||
			    lp->line[x].battr != win->battr) {
				newx = x + win->ch_off;
				if (!(lp->flags & __ISDIRTY))
					lp->flags |= __ISDIRTY;
				/*
				 * firstchp/lastchp are shared between
				 * parent window and sub-window.
				 */
				if (newx < *lp->firstchp)
					*lp->firstchp = newx;
				if (newx > *lp->lastchp)
					*lp->lastchp = newx;
#ifdef DEBUG
				__CTRACE("ADDBYTES: change gives f/l: %d/%d [%d/%d]\n",
				    *lp->firstchp, *lp->lastchp,
				    *lp->firstchp - win->ch_off,
				    *lp->lastchp - win->ch_off);
#endif
			}
			lp->line[x].ch = c;
			lp->line[x].bch = win->bch;
			if (attributes & __STANDOUT)
				lp->line[x].attr |= __STANDOUT;
			else
				lp->line[x].attr &= ~__STANDOUT;
			if (attributes & __UNDERSCORE)
				lp->line[x].attr |= __UNDERSCORE;
			else
				lp->line[x].attr &= ~__UNDERSCORE;
			if (attributes & __REVERSE)
				lp->line[x].attr |= __REVERSE;
			else
				lp->line[x].attr &= ~__REVERSE;
			if (attributes & __BLINK)
				lp->line[x].attr |= __BLINK;
			else
				lp->line[x].attr &= ~__BLINK;
			if (attributes & __DIM)
				lp->line[x].attr |= __DIM;
			else
				lp->line[x].attr &= ~__DIM;
			if (attributes & __BOLD)
				lp->line[x].attr |= __BOLD;
			else
				lp->line[x].attr &= ~__BOLD;
			if (attributes & __BLANK)
				lp->line[x].attr |= __BLANK;
			else
				lp->line[x].attr &= ~__BLANK;
			if (attributes & __PROTECT)
				lp->line[x].attr |= __PROTECT;
			else
				lp->line[x].attr &= ~__PROTECT;
			if (attributes & __ALTCHARSET)
				lp->line[x].attr |= __ALTCHARSET;
			else
				lp->line[x].attr &= ~__ALTCHARSET;
			if (attributes & __COLOR) {
				lp->line[x].attr &= ~__COLOR;
				lp->line[x].attr |= attributes & __COLOR;
			} else
				lp->line[x].attr &= ~__COLOR;
			lp->line[x].battr = win->battr;
			if (x == win->maxx - 1)
				lp->flags |= __ISPASTEOL;
			else
				x++;
#ifdef DEBUG
			__CTRACE("ADDBYTES: 2: y = %d, x = %d, firstch = %d, lastch = %d\n",
			    y, x, *win->lines[y]->firstchp,
			    *win->lines[y]->lastchp);
#endif
			break;
		case '\n':
			SYNCH_OUT;
			wclrtoeol(win);
			SYNCH_IN;
			if (!NONL)
				x = 0;
			goto newline;
		case '\r':
			x = 0;
			break;
		case '\b':
			if (--x < 0)
				x = 0;
			break;
		}
	}
	SYNCH_OUT;
	return (OK);
}
