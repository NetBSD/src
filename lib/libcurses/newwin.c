/*	$NetBSD: newwin.c,v 1.25 2001/05/17 19:04:01 jdc Exp $	*/

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
static char sccsid[] = "@(#)newwin.c	8.3 (Berkeley) 7/27/94";
#else
__RCSID("$NetBSD: newwin.c,v 1.25 2001/05/17 19:04:01 jdc Exp $");
#endif
#endif				/* not lint */

#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

extern struct __winlist	*winlistp;

static WINDOW *__makenew(int nlines, int ncols, int by, int bx, int sub);

/*
 * derwin --
 *      Create a new window in the same manner as subwin but (by, bx)
 *      are relative to the origin of window orig instead of absolute.
 */
WINDOW *
derwin(WINDOW *orig, int nlines, int ncols, int by, int bx)
{
	if (orig == NULL)
		return ERR;
	
	return subwin(orig, nlines, ncols, orig->begy + by, orig->begx + bx);
}

/*
 * dupwin --
 *      Create a copy of the given window.
 */
WINDOW *
dupwin(WINDOW *win)
{
	WINDOW *new_one;

	if ((new_one =
	     newwin(win->maxy, win->maxx, win->begy, win->begx)) == NULL)
		return NULL;

	overwrite(win, new_one);
	return new_one;
}

	
/*
 * newwin --
 *	Allocate space for and set up defaults for a new window.
 */
WINDOW *
newwin(int nlines, int ncols, int by, int bx)
{
	WINDOW *win;
	__LINE *lp;
	int     i, j;
	__LDATA *sp;

	if (nlines == 0)
		nlines = LINES - by;
	if (ncols == 0)
		ncols = COLS - bx;

	if ((win = __makenew(nlines, ncols, by, bx, 0)) == NULL)
		return (NULL);

	win->nextp = win;
	win->ch_off = 0;
	win->orig = NULL;

#ifdef DEBUG
	__CTRACE("newwin: win->ch_off = %d\n", win->ch_off);
#endif

	for (i = 0; i < nlines; i++) {
		lp = win->lines[i];
		lp->flags = 0;
		for (sp = lp->line, j = 0; j < ncols; j++, sp++) {
			sp->ch = ' ';
			sp->bch = ' ';
			sp->attr = 0;
			sp->battr = 0;
		}
		lp->hash = __hash((char *)(void *)lp->line,
		    (int) (ncols * __LDATASIZE));
	}
	return (win);
}

WINDOW *
subwin(WINDOW *orig, int nlines, int ncols, int by, int bx)
{
	int     i;
	__LINE *lp;
	WINDOW *win;

	/* Make sure window fits inside the original one. */
#ifdef	DEBUG
	__CTRACE("subwin: (%0.2o, %d, %d, %d, %d)\n", orig, nlines, ncols, by, bx);
#endif
	if (by < orig->begy || bx < orig->begx
	    || by + nlines > orig->maxy + orig->begy
	    || bx + ncols > orig->maxx + orig->begx)
		return (NULL);
	if (nlines == 0)
		nlines = orig->maxy + orig->begy - by;
	if (ncols == 0)
		ncols = orig->maxx + orig->begx - bx;
	if ((win = __makenew(nlines, ncols, by, bx, 1)) == NULL)
		return (NULL);
	win->nextp = orig->nextp;
	orig->nextp = win;
	win->orig = orig;

	/* Initialize flags here so that refresh can also use __set_subwin. */
	for (lp = win->lspace, i = 0; i < win->maxy; i++, lp++)
		lp->flags = 0;
	__set_subwin(orig, win);
	return (win);
}
/*
 * This code is shared with mvwin().
 */
void
__set_subwin(WINDOW *orig, WINDOW *win)
{
	int     i;
	__LINE *lp, *olp;

	win->ch_off = win->begx - orig->begx;
	/* Point line pointers to line space. */
	for (lp = win->lspace, i = 0; i < win->maxy; i++, lp++) {
		win->lines[i] = lp;
		olp = orig->lines[i + win->begy - orig->begy];
		lp->line = &olp->line[win->ch_off];
		lp->firstchp = &olp->firstch;
		lp->lastchp = &olp->lastch;
		lp->hash = __hash((char *)(void *)lp->line,
		    (int) (win->maxx * __LDATASIZE));
	}

#ifdef DEBUG
	__CTRACE("__set_subwin: win->ch_off = %d\n", win->ch_off);
#endif
}
/*
 * __makenew --
 *	Set up a window buffer and returns a pointer to it.
 */
static WINDOW *
__makenew(int nlines, int ncols, int by, int bx, int sub)
{
	WINDOW			*win;
	__LINE			*lp;
	struct __winlist	*wlp, *wlp2;
	int			 i;


#ifdef	DEBUG
	__CTRACE("makenew: (%d, %d, %d, %d)\n", nlines, ncols, by, bx);
#endif
	if ((win = malloc(sizeof(*win))) == NULL)
		return (NULL);
#ifdef DEBUG
	__CTRACE("makenew: nlines = %d\n", nlines);
#endif

	/* Set up line pointer array and line space. */
	if ((win->lines = malloc(nlines * sizeof(__LINE *))) == NULL) {
		free(win);
		return NULL;
	}
	if ((win->lspace = malloc(nlines * sizeof(__LINE))) == NULL) {
		free(win);
		free(win->lines);
		return NULL;
	}
	/* Don't allocate window and line space if it's a subwindow */
	if (!sub) {
		/*
		 * Allocate window space in one chunk.
		 */
		if ((win->wspace =
			malloc(ncols * nlines * sizeof(__LDATA))) == NULL) {
			free(win->lines);
			free(win->lspace);
			free(win);
			return NULL;
		}
		/*
		 * Append window to window list.
		 */
		if ((wlp = malloc(sizeof(struct __winlist))) == NULL) {
			free(win->wspace);
			free(win->lines);
			free(win->lspace);
			free(win);
			return NULL;
		}
		wlp->winp = win;
		wlp->nextp = NULL;
		if (__winlistp == NULL)
			__winlistp = wlp;
		else {
			wlp2 = __winlistp;
			while (wlp2->nextp != NULL)
				wlp2 = wlp2->nextp;
			wlp2->nextp = wlp;
		}
		/*
		 * Point line pointers to line space, and lines themselves into
		 * window space.
		 */
		for (lp = win->lspace, i = 0; i < nlines; i++, lp++) {
			win->lines[i] = lp;
			lp->line = &win->wspace[i * ncols];
			lp->firstchp = &lp->firstch;
			lp->lastchp = &lp->lastch;
			lp->firstch = 0;
			lp->lastch = 0;
		}
	}
#ifdef DEBUG
	__CTRACE("makenew: ncols = %d\n", ncols);
#endif
	win->cury = win->curx = 0;
	win->maxy = nlines;
	win->maxx = ncols;

	win->begy = by;
	win->begx = bx;
	win->flags = 0;
	win->delay = -1;
	win->wattr = 0;
	win->bch = ' ';
	win->battr = 0;
	win->scr_t = 0;
	win->scr_b = win->maxy - 1;
	__swflags(win);
#ifdef DEBUG
	__CTRACE("makenew: win->wattr = %0.2o\n", win->wattr);
	__CTRACE("makenew: win->flags = %0.2o\n", win->flags);
	__CTRACE("makenew: win->maxy = %d\n", win->maxy);
	__CTRACE("makenew: win->maxx = %d\n", win->maxx);
	__CTRACE("makenew: win->begy = %d\n", win->begy);
	__CTRACE("makenew: win->begx = %d\n", win->begx);
	__CTRACE("makenew: win->scr_t = %d\n", win->scr_t);
	__CTRACE("makenew: win->scr_b = %d\n", win->scr_b);
#endif
	return (win);
}

void
__swflags(WINDOW *win)
{
	win->flags &= ~(__ENDLINE | __FULLWIN | __SCROLLWIN | __LEAVEOK);
	if (win->begx + win->maxx == COLS) {
		win->flags |= __ENDLINE;
		if (win->begx == 0 && win->maxy == LINES && win->begy == 0)
			win->flags |= __FULLWIN;
		if (win->begy + win->maxy == LINES)
			win->flags |= __SCROLLWIN;
	}
}
