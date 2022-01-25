/*   $NetBSD: in_wchstr.c,v 1.10 2022/01/25 03:05:06 blymn Exp $ */

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
__RCSID("$NetBSD: in_wchstr.c,v 1.10 2022/01/25 03:05:06 blymn Exp $");
#endif						  /* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * in_wchstr, in_wchnstr --
 *	Return an array of wide characters at cursor position from stdscr.
 */
__warn_references(in_wchstr,
	"warning: this program uses in_wchstr(), which is unsafe.")
int
in_wchstr(cchar_t *wchstr)
{
	return win_wchstr(stdscr, wchstr);
}

int
in_wchnstr(cchar_t *wchstr, int n)
{
	return win_wchnstr(stdscr, wchstr, n);
}

/*
 * mvin_wchstr, mv_winchnstr --
 *  Return an array of wide characters at position (y, x) from stdscr.
 */
__warn_references(mvin_wchstr,
	"warning: this program uses mvin_wchstr(), which is unsafe.")
int
mvin_wchstr(int y, int x, cchar_t *wchstr)
{
	return mvwin_wchstr(stdscr, y, x, wchstr);
}

int
mvin_wchnstr(int y, int x, cchar_t *wchstr, int n)
{
	return mvwin_wchnstr(stdscr, y, x, wchstr, n);
}

/*
 * mvwin_wchstr, mvwin_wchnstr --
 *  Return an array wide characters at position (y, x) from the given window.
 */
__warn_references(mvwin_wchstr,
	"warning: this program uses mvwin_wchstr(), which is unsafe.")
int
mvwin_wchstr(WINDOW *win, int y, int x, cchar_t *wchstr)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return win_wchstr(win, wchstr);
}

int
mvwin_wchnstr(WINDOW *win, int y, int x, cchar_t *wchstr, int n)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return win_wchnstr(win, wchstr, n);
}

/*
 * win_wchstr, win_wchnstr --
 *	Return an array of characters at cursor position.
 */
__warn_references(win_wchstr,
	"warning: this program uses win_wchstr(), which is unsafe.")
int
win_wchstr(WINDOW *win, cchar_t *wchstr)
{

	return win_wchnstr(win, wchstr, -1);
}

/*
 * - SUSv2/xcurses doesn't document whether the trailing 0 is included
 *   in the length count or not.  For safety's sake it _is_ included.
 */
int
win_wchnstr(WINDOW *win, cchar_t *wchstr, int n)
{
	__LDATA	*start;
	int x = 0, cw = 0, cnt = 0;
	cchar_t *wcp;
	nschar_t *np;

	if (wchstr == NULL)
		return ERR;

	start = &win->alines[win->cury]->line[win->curx];
	x = win->curx;
	cw = start->wcols;
	if (cw < 0) {
		start += cw;
		x += cw;
	}
	wcp = wchstr;
	/* (n - 1) to leave room for the trailing 0 element */
	while ((x < win->maxx) && ((n < 0) || ((n > 1) && (cnt < n - 1)))) {
		cw = start->wcols;
		wcp->vals[0] = start->ch;
		wcp->attributes = start->attr;
		wcp->elements = 1;
		np = start->nsp;
		if (np) {
			do {
				wcp->vals[wcp->elements++] = np->ch;
				np = np->next;
			} while (np);
		}
		wcp++;
		cnt++;
		x += cw;
		if (x < win->maxx)
			start += cw;
	}
	wcp->vals[0] = L'\0';
	wcp->elements = 1;
	wcp->attributes = win->wattr;

	return OK;
}
