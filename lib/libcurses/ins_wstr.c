/*   $NetBSD: ins_wstr.c,v 1.22 2022/01/25 03:05:06 blymn Exp $ */

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
__RCSID("$NetBSD: ins_wstr.c,v 1.22 2022/01/25 03:05:06 blymn Exp $");
#endif						  /* not lint */

#include <string.h>
#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

/*
 * ins_wstr --
 *	insert a multi-character wide-character string into the current window
 */
int
ins_wstr(const wchar_t *wstr)
{
	return wins_wstr(stdscr, wstr);
}

/*
 * ins_nwstr --
 *	insert a multi-character wide-character string into the current window
 *	with at most n characters
 */
int
ins_nwstr(const wchar_t *wstr, int n)
{
	return wins_nwstr(stdscr, wstr, n);
}

/*
 * mvins_wstr --
 *	  Do an insert-string on the line at (y, x).
 */
int
mvins_wstr(int y, int x, const wchar_t *wstr)
{
	return mvwins_wstr(stdscr, y, x, wstr);
}

/*
 * mvins_nwstr --
 *	  Do an insert-n-string on the line at (y, x).
 */
int
mvins_nwstr(int y, int x, const wchar_t *wstr, int n)
{
	return mvwins_nwstr(stdscr, y, x, wstr, n);
}

/*
 * mvwins_wstr --
 *	  Do an insert-string on the line at (y, x) in the given window.
 */
int
mvwins_wstr(WINDOW *win, int y, int x, const wchar_t *wstr)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return wins_wstr(win, wstr);
}

/*
 * mvwins_nwstr --
 *	  Do an insert-n-string on the line at (y, x) in the given window.
 */
int
mvwins_nwstr(WINDOW *win, int y, int x, const wchar_t *wstr, int n)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return wins_nwstr(win, wstr, n);
}


/*
 * wins_wstr --
 *	Do an insert-string on the line, leaving (cury, curx) unchanged.
 */
int
wins_wstr(WINDOW *win, const wchar_t *wstr)
{
	return wins_nwstr(win, wstr, -1);
}

/*
 * wins_nwstr --
 *	Do an insert-n-string on the line, leaving (cury, curx) unchanged.
 */
int
wins_nwstr(WINDOW *win, const wchar_t *wstr, int n)
{
	__LDATA	 *start, *temp1, *temp2;
	__LINE	  *lnp;
	const wchar_t *scp;
	cchar_t cc;
	wchar_t *lstr, *slstr;
	int width, len, lx, sx, x, y, tx, ty, cw, pcw, newx, tn, w;
	wchar_t ws[] = L"		";

	/* check for leading non-spacing character */
	if (!wstr)
		return OK;
	cw = wcwidth(*wstr);
	if (cw < 0)
		cw = 1;
	if (!cw)
		return ERR;

	lstr = malloc(sizeof(wchar_t) * win->maxx);
	if (lstr == NULL)
		return ERR;

	scp = wstr + 1;
	width = cw;
	len = 1;
	n--;
	y = win->cury;
	x = win->curx;
	tn = n;

	/*
	 * Firstly, make sure the string will fit...
	 */
	while (*scp) {
		if (!tn)
			break;
		switch (*scp) {
			case L'\b':
				if (--x < 0)
					x = 0;
				cw = wcwidth(*(scp - 1));
				if (cw < 0)
					cw = 1;
				width -= cw;
				scp++;
				continue;

			case L'\r':
				width = 0;
				x = 0;
				scp++;
				continue;

			case L'\n':
				if (y == win->scr_b) {
					if (!(win->flags & __SCROLLOK)) {
						free(lstr);
						return ERR;
					}
				}
				y++;
				scp++;
				continue;

			case L'\t':
				cw = wcwidth(ws[0]);
				if (cw < 0)
					cw = 1;
				w = cw * (TABSIZE - (x % TABSIZE));
				width += w;
				x += w;
				scp++;
				continue;
		}
		w = wcwidth(*scp);
		if (w < 0)
			w = 1;
		tn--, width += w;
		scp++;
	}
	__CTRACE(__CTRACE_INPUT, "wins_nwstr: width=%d,len=%d\n", width, len);

	if (width > win->maxx - win->curx + 1) {
		free(lstr);
		return ERR;
	}

	scp = wstr;
	x = win->curx;
	y = win->cury;
	len = 0;
	width = 0;
	slstr = lstr;

	while (*scp && n) {
		lstr = slstr;
		lx = x;
		while (*scp) {
			if (!n)
				break;
			switch (*scp) {
				case L'\b':
					if (--x < 0)
						x = 0;
					if (--len < 0)
						len = 0;
					cw = wcwidth(*(scp - 1));
					if (cw < 0)
						cw = 1;
					width -= cw;
					scp++;
					if (lstr != slstr)
						lstr--;
					continue;

				case L'\r':
					width = 0;
					len = 0;
					x = 0;
					scp++;
					lstr = slstr;
					continue;

				case L'\n':
					goto loopdone;

				case L'\t':
					cw = wcwidth(ws[0]);
					if (cw < 0)
						cw = 1;
					w = cw * (TABSIZE - (x % TABSIZE));
					width += w;
					x += w;
					len += w;
					scp++;
					*lstr = *ws;
					lstr++;
					continue;
			}
			w = wcwidth(*scp);
			if (w < 0)
				w = 1;
			*lstr = *scp;
			n--, len++, width += w;
			scp++, lstr++;
		}

loopdone:
		start = &win->alines[y]->line[x];
		sx = x;
		lnp = win->alines[y];
		pcw = start->wcols;
		if (pcw < 0) {
			sx += pcw;
			start += pcw;
		}
		__CTRACE(__CTRACE_INPUT, "wins_nwstr: start@(%d)\n", sx);
		pcw = start->wcols;
		lnp->flags |= __ISDIRTY;
		newx = sx + win->ch_off;
		if (newx < *lnp->firstchp)
			*lnp->firstchp = newx;
#ifdef DEBUG
		{
			int i;

			__CTRACE(__CTRACE_INPUT, "========before=======\n");
			for (i = 0; i < win->maxx; i++)
			__CTRACE(__CTRACE_INPUT,
				    "wins_nwstr: (%d,%d)=(%x,%x,%p)\n",
				    y, i, win->alines[y]->line[i].ch,
				    win->alines[y]->line[i].attr,
				    win->alines[y]->line[i].nsp);
		}
#endif /* DEBUG */

		/* shift all complete characters */
		if (sx + width + pcw <= win->maxx) {
			__CTRACE(__CTRACE_INPUT, "wins_nwstr: shift all characters by %d\n", width);
			temp1 = &win->alines[y]->line[win->maxx - 1];
			temp2 = temp1 - width;
			pcw = (temp2 + 1)->wcols;
			if (pcw < 0) {
				__CTRACE(__CTRACE_INPUT,
				    "wins_nwstr: clear from %d to EOL(%d)\n",
				    win->maxx + pcw, win->maxx - 1);
				temp2 += pcw;
				while (temp1 > temp2 + width) {
					temp1->ch = (wchar_t)btowc((int) win->bch);
					if (_cursesi_copy_nsp(win->bnsp, temp1) == ERR) {
						free(lstr);
						return ERR;
					}
					temp1->attr = win->battr;
					temp1->wcols = 1;
					__CTRACE(__CTRACE_INPUT,
					    "wins_nwstr: empty cell(%p)\n", temp1);
					temp1--;
				}
			}
			while (temp2 >= start) {
				(void)memcpy(temp1, temp2, sizeof(__LDATA));
				temp1--, temp2--;
			}
#ifdef DEBUG
			{
				int i;

				__CTRACE(__CTRACE_INPUT, "=====after shift====\n");
				for (i = 0; i < win->maxx; i++)
					__CTRACE(__CTRACE_INPUT,
					    "wins_nwstr: (%d,%d)=(%x,%x,%p)\n",
					    y, i,
					    win->alines[y]->line[i].ch,
					    win->alines[y]->line[i].attr,
					    win->alines[y]->line[i].nsp);
				__CTRACE(__CTRACE_INPUT, "=====lstr====\n");
				for (i = 0; i < len; i++)
					__CTRACE(__CTRACE_INPUT,
					    "wins_nwstr: lstr[%d]= %x,\n",
					    i, (unsigned) slstr[i]);
			}
#endif /* DEBUG */
		}

		/* update string columns */
		x = lx;
		for (lstr = slstr, temp1 = start; len; len--, lstr++) {
			lnp = win->alines[y];
			cc.vals[0] = *lstr;
			cc.elements = 1;
			cc.attributes = win->wattr;
			_cursesi_addwchar(win, &lnp, &y, &x, &cc, 0);
		}

#ifdef DEBUG
		{
			int i;

			__CTRACE(__CTRACE_INPUT, "lx = %d, x = %x\n", lx, x);
			__CTRACE(__CTRACE_INPUT, "========after=======\n");
			for (i = 0; i < win->maxx; i++)
				__CTRACE(__CTRACE_INPUT,
				    "wins_nwstr: (%d,%d)=(%x,%x,%p)\n",
				    y, i,
				    win->alines[y]->line[i].ch,
				    win->alines[y]->line[i].attr,
				    win->alines[y]->line[i].nsp);
		}
#endif /* DEBUG */

		__touchline(win, (int) y, lx, (int) win->maxx - 1);

		/*
		 * Handle the newline here - we don't need to check
		 * if we are allowed to scroll because this was checked
		 * already.
		 */
		if (*scp == '\n') {
			tx = win->curx;
			ty = win->cury;
			win->curx = x;
			win->cury = y;
			wclrtoeol(win);
			win->curx = tx;
			win->cury = ty;
			if (y == win->scr_b)
				scroll(win);
			else
				y++;
			scp++;
			
		}

		newx = win->maxx - 1 + win->ch_off;
		if (newx > *lnp->lastchp)
			*lnp->lastchp = newx;
	}
	free(lstr);
	__sync(win);
	return OK;
}
