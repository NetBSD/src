/*	$NetBSD: border.c,v 1.8 2006/01/15 11:43:54 jdc Exp $	*/

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
__RCSID("$NetBSD: border.c,v 1.8 2006/01/15 11:43:54 jdc Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS

/*
 * border --
 *      Draw a border around stdscr using the specified
 *	delimiting characters.
 */
int
border(chtype left, chtype right, chtype top, chtype bottom, chtype topleft,
       chtype topright, chtype botleft, chtype botright)
{
	return wborder(stdscr, left, right, top, bottom, topleft, topright,
		       botleft, botright);
}

#endif

/*
 * wborder --
 *	Draw a border around the given window using the specified delimiting
 *	characters.
 */
int
wborder(WINDOW *win, chtype left, chtype right, chtype top, chtype bottom,
	chtype topleft, chtype topright, chtype botleft, chtype botright)
{
	int	 endy, endx, i;
	__LDATA	*fp, *lp;

	if (!(left & __CHARTEXT))
		left |= ACS_VLINE;
	if (!(right & __CHARTEXT))
		right |= ACS_VLINE;
	if (!(top & __CHARTEXT))
		top |= ACS_HLINE;
	if (!(bottom & __CHARTEXT))
		bottom |= ACS_HLINE;
	if (!(topleft & __CHARTEXT))
		topleft |= ACS_ULCORNER;
	if (!(topright & __CHARTEXT))
		topright |= ACS_URCORNER;
	if (!(botleft & __CHARTEXT))
		botleft |= ACS_LLCORNER;
	if (!(botright & __CHARTEXT))
		botright |= ACS_LRCORNER;

#ifdef DEBUG
	__CTRACE("wborder: left = %c, 0x%x\n", left & __CHARTEXT,
	    left & __ATTRIBUTES);
	__CTRACE("wborder: right = %c, 0x%x\n", right & __CHARTEXT,
	    right & __ATTRIBUTES);
	__CTRACE("wborder: top = %c, 0x%x\n", top & __CHARTEXT,
	    top & __ATTRIBUTES);
	__CTRACE("wborder: bottom = %c, 0x%x\n", bottom & __CHARTEXT,
	    bottom & __ATTRIBUTES);
	__CTRACE("wborder: topleft = %c, 0x%x\n", topleft & __CHARTEXT,
	    topleft & __ATTRIBUTES);
	__CTRACE("wborder: topright = %c, 0x%x\n", topright & __CHARTEXT,
	    topright & __ATTRIBUTES);
	__CTRACE("wborder: botleft = %c, 0x%x\n", botleft & __CHARTEXT,
	    botleft & __ATTRIBUTES);
	__CTRACE("wborder: botright = %c, 0x%x\n", botright & __CHARTEXT,
	    botright & __ATTRIBUTES);
#endif

	/* Merge window and background attributes */
	left |= (left & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	left |= (left & __COLOR) ? (win->battr & ~__COLOR) : win->battr;
	right |= (right & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	right |= (right & __COLOR) ? (win->battr & ~__COLOR) : win->battr;
	top |= (top & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	top |= (top & __COLOR) ? (win->battr & ~__COLOR) : win->battr;
	bottom |= (bottom & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	bottom |= (bottom & __COLOR) ? (win->battr & ~__COLOR) : win->battr;
	topleft |= (topleft & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	topleft |= (topleft & __COLOR) ? (win->battr & ~__COLOR) : win->battr;
	topright |= (topright & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	topright |= (topright & __COLOR) ? (win->battr & ~__COLOR) : win->battr;
	botleft |= (botleft & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	botleft |= (botleft & __COLOR) ? (win->battr & ~__COLOR) : win->battr;
	botright |= (botright & __COLOR) ? (win->wattr & ~__COLOR) : win->wattr;
	botright |= (botright & __COLOR) ? (win->battr & ~__COLOR) : win->battr;

	endx = win->maxx - 1;
	endy = win->maxy - 1;
	fp = win->lines[0]->line;
	lp = win->lines[endy]->line;

	/* Sides */
	for (i = 1; i < endy; i++) {
		win->lines[i]->line[0].ch = (wchar_t) left & __CHARTEXT;
		win->lines[i]->line[0].attr = (attr_t) left & __ATTRIBUTES;
		win->lines[i]->line[endx].ch = (wchar_t) right & __CHARTEXT;
		win->lines[i]->line[endx].attr = (attr_t) right & __ATTRIBUTES;
	}
	for (i = 1; i < endx; i++) {
		fp[i].ch = (wchar_t) top & __CHARTEXT;
		fp[i].attr = (attr_t) top & __ATTRIBUTES;
		lp[i].ch = (wchar_t) bottom & __CHARTEXT;
		lp[i].attr = (attr_t) bottom & __ATTRIBUTES;
	}

	/* Corners */
	if (!(win->maxx == LINES && win->maxy == COLS &&
	    (win->flags & __SCROLLOK) && (win->flags & __SCROLLWIN))) {
		fp[0].ch = (wchar_t) topleft & __CHARTEXT;
		fp[0].attr = (attr_t) topleft & __ATTRIBUTES;
		fp[endx].ch = (wchar_t) topright & __CHARTEXT;
		fp[endx].attr = (attr_t) topright & __ATTRIBUTES;
		lp[0].ch = (wchar_t) botleft & __CHARTEXT;
		lp[0].attr = (attr_t) botleft & __ATTRIBUTES;
		lp[endx].ch = (wchar_t) botright & __CHARTEXT;
		lp[endx].attr = (attr_t) botright & __ATTRIBUTES;
	}
	__touchwin(win);
	return (OK);
}
