/*   $NetBSD: add_wch.c,v 1.1 2007/01/21 11:38:59 blymn Exp $ */

/*
 * Copyright (c) 2005 The NetBSD Foundation Inc.
 * All rights reserved.
 *
 * This code is derived from code donated to the NetBSD Foundation
 * by Ruibiao Qiu <ruibiao@arl.wustl.edu,ruibiao@gmail.com>.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
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
__RCSID("$NetBSD: add_wch.c,v 1.1 2007/01/21 11:38:59 blymn Exp $");
#endif /* not lint */

#include <stdlib.h>
#include "curses.h"
#include "curses_private.h"

/*
 * add_wch --
 *	Add the wide character to the current position in stdscr.
 *
 */
int
add_wch(const cchar_t *wch)
{
#ifndef HAVE_WCHAR
	return ERR;
#else
	return wadd_wch(stdscr, wch);
#endif /* HAVE_WCHAR */
}


/*
 * mvadd_wch --
 *      Add the wide character to stdscr at the given location.
 */
int
mvadd_wch(int y, int x, const cchar_t *wch)
{
#ifndef HAVE_WCHAR
	return ERR;
#else
	return mvwadd_wch(stdscr, y, x, wch);
#endif /* HAVE_WCHAR */
}


/*
 * mvwadd_wch --
 *      Add the character to the given window at the given location.
 */
int
mvwadd_wch(WINDOW *win, int y, int x, const cchar_t *wch)
{
#ifndef HAVE_WCHAR
	return ERR;
#else
	if (wmove(win, y, x) == ERR)
		return ERR;

	return wadd_wch(win, wch);
#endif /* HAVE_WCHAR */
}


/*
 * wadd_wch --
 *	Add the wide character to the current position in the
 *	given window.
 *
 */
int
wadd_wch(WINDOW *win, const cchar_t *wch)
{
#ifndef HAVE_WCHAR
	return ERR;
#else
	int x = win->curx, y = win->cury, sx = 0, ex = 0, cw = 0, i = 0,
	    newx = 0;
	__LDATA *lp = &win->lines[y]->line[x], *tp = NULL;
	nschar_t *np = NULL, *tnp = NULL;
	__LINE *lnp = NULL;
	cchar_t cc;

#ifdef DEBUG
	for (i = 0; i < win->maxy; i++) {
		assert(win->lines[i]->sentinel == SENTINEL_VALUE);
	}
	__CTRACE("[wadd_wch]win(%p)", win);
#endif

	/* special characters handling */
	lnp = win->lines[y];
	switch (wch->vals[0]) {
	case L'\b':
		if (--x < 0)
			x = 0;
		win->curx = x;
		return OK;
	case L'\r':
		win->curx = 0;
		return OK;
	case L'\n':
		wclrtoeol(win);
		x = win->curx;
		y = win->cury;
		x = 0;
		lnp->flags &= ~__ISPASTEOL;
		if (y == win->scr_b) {
			if (!(win->flags & __SCROLLOK))
				return ERR;
			win->cury = y;
			win->curx = x;
			scroll(win);
			y = win->cury;
			x = win->curx;
		} else {
			y++;
		}
		win->curx = x;
		win->cury = y;
		return OK;
	case L'\t':
		cc.vals[0] = L' ';
		cc.elements = 1;
		cc.attributes = win->wattr;
		for (i = 0; i < 8 - (x % 8); i++) {
			if (wadd_wch(win, &cc) == ERR)
				return ERR;
		}
		return OK;
	}

	/* check for non-spacing character */
	if (!wcwidth(wch->vals[0])) {
		cw = WCOL(*lp);
		if (cw < 0) {
			lp += cw;
			x += cw;
		}
		for (i = 0; i < wch->elements; i++) {
			if (!(np = (nschar_t *) malloc(sizeof(nschar_t))))
				return ERR;;
			np->ch = wch->vals[i];
			np->next = lp->nsp;
			lp->nsp = np;
		}
		lnp->flags |= __ISDIRTY;
		newx = x + win->ch_off;
		if (newx < *lnp->firstchp)
			*lnp->firstchp = newx;
		if (newx > *lnp->lastchp)
			*lnp->lastchp = newx;
		__touchline(win, y, x, x);
		return OK;
	}
	/* check for new line first */
	if (lnp->flags & __ISPASTEOL) {
		x = 0;
		lnp->flags &= ~__ISPASTEOL;
		if (y == win->scr_b) {
			if (!(win->flags & __SCROLLOK))
				return ERR;
			win->cury = y;
			win->curx = x;
			scroll(win);
			y = win->cury;
			x = win->curx;
		} else {
			y++;
		}
		lnp = win->lines[y];
		lp = &win->lines[y]->line[x];
	}
	/* clear out the current character */
	cw = WCOL(*lp);
	if (cw >= 0) {
		sx = x;
	} else {
		for (sx = x - 1; sx >= max(x + cw, 0); sx--) {
#ifdef DEBUG
			__CTRACE("[wadd_wch]clear current char (%d,%d)",
				 y, sx);
#endif /* DEBUG */
			tp = &win->lines[y]->line[sx];
			tp->ch = (wchar_t) btowc((int) win->bch);
			if (_cursesi_copy_nsp(win->bnsp, tp) == ERR)
				return ERR;

			tp->attr = win->battr;
			SET_WCOL(*tp, 1);
		}
		sx = x + cw;
		lnp->flags |= __ISDIRTY;
		newx = sx + win->ch_off;
		if (newx < *lnp->firstchp)
			*lnp->firstchp = newx;
	}

	/* check for enough space before the end of line */
	cw = wcwidth(wch->vals[0]);
	if (cw > win->maxx - x) {
#ifdef DEBUG
		__CTRACE("[wadd_wch]clear EOL (%d,%d)", y, x);
#endif /* DEBUG */
		lnp->flags |= __ISDIRTY;
		newx = x + win->ch_off;
		if (newx < *lnp->firstchp)
			*lnp->firstchp = newx;
		for (tp = lp; x < win->maxx; tp++, x++) {
			tp->ch = (wchar_t) btowc((int) win->bch);
			if (_cursesi_copy_nsp(win->bnsp, tp) == ERR)
				return ERR;
			tp->attr = win->battr;
			SET_WCOL(*tp, 1);
		}
		newx = win->maxx - 1 + win->ch_off;
		if (newx > *lnp->lastchp)
			*lnp->lastchp = newx;
		__touchline(win, y, sx, (int) win->maxx - 1);
		sx = x = 0;
		if (y == win->scr_b) {
			if (!(win->flags & __SCROLLOK))
				return ERR;
			win->curx = x;
			win->cury = y;
			scroll(win);
			x = win->curx;
			y = win->cury;
		} else {
			y++;
		}
		lp = &win->lines[y]->line[0];
		lnp = win->lines[y];
	}
	win->cury = y;

	/* add spacing character */
#ifdef DEBUG
	__CTRACE("[wadd_wch]add character (%d,%d)%x",
		y, x, wch->vals[0]);
#endif /* DEBUG */
	lnp->flags |= __ISDIRTY;
	newx = x + win->ch_off;
	if (newx < *lnp->firstchp)
		*lnp->firstchp = newx;
	if (lp->nsp) {
		np = lp->nsp;
		while (np) {
			tnp = np->next;
			free(np);
			np = tnp;
		}
		lp->nsp = NULL;
	}
	lp->ch = wch->vals[0];
	lp->attr = wch->attributes & WA_ATTRIBUTES;
	SET_WCOL(*lp, cw);
	if (wch->elements > 1) {
		for (i = 1; i < wch->elements; i++) {
			np = (nschar_t *)malloc(sizeof(nschar_t));
			if (!np)
				return ERR;;
			np->ch = wch->vals[i];
			np->next = lp->nsp;
#ifdef DEBUG
			__CTRACE("[wadd_wch]add non-spacing char 0x%x",
			    np->ch);
#endif /* DEBUG */
			lp->nsp = np;
		}
	}
#ifdef DEBUG
	__CTRACE("[wadd_wch]non-spacing list header: %p\n", lp->nsp);
	__CTRACE("[wadd_wch]add rest columns (%d:%d)\n",
		sx + 1, sx + cw - 1);
#endif /* DEBUG */
	for (tp = lp + 1, x = sx + 1; x - sx <= cw - 1; tp++, x++) {
		if (tp->nsp) {
			np = tp->nsp;
			while (np) {
				tnp = np->next;
				free(np);
				np = tnp;
			}
			tp->nsp = NULL;
		}
		tp->ch = wch->vals[0];
		tp->attr = wch->attributes & WA_ATTRIBUTES;
		SET_WCOL(*tp, sx - x);
	}
	if (x == win->maxx) {
		lnp->flags |= __ISPASTEOL;
		newx = win->maxx - 1 + win->ch_off;
		if (newx > *lnp->lastchp)
			*lnp->lastchp = newx;
		__touchline(win, y, sx, (int) win->maxx - 1);
		win->curx = sx;
	} else {
		win->curx = x;

		/* clear the remining of the current characer */
		if (x && x < win->maxx) {
			ex = sx + cw;
			tp = &win->lines[y]->line[ex];
			while (ex < win->maxx && WCOL(*tp) < 0) {
#ifdef DEBUG
				__CTRACE("[wadd_wch]clear remaining of current char (%d,%d)",
				    y, ex);
#endif /* DEBUG */
				tp->ch = (wchar_t) btowc((int) win->bch);
				if (_cursesi_copy_nsp(win->bnsp, tp) == ERR)
					return ERR;
				tp->attr = win->battr;
				SET_WCOL(*tp, 1);
				tp++, ex++;
			}
			newx = ex - 1 + win->ch_off;
			if (newx > *lnp->lastchp)
				*lnp->lastchp = newx;
			__touchline(win, y, sx, ex - 1);
		}
	}

#ifdef DEBUG
	__CTRACE("add_wch: %d : 0x%x\n", lp->ch, lp->attr);
#endif /* DEBUG */
	return OK;
#endif /* HAVE_WCHAR */
}
