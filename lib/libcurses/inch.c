/*	$NetBSD: inch.c,v 1.14 2019/06/09 07:40:14 blymn Exp $	*/

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
__RCSID("$NetBSD: inch.c,v 1.14 2019/06/09 07:40:14 blymn Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

#ifndef _CURSES_USE_MACROS

/*
 * inch --
 *	Return character at cursor position from stdscr.
 */
chtype
inch(void)
{

	return winch(stdscr);
}

/*
 * mvinch --
 *      Return character at position (y, x) from stdscr.
 */
chtype
mvinch(int y, int x)
{

	return mvwinch(stdscr, y, x);
}

/*
 * mvwinch --
 *      Return character at position (y, x) from the given window.
 */
chtype
mvwinch(WINDOW *win, int y, int x)
{

	if (wmove(win, y, x) == ERR)
		return ERR;

	return winch(win);
}

#endif

/*
 * winch --
 *	Return character at cursor position.
 */
chtype
winch(WINDOW *win)
{
	chtype	ch;
	attr_t	attr;

	ch = (chtype) ((win)->alines[(win)->cury]->line[(win)->curx].ch &
	    __CHARTEXT);
	attr = (attr_t) ((win)->alines[(win)->cury]->line[(win)->curx].attr &
	    __ATTRIBUTES);
	if (__using_color && ((attr & __COLOR) == __default_color))
		attr &= ~__COLOR;
	return (ch | attr);
}
