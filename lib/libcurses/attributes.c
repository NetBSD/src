/*	$NetBSD: attributes.c,v 1.9 2000/12/19 21:34:25 jdc Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: attributes.c,v 1.9 2000/12/19 21:34:25 jdc Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS
/*
 * attron --
 *	Test and set attributes on stdscr
 */

int
attron(int attr)
{
	return wattron(stdscr, attr);
}

/*
 * attroff --
 *	Test and unset attributes on stdscr.
 */
int
attroff(int attr)
{
	return wattroff(stdscr, attr);
}

/*
 * attrset --
 *	Set specific attribute modes.
 *	Unset others.  On stdscr.
 */
int
attrset(int attr)
{
	return wattrset(stdscr, attr);
}

#endif

/*
 * wattron --
 *	Test and set attributes.
 *
 *	Modes are blinking, bold (extra bright), dim (half-bright),
 *	blanking (invisible), protected and reverse video
 */

int
wattron(WINDOW *win, int attr)
{
#ifdef DEBUG
	__CTRACE ("wattron: win %0.2o, attr %08x\n", win, attr);
#endif
	/* If can enter modes, set the relevent attribute bits. */
	if (__tc_me != NULL) {
		if ((attr_t) attr & __BLINK && __tc_mb != NULL)
			win->wattr |= __BLINK;
		if ((attr_t) attr & __BOLD && __tc_md != NULL)
			win->wattr |= __BOLD;
		if ((attr_t) attr & __DIM && __tc_mh != NULL)
			win->wattr |= __DIM;
		if ((attr_t) attr & __BLANK && __tc_mk != NULL)
			win->wattr |= __BLANK;
		if ((attr_t) attr & __PROTECT && __tc_mp != NULL)
			win->wattr |= __PROTECT;
		if ((attr_t) attr & __REVERSE && __tc_mr != NULL)
			win->wattr |= __REVERSE;
	}
	if ((attr_t) attr & __STANDOUT)
		wstandout(win);
	if ((attr_t) attr & __UNDERSCORE)
		wunderscore(win);
	/* Check for conflict with color. */
	if (win->wattr & __nca)
		win->wattr &= ~__COLOR;
	if ((attr_t) attr & __COLOR) {
		/* If another color pair is set, turn that off first. */
		win->wattr &= ~__COLOR;
		/* If can do color video, set the color pair bits. */
		if (__tc_Co != NULL) {
			win->wattr |= attr & __COLOR;
			win->wattr &= ~__nca;
		}
	}
	return (OK);
}

/*
 * wattroff --
 *	Test and unset attributes.
 *
 *	Note that the 'me' sequence unsets all attributes.  We handle
 *	which attributes should really be set in refresh.c:makech().
 */
int
wattroff(WINDOW *win, int attr)
{
#ifdef DEBUG
	__CTRACE ("wattroff: win %0.2o, attr %08x\n", win, attr);
#endif
	/* If can do exit modes, unset the relevent attribute bits. */
	if (__tc_me != NULL) {
		if ((attr_t) attr & __BLINK)
			win->wattr &= ~__BLINK;
		if ((attr_t) attr & __BOLD)
			win->wattr &= ~__BOLD;
		if ((attr_t) attr & __DIM)
			win->wattr &= ~__DIM;
		if ((attr_t) attr & __BLANK)
			win->wattr &= ~__BLANK;
		if ((attr_t) attr & __PROTECT)
			win->wattr &= ~__PROTECT;
		if ((attr_t) attr & __REVERSE)
			win->wattr &= ~__REVERSE;
	}
	if ((attr_t) attr & __STANDOUT)
		wstandend(win);
	if ((attr_t) attr & __UNDERSCORE)
		wunderend(win);
	if ((attr_t) attr & __COLOR) {
		if (__tc_Co != NULL)
			win->wattr &= ~__COLOR;
	}
	return (OK);
}

/*
 * wattrset --
 *	Set specific attribute modes.
 *	Unset others.
 */
int
wattrset(WINDOW *win, int attr)
{
#ifdef DEBUG
	__CTRACE ("wattrset: win %0.2o, attr %08x\n", win, attr);
#endif
	wattron(win, attr);
	wattroff(win, (~attr & ~__COLOR) | ((attr & __COLOR) ? 0 : __COLOR));
	return (OK);
}

/*
 * getattrs --
 * Get window attributes.
 */
chtype
getattrs(WINDOW *win)
{
#ifdef DEBUG
	__CTRACE ("getattrs: win %0.2o\n", win);
#endif
	return((chtype) win->wattr);
}
