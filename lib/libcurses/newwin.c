/*
 * Copyright (c) 1981 Regents of the University of California.
 * All rights reserved.
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

#ifndef lint
/*static char sccsid[] = "from: @(#)newwin.c	5.6 (Berkeley) 8/23/92";*/
static char rcsid[] = "$Id: newwin.c,v 1.4 1993/08/07 05:49:00 mycroft Exp $";
#endif	/* not lint */

#include <curses.h>
#include <stdlib.h>

#undef	nl		/* Don't need it here, and it interferes. */

static WINDOW *makenew __P((int, int, int, int));

/*
 * newwin --
 *	Allocate space for and set up defaults for a new window.
 */
WINDOW *
newwin(num_lines, num_cols, begy, begx)
	int num_lines, num_cols, begy, begx;
{
	register WINDOW *win;
	register int by, bx, i, j, nl, nc;
	register char *sp;

	by = begy;
	bx = begx;
	nl = num_lines;
	nc = num_cols;

	if (nl == 0)
		nl = LINES - by;
	if (nc == 0)
		nc = COLS - bx;
	if ((win = makenew(nl, nc, by, bx)) == NULL)
		return (NULL);
	if ((win->_firstch = malloc(nl * sizeof(win->_firstch[0]))) == NULL) {
		free(win->_y);
		free(win);
		return (NULL);
	}
	if ((win->_lastch = malloc(nl * sizeof(win->_lastch[0]))) == NULL) {
		free(win->_y);
		free(win->_firstch);
		free(win);
		return (NULL);
	}
	win->_nextp = win;
	for (i = 0; i < nl; i++) {
		win->_firstch[i] = _NOCHANGE;
		win->_lastch[i] = _NOCHANGE;
	}
	for (i = 0; i < nl; i++)
		if ((win->_y[i] = malloc(nc * sizeof(win->_y[0]))) == NULL) {
			for (j = 0; j < i; j++)
				free(win->_y[j]);
			free(win->_firstch);
			free(win->_lastch);
			free(win->_y);
			free(win);
			return (NULL);
		} else
			for (sp = win->_y[i]; sp < win->_y[i] + nc;)
				*sp++ = ' ';
	win->_ch_off = 0;
#ifdef DEBUG
	__TRACE("newwin: win->_ch_off = %d\n", win->_ch_off);
#endif
	return (win);
}

WINDOW *
subwin(orig, num_lines, num_cols, begy, begx)
	register WINDOW *orig;
	int num_lines, num_cols, begy, begx;
{
	register WINDOW *win;
	register int by, bx, nl, nc;

	by = begy;
	bx = begx;
	nl = num_lines;
	nc = num_cols;

	/* Make sure window fits inside the original one. */
#ifdef	DEBUG
	__TRACE("subwin: (%0.2o, %d, %d, %d, %d)\n", orig, nl, nc, by, bx);
#endif
	if (by < orig->_begy || bx < orig->_begx
	    || by + nl > orig->_maxy + orig->_begy
	    || bx + nc > orig->_maxx + orig->_begx)
		return (NULL);
	if (nl == 0)
		nl = orig->_maxy + orig->_begy - by;
	if (nc == 0)
		nc = orig->_maxx + orig->_begx - bx;
	if ((win = makenew(nl, nc, by, bx)) == NULL)
		return (NULL);
	win->_nextp = orig->_nextp;
	orig->_nextp = win;
	win->_orig = orig;
	__set_subwin(orig, win);
	return (win);
}

/*
 * This code is shared with mvwin().
 */
void
__set_subwin(orig, win)
	register WINDOW *orig, *win;
{
	register int i, j, k;

	j = win->_begy - orig->_begy;
	k = win->_begx - orig->_begx;
	win->_ch_off = k;
#ifdef DEBUG
	__TRACE("__set_subwin: win->_ch_off = %d\n", win->_ch_off);
#endif
	win->_firstch = &orig->_firstch[j];
	win->_lastch = &orig->_lastch[j];
	for (i = 0; i < win->_maxy; i++, j++)
		win->_y[i] = &orig->_y[j][k];
}

/*
 * makenew --
 *	Set up a window buffer and returns a pointer to it.
 */
static WINDOW *
makenew(num_lines, num_cols, begy, begx)
	int num_lines, num_cols, begy, begx;
{
	register WINDOW *win;
	register int by, bx, nl, nc;

	by = begy;
	bx = begx;
	nl = num_lines;
	nc = num_cols;

#ifdef	DEBUG
	__TRACE("makenew: (%d, %d, %d, %d)\n", nl, nc, by, bx);
#endif
	if ((win = malloc(sizeof(*win))) == NULL)
		return (NULL);
#ifdef DEBUG
	__TRACE("makenew: nl = %d\n", nl);
#endif
	if ((win->_y = malloc(nl * sizeof(win->_y[0]))) == NULL) {
		free(win);
		return (NULL);
	}
#ifdef DEBUG
	__TRACE("makenew: nc = %d\n", nc);
#endif
	win->_cury = win->_curx = 0;
	win->_clear = 0;
	win->_maxy = nl;
	win->_maxx = nc;
	win->_begy = by;
	win->_begx = bx;
	win->_flags = 0;
	win->_scroll = win->_leave = 0;
	__swflags(win);
#ifdef DEBUG
	__TRACE("makenew: win->_clear = %d\n", win->_clear);
	__TRACE("makenew: win->_leave = %d\n", win->_leave);
	__TRACE("makenew: win->_scroll = %d\n", win->_scroll);
	__TRACE("makenew: win->_flags = %0.2o\n", win->_flags);
	__TRACE("makenew: win->_maxy = %d\n", win->_maxy);
	__TRACE("makenew: win->_maxx = %d\n", win->_maxx);
	__TRACE("makenew: win->_begy = %d\n", win->_begy);
	__TRACE("makenew: win->_begx = %d\n", win->_begx);
#endif
	return (win);
}

void
__swflags(win)
	register WINDOW *win;
{
	win->_flags &= ~(_ENDLINE | _FULLLINE | _FULLWIN | _SCROLLWIN);
	if (win->_begx + win->_maxx == COLS) {
		win->_flags |= _ENDLINE;
		if (win->_begx == 0) {
			if (AL && DL)
				win->_flags |= _FULLLINE;
			if (win->_maxy == LINES && win->_begy == 0)
				win->_flags |= _FULLWIN;
		}
		if (win->_begy + win->_maxy == LINES)
			win->_flags |= _SCROLLWIN;
	}
}
