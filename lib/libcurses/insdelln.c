/*	$NetBSD: insdelln.c,v 1.10 2003/07/29 16:42:55 dsl Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: insdelln.c,v 1.10 2003/07/29 16:42:55 dsl Exp $");
#endif				/* not lint */

/* 
 * Based on deleteln.c and insertln.c -
 * Copyright (c) 1981, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 */

#include <string.h>

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS

/*
 * insdelln --
 *	Insert or delete lines on stdscr, leaving (cury, curx) unchanged.
 */
int
insdelln(int lines)
{
	return winsdelln(stdscr, lines);
}

#endif

/*
 * winsdelln --
 *	Insert or delete lines on the window, leaving (cury, curx) unchanged.
 */
int
winsdelln(WINDOW *win, int lines)
{
	int     y, i, last;
	__LINE *temp;

#ifdef DEBUG
	__CTRACE("winsdelln: (%p) cury=%d lines=%d\n", win, win->cury,
	    lines);
#endif

	if (!lines)
		return(OK);

	if (lines > 0) {
		/* Insert lines */
		if (win->cury < win->scr_t || win->cury > win->scr_b) {
			/*  Outside scrolling region */
			if (lines > win->maxy - win->cury)
				lines = win->maxy - win->cury;
			last = win->maxy - 1;
		} else {
			/* Inside scrolling region */
			if (lines > win->scr_b + 1 - win->cury)
				lines = win->scr_b + 1 - win->cury;
			last = win->scr_b;
		}
		for (y = last - lines; y >= win->cury; --y) {
			win->lines[y]->flags &= ~__ISPASTEOL;
			win->lines[y + lines]->flags &= ~__ISPASTEOL;
			if (win->orig == NULL) {
				temp = win->lines[y + lines];
				win->lines[y + lines] = win->lines[y];
				win->lines[y] = temp;
			} else {
				(void) memcpy(win->lines[y + lines]->line,
				    win->lines[y]->line,
				    (size_t) win->maxx * __LDATASIZE);
			}
		}
		for (y = win->cury - 1 + lines; y >= win->cury; --y)
			for (i = 0; i < win->maxx; i++) {
				win->lines[y]->line[i].ch = ' ';
				win->lines[y]->line[i].bch = win->bch;
				win->lines[y]->line[i].attr = 0;
				win->lines[y]->line[i].battr = win->battr;
			}
		for (y = last; y >= win->cury; --y)
			__touchline(win, y, 0, (int) win->maxx - 1);
	} else {
		/* Delete lines */
		lines = 0 - lines;
		if (win->cury < win->scr_t || win->cury > win->scr_b) {
			/*  Outside scrolling region */
			if (lines > win->maxy - win->cury)
				lines = win->maxy - win->cury;
			last = win->maxy;
		} else {
			/* Inside scrolling region */
			if (lines > win->scr_b + 1 - win->cury)
				lines = win->scr_b + 1 - win->cury;
			last = win->scr_b + 1;
		}
		for (y = win->cury; y < last - lines; y++) {
			win->lines[y]->flags &= ~__ISPASTEOL;
			win->lines[y + lines]->flags &= ~__ISPASTEOL;
			if (win->orig == NULL) {
				temp = win->lines[y];
				win->lines[y] = win->lines[y + lines];
				win->lines[y + lines] = temp;
			} else {
				(void) memcpy(win->lines[y]->line,
				    win->lines[y + lines]->line,
				    (size_t) win->maxx * __LDATASIZE);
			}
		}
		for (y = last - lines; y < last; y++)
			for (i = 0; i < win->maxx; i++) {
				win->lines[y]->line[i].ch = ' ';
				win->lines[y]->line[i].bch = win->bch;
				win->lines[y]->line[i].attr = 0;
				win->lines[y]->line[i].battr = win->battr;
			}
		for (y = win->cury; y < last; y++)
			__touchline(win, y, 0, (int) win->maxx - 1);
	}
	if (win->orig != NULL)
		__id_subwins(win->orig);
	return (OK);
}
