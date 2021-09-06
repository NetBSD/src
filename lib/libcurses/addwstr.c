/*   $NetBSD: addwstr.c,v 1.8 2021/09/06 07:45:48 rin Exp $ */

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
__RCSID("$NetBSD: addwstr.c,v 1.8 2021/09/06 07:45:48 rin Exp $");
#endif						  /* not lint */

#include <string.h>

#include "curses.h"
#include "curses_private.h"

/*
 * addwstr --
 *	  Add a string to stdscr starting at (_cury, _curx).
 */
int
addwstr(const wchar_t *s)
{
	return waddnwstr(stdscr, s, -1);
}

/*
 * waddwstr --
 *	  Add a string to the given window starting at (_cury, _curx).
 */
int
waddwstr(WINDOW *win, const wchar_t *s)
{
	return waddnwstr(win, s, -1);
}

/*
 * addnwstr --
 *	  Add a string (at most n characters) to stdscr starting
 *	at (_cury, _curx).  If n is negative, add the entire string.
 */
int
addnwstr(const wchar_t *str, int n)
{
	return waddnwstr(stdscr, str, n);
}

/*
 * mvaddwstr --
 *	  Add a string to stdscr starting at (y, x)
 */
int
mvaddwstr(int y, int x, const wchar_t *str)
{
	return mvwaddnwstr(stdscr, y, x, str, -1);
}

/*
 * mvwaddwstr --
 *	  Add a string to the given window starting at (y, x)
 */
int
mvwaddwstr(WINDOW *win, int y, int x, const wchar_t *str)
{
	return mvwaddnwstr(win, y, x, str, -1);
}

/*
 * mvaddnwstr --
 *	  Add a string of at most n characters to stdscr
 *	  starting at (y, x).
 */
int
mvaddnwstr(int y, int x, const wchar_t *str, int count)
{
	return mvwaddnwstr(stdscr, y, x, str, count);
}

/*
 * mvwaddnwstr --
 *	  Add a string of at most n characters to the given window
 *	  starting at (y, x).
 */
int
mvwaddnwstr(WINDOW *win, int y, int x, const wchar_t *str, int count)
{
	if (wmove(win, y, x) == ERR)
		return ERR;

	return waddnwstr(win, str, count);
}

/*
 * waddnwstr --
 *	Add a string (at most n characters) to the given window
 *	starting at (_cury, _curx).  If n is negative, add the
 *	entire string.
 */
int
waddnwstr(WINDOW *win, const wchar_t *s, int n)
{
	size_t  len;
	const wchar_t *p;
	cchar_t cc;
	wchar_t wc[2];

	/*
	 * BSD curses: if (n > 0) then "at most n", else "len = strlen(s)"
	 * ncurses: if (n >= 0) then "at most n", else "len = strlen(s)"
	 * XCURSES: if (n != -1) then "at most n", else "len = strlen(s)"
	 */
	/* compute the length and column width of string */
	if (n < -1)
		return ERR;
	if (n >= 0)
		for (p = s, len = 0; n-- && *p++; ++len);
	else
		len = wcslen(s);
	__CTRACE(__CTRACE_INPUT, "waddnwstr: string len=%zu\n", len);

	p = s;
	while (len) {
		wc[0] = *p;
		wc[1] = L'\0';
		if (setcchar( &cc, wc, win->wattr, 0, NULL ) == ERR)
			return ERR;
		if (wadd_wch( win, &cc ) == ERR)
			return ERR;
		__CTRACE(__CTRACE_INPUT, "waddnwstr: (%x,%x,%d) added\n",
		    cc.vals[0], cc.attributes, cc.elements);
		p++, len--;
	}

	return OK;
}
