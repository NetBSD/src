/*	$NetBSD: attributes.c,v 1.4 2000/04/15 13:17:02 blymn Exp $	*/

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

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS
/*
 * attron
 *	Test and set attributes on stdscr
 */

int
attron(int attr)
{
	return wattron(stdscr, attr);
}

/*
 * attroff
 *	Test and unset attributes on stdscr.
 */
int
attroff(int attr)
{
	return wattroff(stdscr, attr);
}

/*
 * attrset
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
 * wattron
 *	Test and set attributes.
 *
 *	Modes are blinking, bold (extra bright), dim (half-bright),
 *	blanking (invisible), protected and reverse video
 */

int
wattron(WINDOW *win, int attr)
{
#ifdef DEBUG
	__CTRACE ("wattron: %08x, %08x\n", attr, __nca);
#endif
	if ((attr_t) attr & __BLINK) {
		/* If can do blink, set the screen blink bit. */
		if (MB != NULL && ME != NULL) {
			win->wattr |= __BLINK;
			/*
			 * Check for conflict with color.
			 */
			if ((win->wattr & __COLOR) && (__nca & __BLINK)) {
				win->wattr &= ~__COLOR;
			}
		}
	}
	if ((attr_t) attr & __BOLD) {
		/* If can do bold, set the screen bold bit. */
		if (MD != NULL && ME != NULL) {
			win->wattr |= __BOLD;
			if ((win->wattr & __COLOR) && (__nca & __BOLD)) {
				win->wattr &= ~__COLOR;
			}
		}
	}
	if ((attr_t) attr & __DIM) {
		/* If can do dim, set the screen dim bit. */
		if (MH != NULL && ME != NULL) {
			win->wattr |= __DIM;
			if ((win->wattr & __COLOR) && (__nca & __DIM)) {
				win->wattr &= ~__COLOR;
			}
		}
	}
	if ((attr_t) attr & __BLANK) {
		/* If can do blank, set the screen blank bit. */
		if (MK != NULL && ME != NULL) {
			win->wattr |= __BLANK;
			if ((win->wattr & __COLOR) && (__nca & __BLANK)) {
				win->wattr &= ~__COLOR;
			}
		}
	}
	if ((attr_t) attr & __PROTECT) {
		/* If can do protected, set the screen protected bit. */
		if (MP != NULL && ME != NULL) {
			win->wattr |= __PROTECT;
			if ((win->wattr & __COLOR) && (__nca & __PROTECT)) {
				win->wattr &= ~__COLOR;
			}
		}
	}
	if ((attr_t) attr & __REVERSE) {
		/* If can do reverse video, set the screen reverse video bit. */
		if (MR != NULL && ME != NULL)
		{
			win->wattr |= __REVERSE;
			if ((win->wattr & __COLOR) && (__nca & __REVERSE)) {
				win->wattr &= ~__COLOR;
			}
		}
	}
	if ((attr_t) attr & __STANDOUT) {
		wstandout(win);
	}
	if ((attr_t) attr & __UNDERSCORE) {
		wunderscore(win);
	}
	if ((attr_t) attr & __COLOR) {
		/* If another color pair is set, turn that off first. */
		if ((win->wattr & __COLOR) != ((attr_t) attr & __COLOR))
			win->wattr &= ~__COLOR;
		/* If can do color video, set the color pair bits. */
		if (cO != NULL)
		{
			win->wattr |= attr & __COLOR;
			if (__nca != __NORMAL) {
				win->wattr &= ~__nca;
			}
		}
	}
	return (1);
}

/*
 * wattroff
 *	Test and unset attributes.
 *
 *	Note that the 'me' sequence unsets all attributes.  We handle
 *	which attributes should really be set in refresh.c:makech().
 */
int
wattroff(WINDOW *win, int attr)
{
#ifdef DEBUG
	__CTRACE ("wattroff: %08x\n", attr);
#endif
	/* If can do exit modes, unset the relevent attribute bits. */
	if ((attr_t) attr & __BLINK) {
		if (ME != NULL) {
			win->wattr &= ~__BLINK;
		}
	}
	if ((attr_t) attr & __BOLD) {
		if (ME != NULL) {
			win->wattr &= ~__BOLD;
		}
	}
	if ((attr_t) attr & __DIM) {
		if (ME != NULL) {
			win->wattr &= ~__DIM;
		}
	}
	if ((attr_t) attr & __BLANK) {
		if (ME != NULL) {
			win->wattr &= ~__BLANK;
		}
	}
	if ((attr_t) attr & __PROTECT) {
		if (ME != NULL) {
			win->wattr &= ~__PROTECT;
		}
	}
	if ((attr_t) attr & __REVERSE) {
		if (ME != NULL) {
			win->wattr &= ~__REVERSE;
		}
	}
	if ((attr_t) attr & __STANDOUT) {
		wstandend(win);
	}
	if ((attr_t) attr & __UNDERSCORE) {
		wunderend(win);
	}
	if ((attr_t) attr & __COLOR) {
		if (cO != NULL)
		{
			win->wattr &= ~__COLOR;
		}
	}

	return (1);
}

/*
 * wattrset
 *	Set specific attribute modes.
 *	Unset others.
 */
int
wattrset(WINDOW *win, int attr)
{
	if ((attr_t) attr & __BLINK)
		wattron(win, __BLINK);
	else
		wattroff(win, __BLINK);
	if ((attr_t) attr & __BOLD)
		wattron(win, __BOLD);
	else
		wattroff(win, __BOLD);
	if ((attr_t) attr & __DIM)
		wattron(win, __DIM);
	else
		wattroff(win, __DIM);
	if ((attr_t) attr & __BLANK)
		wattron(win, __BLANK);
	else
		wattroff(win, __BLANK);
	if ((attr_t) attr & __PROTECT)
		wattron(win, __PROTECT);
	else
		wattroff(win, __PROTECT);
	if ((attr_t) attr & __REVERSE)
		wattron(win, __REVERSE);
	else
		wattroff(win, __REVERSE);
	if ((attr_t) attr & __STANDOUT)
		wstandout(win);
	else
		wstandend(win);
	if ((attr_t) attr & __UNDERSCORE)
		wunderscore(win);
	else
		wunderend(win);
	if ((attr_t) attr & __COLOR)
		wattron(win, attr & (int) __COLOR);
	else
		wattroff(win, (int) __COLOR);
	return (1);
}
