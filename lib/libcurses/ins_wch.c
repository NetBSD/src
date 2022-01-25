/*   $NetBSD: ins_wch.c,v 1.18 2022/01/25 03:05:06 blymn Exp $ */

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
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the NetBSD Foundation nor the names of its
 *	contributors may be used to endorse or promote products derived
 *	from this software without specific prior written permission.
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
__RCSID("$NetBSD: ins_wch.c,v 1.18 2022/01/25 03:05:06 blymn Exp $");
#endif						  /* not lint */

#include <string.h>
#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

/*
 * ins_wch --
 *	Do an insert-char on the line, leaving (cury, curx) unchanged.
 */
int
ins_wch(const cchar_t *wch)
{
	return wins_wch(stdscr, wch);
}

/*
 * mvins_wch --
 *	  Do an insert-char on the line at (y, x).
 */
int
mvins_wch(int y, int x, const cchar_t *wch)
{
	return mvwins_wch(stdscr, y, x, wch);
}

/*
 * mvwins_wch --
 *	  Do an insert-char on the line at (y, x) in the given window.
 */
int
mvwins_wch(WINDOW *win, int y, int x, const cchar_t *wch)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return wins_wch(win, wch);
}

/*
 * wins_wch --
 *	Do an insert-char on the line, leaving (cury, curx) unchanged.
 */
int
wins_wch(WINDOW *win, const cchar_t *wch)
{
	__LDATA	*start, *temp1, *temp2;
	__LINE *lnp;
	int cw, pcw, x, y, sx, ex, newx, i;
	nschar_t *np, *tnp;
	wchar_t ws[] = L"		";

	/* check for non-spacing characters */
	if (!wch)
		return OK;
	cw = wcwidth(wch->vals[0]);
	__CTRACE(__CTRACE_INPUT, "wins_wch: wcwidth %d\n", cw);
	if (cw < 0)
		cw = 1;
	if (!cw)
		return wadd_wch( win, wch );

	x = win->curx;
	y = win->cury;
	__CTRACE(__CTRACE_INPUT, "wins_wch: (%d,%d)\n", y, x);
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
			if (y == win->scr_b) {
				if (!(win->flags & __SCROLLOK))
					return ERR;
				scroll(win);
			}
			return OK;
		case L'\t':
			if (wins_nwstr(win, ws, min(win->maxx - x,
			    TABSIZE - (x % TABSIZE))) == ERR)
				return ERR;
			return OK;
	}

	/* locate current cell */
	x = win->curx;
	y = win->cury;
	lnp = win->alines[y];
	start = &win->alines[y]->line[x];
	sx = x;
	pcw = start->wcols;
	if (pcw < 0) {
		start += pcw;
		sx += pcw;
	}
	if (cw > win->maxx - sx)
		return ERR;
	lnp->flags |= __ISDIRTY;
	newx = sx + win->ch_off;
	if (newx < *lnp->firstchp)
		*lnp->firstchp = newx;

	/* shift all complete characters */
	__CTRACE(__CTRACE_INPUT, "wins_wch: shift all characters\n");
	temp1 = &win->alines[y]->line[win->maxx - 1];
	temp2 = temp1 - cw;
	pcw = (temp2 + 1)->wcols;
	if (pcw < 0) {
		__CTRACE(__CTRACE_INPUT, "wins_wch: clear EOL\n");
		temp2 += pcw;
		while (temp1 > temp2 + cw) {
			np = temp1->nsp;
			if (np) {
				while (np) {
					tnp = np->next;
					free(np);
					np = tnp;
				}
				temp1->nsp = NULL;
			}
			temp1->ch = (wchar_t)btowc((int)win->bch );
			if (_cursesi_copy_nsp(win->bnsp, temp1) == ERR)
				return ERR;
			temp1->attr = win->battr;
			temp1->wcols = 1;
			temp1--;
		}
	}
	while (temp2 >= start) {
		(void)memcpy(temp1, temp2, sizeof(__LDATA));
		temp1--, temp2--;
	}

	/* update character under cursor */
	start->nsp = NULL;
	start->ch = wch->vals[0];
	start->attr = wch->attributes & WA_ATTRIBUTES;
	start->wcols = cw;
	if (wch->elements > 1) {
		for (i = 1; i < wch->elements; i++) {
			np = malloc(sizeof(nschar_t));
			if (!np)
				return ERR;
			np->ch = wch->vals[i];
			np->next = start->nsp;
			start->nsp = np;
		}
	}
	__CTRACE(__CTRACE_INPUT, "wins_wch: insert (%x,%x,%p)\n",
	    start->ch, start->attr, start->nsp);
	temp1 = start + 1;
	ex = x + 1;
	while (ex - x < cw) {
		temp1->ch = wch->vals[0];
		temp1->wcols = x - ex;
		temp1->nsp = NULL;
		ex++, temp1++;
	}

	newx = win->maxx - 1 + win->ch_off;
	if (newx > *lnp->lastchp)
		*lnp->lastchp = newx;
	__touchline(win, y, sx, (int)win->maxx - 1);
	__sync(win);
	return OK;
}
