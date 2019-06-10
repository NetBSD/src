/*   $NetBSD: echo_wchar.c,v 1.2.76.1 2019/06/10 22:05:22 christos Exp $ */

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
__RCSID("$NetBSD: echo_wchar.c,v 1.2.76.1 2019/06/10 22:05:22 christos Exp $");
#endif						  /* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * echo_wchar --
 *	Echo wide character and attributes on stdscr and refresh stdscr.
 */
int
echo_wchar(const cchar_t *wch)
{
	return wecho_wchar(stdscr, wch);
}

/*
 * echo_wchar --
 *	Echo wide character and attributes on "win" and refresh "win".
 */
int
wecho_wchar(WINDOW *win, const cchar_t *wch)
{
	int retval;

	retval = wadd_wch(win, wch);
	if (retval == OK)
		 retval = wrefresh(win);
	return retval;
}

/*
 * pecho_wchar --
 *	Echo character and attributes on "pad" and refresh "pad" at
 *	its previous position on the screen.
 */
int
pecho_wchar(WINDOW *pad, const cchar_t *wch)
{
	int retval;

	retval = wadd_wch(pad, wch);
	if (retval == OK)
		 retval = prefresh(pad, pad->pbegy, pad->pbegx,
			pad->sbegy, pad->sbegx, pad->smaxy, pad->smaxx);
	return retval;
}
