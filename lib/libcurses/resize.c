/*	$NetBSD: resize.c,v 1.9 2003/07/30 11:11:55 dsl Exp $	*/

/*
 * Copyright (c) 2001
 *	Brett Lymn.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
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
static char sccsid[] = "@(#)resize.c   blymn 2001/08/26";
#else
__RCSID("$NetBSD: resize.c,v 1.9 2003/07/30 11:11:55 dsl Exp $");
#endif
#endif				/* not lint */

#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

static int __resizewin(WINDOW *win, int nlines, int ncols);

/*
 * wresize --
 *	Resize the given window to the new size.
 */
int
wresize(WINDOW *win, int req_nlines, int req_ncols)
{
	int	nlines = req_nlines;
	int	ncols = req_ncols;

	if (win == NULL)
		return ERR;

	nlines = req_nlines;
	ncols = req_ncols;
	if (win->orig == NULL) {
		/* bound window to screen */
		if (win->begy + nlines > LINES)
			nlines = 0;
		if (nlines <= 0)
			nlines += LINES - win->begy;
		if (win->begx + ncols > COLS)
			ncols = 0;
		if (ncols <= 0)
			ncols += COLS - win->begx;
	} else {
		/* subwins must fit inside the parent - check this */
		if (win->begy + nlines > win->orig->begy + win->orig->maxy)
			nlines = 0;
		if (nlines <= 0)
			nlines += win->orig->begy + win->orig->maxy - win->begy;
		if (win->begx + ncols > win->orig->begx + win->orig->maxx)
			ncols = 0;
		if (ncols <= 0)
			ncols += win->orig->begx + win->orig->maxx - win->begx;
	}

	if ((__resizewin(win, nlines, ncols)) == ERR)
		return ERR;

	win->reqy = req_nlines;
	win->reqx = req_ncols;

	return OK;
}

/*
 * resizeterm --
 *      Resize the terminal window, resizing the dependent windows.
 */
int
resizeterm(int nlines, int ncols)
{
	WINDOW *win;
	struct __winlist *list;
	int newlines, newcols;

	  /* don't worry if things have not changed... we would like to
	     do this but some bastard programs update LINES and COLS before
	     calling resizeterm thus negating it's effect.
	if ((nlines == LINES) && (ncols == COLS))
	return OK;*/

#ifdef	DEBUG
	__CTRACE("resizeterm: (%d, %d)\n", nlines, ncols);
#endif


	for (list = _cursesi_screen->winlistp; list != NULL; list = list->nextp) {
		win = list->winp;

		newlines = win->reqy;
		if (win->begy + newlines >= nlines)
			newlines = 0;
		if (newlines == 0)
			newlines = nlines - win->begy;

		newcols = win->reqx;
		if (win->begx + newcols >= ncols)
			newcols = 0;
		if (newcols == 0)
			newcols = ncols - win->begx;

		if (__resizewin(win, newlines, newcols) != OK)
			return ERR;
	}

	LINES = nlines;
	COLS = ncols;

	  /* tweak the flags now that we have updated the LINES and COLS */
	for (list = _cursesi_screen->winlistp; list != NULL; list = list->nextp) {
		if (!(win->flags & __ISPAD))
			__swflags(list->winp);
	}

	wrefresh(curscr);
	return OK;
}

/*
 * __resizewin --
 *	Resize the given window.
 */
static int
__resizewin(WINDOW *win, int nlines, int ncols)
{
	__LINE			*lp, *olp, **newlines, *newlspace;
	__LDATA			*sp;
	__LDATA                 *newwspace;
	int			 i, j;
	int			 y, x;
	WINDOW			*swin;

#ifdef	DEBUG
	__CTRACE("resize: (%p, %d, %d)\n", win, nlines, ncols);
	__CTRACE("resize: win->wattr = %08x\n", win->wattr);
	__CTRACE("resize: win->flags = %#.4x\n", win->flags);
	__CTRACE("resize: win->maxy = %d\n", win->maxy);
	__CTRACE("resize: win->maxx = %d\n", win->maxx);
	__CTRACE("resize: win->begy = %d\n", win->begy);
	__CTRACE("resize: win->begx = %d\n", win->begx);
	__CTRACE("resize: win->scr_t = %d\n", win->scr_t);
	__CTRACE("resize: win->scr_b = %d\n", win->scr_b);
#endif

	if (nlines <= 0 || ncols <= 0)
		nlines = ncols = 0;
	else {
		/* Reallocate line pointer array and line space. */
		newlines = realloc(win->lines, nlines * sizeof(__LINE *));
		if (newlines == NULL)
			return ERR;
		win->lines = newlines;

		newlspace = realloc(win->lspace, nlines * sizeof(__LINE));
		if (newlspace == NULL)
			return ERR;
		win->lspace = newlspace;
	}

	/* Don't allocate window and line space if it's a subwindow */
	if (win->orig == NULL) {
		/*
		 * Allocate window space in one chunk.
		 */
		if (ncols != 0) {
			newwspace = realloc(win->wspace,
					    ncols * nlines * sizeof(__LDATA));
			if (newwspace == NULL)
				return ERR;
			win->wspace = newwspace;
		}

		/*
		 * Point line pointers to line space, and lines themselves into
		 * window space.
		 */
		for (lp = win->lspace, i = 0; i < nlines; i++, lp++) {
			win->lines[i] = lp;
			lp->line = &win->wspace[i * ncols];
#ifdef DEBUG
			lp->sentinel = SENTINEL_VALUE;
#endif
			lp->firstchp = &lp->firstch;
			lp->lastchp = &lp->lastch;
			lp->firstch = 0;
			lp->lastch = ncols - 1;
			lp->flags = __ISDIRTY;
		}
	} else {

		win->ch_off = win->begx - win->orig->begx;
		  /* Point line pointers to line space. */
		for (lp = win->lspace, i = 0; i < nlines; i++, lp++) {
			win->lines[i] = lp;
			olp = win->orig->lines[i + win->begy
					      - win->orig->begy];
			lp->line = &olp->line[win->ch_off];
#ifdef DEBUG
			lp->sentinel = SENTINEL_VALUE;
#endif
			lp->firstchp = &olp->firstch;
			lp->lastchp = &olp->lastch;
			lp->flags = __ISDIRTY;
		}
	}


	win->cury = win->curx = 0;
	win->maxy = nlines;
	win->maxx = ncols;
	win->scr_b = win->maxy - 1;
	__swflags(win);

	  /*
	   * we must zot the window contents otherwise lines may pick
	   * up attributes from the previous line when the window is
	   * made smaller.  The client will redraw the window anyway
	   * so this is no big deal.
	   */
	for (i = 0; i < win->maxy; i++) {
		lp = win->lines[i];
		for (sp = lp->line, j = 0; j < win->maxx; j++, sp++) {
			sp->ch = ' ';
			sp->bch = ' ';
			sp->attr = 0;
			sp->battr = 0;
		}
		lp->hash = __hash((char *)(void *)lp->line,
				  (size_t) (ncols * __LDATASIZE));
	}

#ifdef DEBUG
	__CTRACE("resize: win->wattr = %08x\n", win->wattr);
	__CTRACE("resize: win->flags = %#.4x\n", win->flags);
	__CTRACE("resize: win->maxy = %d\n", win->maxy);
	__CTRACE("resize: win->maxx = %d\n", win->maxx);
	__CTRACE("resize: win->begy = %d\n", win->begy);
	__CTRACE("resize: win->begx = %d\n", win->begx);
	__CTRACE("resize: win->scr_t = %d\n", win->scr_t);
	__CTRACE("resize: win->scr_b = %d\n", win->scr_b);
#endif

	if (win->orig == NULL) {
		/* bound subwindows to new size and fixup their pointers */
		for (swin = win->nextp; swin != win; swin = swin->nextp) {
			y = swin->reqy;
			if (swin->begy + y > win->begy + win->maxy)
				y = 0;
			if (y <= 0)
				y += win->begy + win->maxy - swin->begy;
			x = swin->reqx;
			if (swin->begx + x > win->begx + win->maxx)
				x = 0;
			if (x <= 0)
				x += win->begy + win->maxx - swin->begx;
			__resizewin(swin, y, x);
		}
	}

	return OK;
}

