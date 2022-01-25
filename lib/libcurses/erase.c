/*	$NetBSD: erase.c,v 1.34 2022/01/25 03:05:06 blymn Exp $	*/

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
static char sccsid[] = "@(#)erase.c	8.2 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: erase.c,v 1.34 2022/01/25 03:05:06 blymn Exp $");
#endif
#endif				/* not lint */

#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS

/*
 * erase --
 *	Erases everything on stdscr.
 */
int
erase(void)
{
	return werase(stdscr);
}

#endif

/*
 * werase --
 *	Erases everything on the window.
 */
int
werase(WINDOW *win)
{
	int     y;
	__LDATA *sp, *end, *start;
	wchar_t	bch;
	attr_t	battr;

	__CTRACE(__CTRACE_ERASE, "werase: (%p)\n", win);
#ifdef HAVE_WCHAR
	bch = (wchar_t)btowc((int)win->bch);
#else
	bch = win->bch;
#endif
	if (win != curscr)
		battr = win->battr & __ATTRIBUTES;
	else
		battr = 0;

	for (y = 0; y < win->maxy; y++) {
		start = win->alines[y]->line;
		end = &start[win->maxx];
		for (sp = start; sp < end; sp++) {
			if (!(__NEED_ERASE(sp, bch, battr)))
				continue;

			sp->ch = bch;
			sp->attr = battr | (sp->attr & __ALTCHARSET);
#ifdef HAVE_WCHAR
			if (_cursesi_copy_nsp(win->bnsp, sp) == ERR)
				return ERR;
			sp->wcols = 1;
#endif
		}
	}

	/*
	 * Mark the whole window as changed in case we have overlapping
	 * windows - this will result in the (intended) clearing of the
	 * screen over the area covered by the window. */
	__touchwin(win);
	wmove(win, 0, 0);
	return OK;
}
