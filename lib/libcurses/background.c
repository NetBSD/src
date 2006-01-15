/*	$NetBSD: background.c,v 1.9 2006/01/15 11:43:54 jdc Exp $	*/

/*-
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
__RCSID("$NetBSD: background.c,v 1.9 2006/01/15 11:43:54 jdc Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * bkgdset
 *	Set new background attributes on stdscr.
 */
void
bkgdset(chtype ch)
{
	wbkgdset(stdscr, ch);
}

/*
 * bkgd --
 *	Set new background and new background attributes on stdscr.
 */
int
bkgd(chtype ch)
{
	return(wbkgd(stdscr, ch));
}

/*
 * wbkgdset
 *	Set new background attributes.
 */
void
wbkgdset(WINDOW *win, chtype ch)
{
#ifdef DEBUG
	__CTRACE("wbkgdset: (%p), '%s', %08x\n",
	    win, unctrl(ch & +__CHARTEXT), ch & __ATTRIBUTES);
#endif

	/* Background character. */
	if (ch & __CHARTEXT)
		win->bch = (wchar_t) ch & __CHARTEXT;

	/* Background attributes (check colour). */
	if (__using_color && !(ch & __COLOR))
		ch |= __default_color;
	win->battr = (attr_t) ch & __ATTRIBUTES;
}

/*
 * wbkgd --
 *	Set new background and new background attributes.
 */
int
wbkgd(WINDOW *win, chtype ch)
{
	int	y, x;

#ifdef DEBUG
	__CTRACE("wbkgd: (%p), '%s', %08x\n",
	    win, unctrl(ch & +__CHARTEXT), ch & __ATTRIBUTES);
#endif

	/* Background attributes (check colour). */
	if (__using_color && !(ch & __COLOR))
		ch |= __default_color;

	win->battr = (attr_t) ch & __ATTRIBUTES;
	wbkgdset(win, ch);
	for (y = 0; y < win->maxy; y++)
		for (x = 0; x < win->maxx; x++) {
			/* Copy character if space */
			if (ch & A_CHARTEXT && win->lines[y]->line[x].ch == ' ')
				win->lines[y]->line[x].ch = ch & __CHARTEXT;
			/* Merge attributes */
			if (win->lines[y]->line[x].attr & __ALTCHARSET)
				win->lines[y]->line[x].attr =
				    (ch & __ATTRIBUTES) | __ALTCHARSET;
			else
				win->lines[y]->line[x].attr =
				    ch & __ATTRIBUTES;
		}
	__touchwin(win);
	return(OK);
}

/*
 * getbkgd --
 *	Get current background attributes.
 */	
chtype
getbkgd(WINDOW *win)
{
	attr_t	battr;

	/* Background attributes (check colour). */
	battr = win->battr & A_ATTRIBUTES;
	if (__using_color && ((battr & __COLOR) == __default_color))
		battr &= ~__default_color;

	return ((chtype) ((win->bch & A_CHARTEXT) | battr));
}
