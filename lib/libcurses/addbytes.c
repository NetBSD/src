/*	$NetBSD: addbytes.c,v 1.62 2022/04/12 07:03:04 blymn Exp $	*/

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
static char sccsid[] = "@(#)addbytes.c	8.4 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: addbytes.c,v 1.62 2022/04/12 07:03:04 blymn Exp $");
#endif
#endif				/* not lint */

#include <stdlib.h>
#include <string.h>
#include "curses.h"
#include "curses_private.h"
#ifdef DEBUG
#include <assert.h>
#endif

#ifndef _CURSES_USE_MACROS

/*
 * addbytes --
 *      Add the characters to the current position in stdscr.
 */
int
addbytes(const char *bytes, int count)
{

	return _cursesi_waddbytes(stdscr, bytes, count, 0, 1);
}

/*
 * waddbytes --
 *      Add the characters to the current position in the given window.
 */
int
waddbytes(WINDOW *win, const char *bytes, int count)
{

	return _cursesi_waddbytes(win, bytes, count, 0, 1);
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

	return _cursesi_waddbytes(win, bytes, count, 0, 1);
}

#endif

int
__waddbytes(WINDOW *win, const char *bytes, int count, attr_t attr)
{

	return _cursesi_waddbytes(win, bytes, count, attr, 1);
}

/*
 * _cursesi_waddbytes --
 *	Add the characters to the current position in the given window.
 * if char_interp is non-zero then character interpretation is done on
 * the byte (i.e. \n to newline, \r to carriage return, \b to backspace
 * and so on).
 */
int
_cursesi_waddbytes(WINDOW *win, const char *bytes, int count, attr_t attr,
	    int char_interp)
{
	int		*py = &win->cury, *px = &win->curx, err;
	__LINE		*lp;
#ifdef HAVE_WCHAR
	int		n;
	cchar_t		cc;
	wchar_t		wc;
	mbstate_t	st;
#else
	int		c;
#endif
#ifdef DEBUG
	int             i;

	for (i = 0; i < win->maxy; i++) {
		assert(win->alines[i]->sentinel == SENTINEL_VALUE);
	}

	__CTRACE(__CTRACE_INPUT, "ADDBYTES: add %d bytes\n", count);
#endif

	err = OK;
	lp = win->alines[*py];

#ifdef HAVE_WCHAR
	(void)memset(&st, 0, sizeof(st));
#endif
	while (count > 0) {
#ifndef HAVE_WCHAR
		c = *bytes++;
		__CTRACE(__CTRACE_INPUT, "ADDBYTES('%c', %x) at (%d, %d)\n",
		    c, attr, *py, *px);
		err = _cursesi_addbyte(win, &lp, py, px, c, attr, char_interp);
		count--;
#else
		/*
		 * For wide-character support only, try to convert the
		 * given string into a wide character - we do this because
		 * this is how ncurses behaves (not that I think this is
		 * actually the correct thing to do but if we don't do it
		 * a lot of things that rely on this behaviour will break
		 * and we will be blamed).  If the conversion succeeds
		 * then we eat the n characters used to make the wide char
		 * from the string.
		 */
		n = (int)mbrtowc(&wc, bytes, (size_t)count, &st);
		if (n < 0) {
			/* not a valid conversion just eat a char */
			wc = *bytes;
			n = 1;
			(void)memset(&st, 0, sizeof(st));
		} else if (wc == 0) {
			break;
		}

		__CTRACE(__CTRACE_INPUT,
		    "ADDBYTES WIDE(0x%x [%s], %x) at (%d, %d), ate %d bytes\n",
		    (unsigned)wc, unctrl((unsigned)wc), attr, *py, *px, n);
		cc.vals[0] = wc;
		cc.elements = 1;
		cc.attributes = attr;
		err = _cursesi_addwchar(win, &lp, py, px, &cc, char_interp);
		bytes += n;
		count -= n;
#endif
	}

#ifdef DEBUG
	for (i = 0; i < win->maxy; i++) {
		assert(win->alines[i]->sentinel == SENTINEL_VALUE);
	}
#endif

	return (err);
}

/*
 * _cursesi_addbyte -
 *	Internal function to add a byte and update the row and column
 * positions as appropriate.  If char_interp is non-zero then
 * character interpretation is done on the byte.  This function is
 * only used in the narrow character version of curses.
 */
int
_cursesi_addbyte(WINDOW *win, __LINE **lp, int *y, int *x, int c,
		 attr_t attr, int char_interp)
{
	static char	 blank[] = " ";
	int		 tabsize;
	int		 newx, i, wcols;
	attr_t		 attributes;

	if (char_interp) {
		switch (c) {
		case '\t':
			tabsize = win->screen->TABSIZE;
			newx = tabsize - (*x % tabsize);
			/* if at the bottom of the window and
			   not allowed to scroll then just do
			   what we can */
			if ((*y == win->scr_b) &&
			    !(win->flags & __SCROLLOK)) {
				if ((*lp)->flags & __ISPASTEOL) {
					return OK;
				}

				if (*x + newx > win->maxx - 1)
					newx = win->maxx - *x - 1;
			}

			for (i = 0; i < newx; i++) {
				if (waddbytes(win, blank, 1) == ERR)
					return ERR;
			}
			return OK;

		case '\n':
			wclrtoeol(win);
			(*lp)->flags |= __ISPASTEOL;
			break;

		case '\r':
			*x = 0;
			return OK;

		case '\b':
			if (--(*x) < 0)
				*x = 0;
			return OK;
		}
	}

	__CTRACE(__CTRACE_INPUT, "ADDBYTES(%p, %d, %d)\n", win, *y, *x);

	if (char_interp && ((*lp)->flags & __ISPASTEOL)) {
		*x = 0;
		(*lp)->flags &= ~__ISPASTEOL;
		if (*y == win->scr_b) {
			__CTRACE(__CTRACE_INPUT,
			    "ADDBYTES - on bottom of scrolling region\n");
			if (!(win->flags & __SCROLLOK))
				return ERR;
			scroll(win);
		} else {
			(*y)++;
		}
		*lp = win->alines[*y];
		if (c == '\n')
			return OK;
	}

	__CTRACE(__CTRACE_INPUT,
	    "ADDBYTES: 1: y = %d, x = %d, firstch = %d, lastch = %d\n",
	    *y, *x, *win->alines[*y]->firstchp, *win->alines[*y]->lastchp);

	attributes = (win->wattr | attr) & (__ATTRIBUTES & ~__COLOR);
	if (attr & __COLOR)
		attributes |= attr & __COLOR;
	else if (win->wattr & __COLOR)
		attributes |= win->wattr & __COLOR;


	wcols = wcwidth(c);
	if (wcols < 0)
		wcols = 1;

	/*
	 * Always update the change pointers.  Otherwise,
	 * we could end up not displaying 'blank' characters
	 * when overlapping windows are displayed.
	 */
	newx = *x + win->ch_off;
	(*lp)->flags |= __ISDIRTY;
	/*
	 * firstchp/lastchp are shared between
	 * parent window and sub-window.
	 */
	if (newx < *(*lp)->firstchp)
		*(*lp)->firstchp = newx;
	if (newx > *(*lp)->lastchp)
		*(*lp)->lastchp = newx;
	__CTRACE(__CTRACE_INPUT, "ADDBYTES: change gives f/l: %d/%d [%d/%d]\n",
	    *(*lp)->firstchp, *(*lp)->lastchp,
	    *(*lp)->firstchp - win->ch_off,
	    *(*lp)->lastchp - win->ch_off);
	if (win->bch != ' ' && c == ' ') {
		(*lp)->line[*x].ch = win->bch;
#ifdef HAVE_CHAR
		(*lp)->line[*x].wcols = win->wcols;
#endif
	} else {
		(*lp)->line[*x].ch = c;
#ifdef HAVE_CHAR
		(*lp)->line[*x].wcols = wcols;
#endif
	}

	if (attributes & __COLOR)
		(*lp)->line[*x].attr =
			attributes | (win->battr & ~__COLOR);
	else
		(*lp)->line[*x].attr = attributes | win->battr;

	if (*x == win->maxx - 1)
		(*lp)->flags |= __ISPASTEOL;
	else
		(*x)++;

	__CTRACE(__CTRACE_INPUT,
	    "ADDBYTES: 2: y = %d, x = %d, firstch = %d, lastch = %d\n",
	    *y, *x, *win->alines[*y]->firstchp, *win->alines[*y]->lastchp);
	__sync(win);
	return OK;
}

/*
 * _cursesi_addwchar -
 *	Internal function to add a wide character and update the row
 * and column positions.
 */
int
_cursesi_addwchar(WINDOW *win, __LINE **lnp, int *y, int *x,
		  const cchar_t *wch, int char_interp)
{
#ifndef HAVE_WCHAR
	return ERR;
#else
	int sx = 0, ex = 0, cw = 0, i = 0, newx = 0, tabsize;
	__LDATA *lp = &win->alines[*y]->line[*x], *tp = NULL;
	nschar_t *np = NULL;
	cchar_t cc;
	attr_t attributes;

	if (char_interp) {
		/* special characters handling */
		switch (wch->vals[0]) {
		case L'\b':
			if (--*x < 0)
				*x = 0;
			return OK;
		case L'\r':
			*x = 0;
			return OK;
		case L'\n':
			if (*y == win->scr_b) {
				if (!(win->flags & __SCROLLOK))
					return ERR;
				wclrtoeol(win);
				scroll(win);
			} else {
				wclrtoeol(win);
				(*y)++;
			}
			*x = 0;
			(*lnp)->flags &= ~__ISPASTEOL;
			return OK;
		case L'\t':
			cc.vals[0] = L' ';
			cc.elements = 1;
			cc.attributes = win->wattr;
			tabsize = win->screen->TABSIZE;
			newx = tabsize - (*x % tabsize);

			/* if at the bottom of the window and
			   not allowed to scroll then just do
			   what we can */
			if ((*y == win->scr_b) &&
			    !(win->flags & __SCROLLOK)) {
				if ((*lnp)->flags & __ISPASTEOL) {
					return ERR;
				}

				if (*x + newx > win->maxx - 1)
					newx = win->maxx - *x - 1;
			}

			for (i = 0; i < newx; i++) {
				if (wadd_wch(win, &cc) == ERR)
					return ERR;
			}
			return OK;
		}
	}

	/* check for non-spacing character */
	if (!wcwidth(wch->vals[0])) {
		__CTRACE(__CTRACE_INPUT,
		    "_cursesi_addwchar: char '%c' is non-spacing\n",
		    wch->vals[0]);
		cw = (*lp).wcols;
		if (cw < 0) {
			lp += cw;
			*x += cw;
		}
		for (i = 0; i < wch->elements; i++) {
			if (!(np = (nschar_t *) malloc(sizeof(nschar_t))))
				return ERR;
			np->ch = wch->vals[i];
			np->next = lp->nsp;
			lp->nsp = np;
		}
		(*lnp)->flags |= __ISDIRTY;
		newx = *x + win->ch_off;
		if (newx < *(*lnp)->firstchp)
			*(*lnp)->firstchp = newx;
		if (newx > *(*lnp)->lastchp)
			*(*lnp)->lastchp = newx;
		__touchline(win, *y, *x, *x);
		return OK;
	}
	/* check for new line first */
	if (char_interp && ((*lnp)->flags & __ISPASTEOL)) {
		*x = 0;
		(*lnp)->flags &= ~__ISPASTEOL;
		if (*y == win->scr_b) {
			if (!(win->flags & __SCROLLOK))
				return ERR;
			scroll(win);
		} else {
			(*y)++;
		}
		(*lnp) = win->alines[*y];
		lp = &win->alines[*y]->line[*x];
	}
	/* clear out the current character */
	cw = (*lp).wcols;
	if (cw >= 0) {
		sx = *x;
	} else {
		for (sx = *x - 1; sx >= max(*x + cw, 0); sx--) {
			__CTRACE(__CTRACE_INPUT,
			    "_cursesi_addwchar: clear current char (%d,%d)\n",
			    *y, sx);
			tp = &win->alines[*y]->line[sx];
			tp->ch = win->bch;
			if (_cursesi_copy_nsp(win->bnsp, tp) == ERR)
				return ERR;

			tp->attr = win->battr;
			tp->wcols = win->wcols;
		}
		sx = *x + cw;
		(*lnp)->flags |= __ISDIRTY;
		newx = sx + win->ch_off;
		if (newx < *(*lnp)->firstchp)
			*(*lnp)->firstchp = newx;
	}

	/* check for enough space before the end of line */
	cw = wcwidth(wch->vals[0]);
	if (cw < 0)
		cw = 1;

	if (cw > win->maxx - *x) {
		__CTRACE(__CTRACE_INPUT,
		    "_cursesi_addwchar: clear EOL (%d,%d)\n", *y, *x);
		if (*y == win->scr_b) {
			if (!(win->flags & __SCROLLOK))
				return ERR;
			scroll(win);
		}

		(*lnp)->flags |= __ISDIRTY;
		newx = *x + win->ch_off;
		if (newx < *(*lnp)->firstchp)
			*(*lnp)->firstchp = newx;
		for (tp = lp; *x < win->maxx; tp++, (*x)++) {
			tp->ch = win->bch;
			if (_cursesi_copy_nsp(win->bnsp, tp) == ERR)
				return ERR;
			tp->attr = win->battr;
			tp->wcols = win->wcols;
		}
		newx = win->maxx - 1 + win->ch_off;
		if (newx > *(*lnp)->lastchp)
			*(*lnp)->lastchp = newx;
		__touchline(win, *y, sx, (int) win->maxx - 1);
		sx = *x = 0;
		if (*y != win->scr_b) {
			(*y)++;
		}
		lp = &win->alines[*y]->line[0];
		(*lnp) = win->alines[*y];
	}

	/* add spacing character */
	__CTRACE(__CTRACE_INPUT,
	    "_cursesi_addwchar: add character (%d,%d) 0x%x\n",
	    *y, *x, wch->vals[0]);
	(*lnp)->flags |= __ISDIRTY;
	newx = *x + win->ch_off;
	if (newx < *(*lnp)->firstchp)
		*(*lnp)->firstchp = newx;
	if (lp->nsp) {
		__cursesi_free_nsp(lp->nsp);
		lp->nsp = NULL;
	}

	lp->ch = wch->vals[0];

	attributes = (win->wattr | wch->attributes)
		& (WA_ATTRIBUTES & ~__COLOR);
	if (wch->attributes & __COLOR)
		attributes |= wch->attributes & __COLOR;
	else if (win->wattr & __COLOR)
		attributes |= win->wattr & __COLOR;
	if (attributes & __COLOR)
		lp->attr = attributes | (win->battr & ~__COLOR);
	else
		lp->attr = attributes | win->battr;

	lp->wcols = cw;

	__CTRACE(__CTRACE_INPUT,
	    "_cursesi_addwchar: add spacing char 0x%x, attr 0x%x, width %d\n",
	    lp->ch, lp->attr, lp->wcols);

	if (wch->elements > 1) {
		for (i = 1; i < wch->elements; i++) {
			np = malloc(sizeof(nschar_t));
			if (!np)
				return ERR;
			np->ch = wch->vals[i];
			np->next = lp->nsp;
			__CTRACE(__CTRACE_INPUT,
			    "_cursesi_addwchar: add non-spacing char 0x%x\n",
			    np->ch);
			lp->nsp = np;
		}
	}
	__CTRACE(__CTRACE_INPUT,
	    "_cursesi_addwchar: non-spacing list header: %p\n", lp->nsp);
	__CTRACE(__CTRACE_INPUT,
	    "_cursesi_addwchar: add rest columns (%d:%d)\n",
	    sx + 1, sx + cw - 1);
	__CTRACE(__CTRACE_INPUT, "_cursesi_addwchar: *x = %d, win->maxx = %d\n",
	    *x, win->maxx);
	for (tp = lp + 1, *x = sx + 1; *x - sx <= cw - 1; tp++, (*x)++) {
		if (tp->nsp) {
			__cursesi_free_nsp(tp->nsp);
			tp->nsp = NULL;
		}
		tp->ch = wch->vals[0];
		tp->attr = lp->attr & WA_ATTRIBUTES;
		/* Mark as "continuation" cell */
		tp->wflags |= WCA_CONTINUATION;
	}


	if (*x >= win->maxx) {
		__CTRACE(__CTRACE_INPUT, "_cursesi_addwchar: do line wrap\n");
		if (*y == win->scr_b) {
			__CTRACE(__CTRACE_INPUT,
			    "_cursesi_addwchar: at bottom of screen\n");
			if (!(win->flags & __SCROLLOK))
				return ERR;
			__CTRACE(__CTRACE_INPUT,
			    "_cursesi_addwchar: do a scroll\n");
			scroll(win);
		}
		newx = win->maxx - 1 + win->ch_off;
		if (newx > *(*lnp)->lastchp)
			*(*lnp)->lastchp = newx;
		__touchline(win, *y, sx, (int) win->maxx - 1);
		*x = sx = 0;
		if (*y != win->scr_b) {
			(*y)++;
		}
		lp = &win->alines[*y]->line[0];
		(*lnp) = win->alines[*y];
	} else {

		/* clear the remaining of the current character */
		if (*x && *x < win->maxx) {
			ex = sx + cw;
			tp = &win->alines[*y]->line[ex];
			while (ex < win->maxx && tp->wcols < 0) {
				__CTRACE(__CTRACE_INPUT,
				    "_cursesi_addwchar: clear "
				    "remaining of current char (%d,%d)nn",
				    *y, ex);
				tp->ch = win->bch;
				if (_cursesi_copy_nsp(win->bnsp, tp) == ERR)
					return ERR;
				tp->attr = win->battr;
				tp->wcols = win->wcols;
				tp++, ex++;
			}
			newx = ex - 1 + win->ch_off;
			if (newx > *(*lnp)->lastchp)
				*(*lnp)->lastchp = newx;
			__touchline(win, *y, sx, ex - 1);
		}
	}

	__CTRACE(__CTRACE_INPUT, "_cursesi_addwchar: %d : 0x%x\n",
	    lp->ch, lp->attr);
	__CTRACE(__CTRACE_INPUT,
	    "_cursesi_addwchar: *x = %d, *y = %d, win->maxx = %d\n",
	    *x, *y, win->maxx);
	__sync(win);
	return OK;
#endif
}
