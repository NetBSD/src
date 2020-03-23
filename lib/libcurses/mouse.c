/*	$NetBSD: mouse.c,v 1.1 2020/03/23 13:37:36 roy Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__RCSID("$NetBSD: mouse.c,v 1.1 2020/03/23 13:37:36 roy Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

#define	DEFAULT_MAXCLICK	166	/* ncurses default */

/*
 * Return true if screen relative y and x are enclosed by the window.
 */
bool
wenclose(const WINDOW *win, int y, int x)
{

	if (y < win->begy || y > win->begy + win->maxy ||
	    x < win->begx || x > win->begx + win->maxx)
		return false;
	return true;
}

bool
mouse_trafo(int *y, int *x, bool to_screen)
{

	return wmouse_trafo(stdscr, y, x, to_screen);
}

/*
 * Transform y and x from screen relative to window relative
 */
bool wmouse_trafo(const WINDOW *win, int *y, int *x, bool to_screen)
{
	int wy = *y, wx = *x;

	if (to_screen) {
		wy += win->begy;
		wx += win->begx;
	}

	if (!wenclose(win, wy, wx))
		return false;

	if (!to_screen) {
		wy -= win->begy;
		wx -= win->begx;
	}

	*y = wy;
	*x = wx;
	return true;
}

/*
 * The below functions actually need some kind of mouse support.
 * We should look into wsmouse(4) support at least as well as xterm.
 */

bool
has_mouse(void)
{

	return false;
}

int
getmouse(__unused MEVENT *event)
{

	return ERR;
}

int
ungetmouse(__unused MEVENT *event)
{

	return ERR;
}

mmask_t
mousemask(__unused mmask_t newmask, __unused mmask_t *oldmask)
{

	return 0;
}

int
mouseinterval(__unused int erval)
{

	return DEFAULT_MAXCLICK;
}
