/*	$NetBSD: border.c,v 1.2 2000/04/11 13:57:08 blymn Exp $	*/

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

#include "curses.h"
#include "curses_private.h"

/*
 * wborder --
 *	Draw a border around the given window using the specified delimiting
 *	characters.
 */
int
wborder(win, left, right, top, bottom, topleft, topright, botleft, botright)
	WINDOW	*win;
	chtype	 left, right, top, bottom;
	chtype	 topleft, topright, botleft, botright;
{
	int	 endy, endx, i;
	__LDATA	*fp, *lp;

	if (!(left & __CHARTEXT)) left = ACS_VLINE;
	if (!(right & __CHARTEXT)) right = ACS_VLINE;
	if (!(top & __CHARTEXT)) top = ACS_HLINE;
	if (!(bottom & __CHARTEXT)) bottom = ACS_HLINE;
	if (!(topleft & __CHARTEXT)) topleft = ACS_ULCORNER;
	if (!(topright & __CHARTEXT)) topright = ACS_URCORNER;
	if (!(botleft & __CHARTEXT)) botleft = ACS_LLCORNER;
	if (!(botright & __CHARTEXT)) botright = ACS_LRCORNER;

#ifdef DEBUG
	__CTRACE("wborder: left = %c, 0x%x\n", left, left & __ATTRIBUTES);
	__CTRACE("wborder: right = %c, 0x%x\n", right, right & __ATTRIBUTES);
	__CTRACE("wborder: top = %c, 0x%x\n", top, top & __ATTRIBUTES);
	__CTRACE("wborder: bottom = %c, 0x%x\n", bottom, bottom & __ATTRIBUTES);
	__CTRACE("wborder: topleft = %c, 0x%x\n", topleft, topleft & __ATTRIBUTES);
	__CTRACE("wborder: topright = %c, 0x%x\n", topright, topright & __ATTRIBUTES);
	__CTRACE("wborder: botleft = %c, 0x%x\n", botleft, botleft & __ATTRIBUTES);
	__CTRACE("wborder: botright = %c, 0x%x\n", botright, botright & __ATTRIBUTES);
#endif

	/* Merge window attributes */
	left |= win->wattr;
	right |= win->wattr;
	top |= win->wattr;
	bottom |= win->wattr;
	topleft |= win->wattr;
	topright |= win->wattr;
	botleft |= win->wattr;
	botright |= win->wattr;

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
