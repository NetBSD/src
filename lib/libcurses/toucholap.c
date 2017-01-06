/*	$NetBSD: toucholap.c,v 1.17 2017/01/06 13:53:18 roy Exp $	*/

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
static char sccsid[] = "@(#)toucholap.c	8.2 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: toucholap.c,v 1.17 2017/01/06 13:53:18 roy Exp $");
#endif
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * touchoverlap --
 *	Touch, on win2, the part that overlaps with win1.
 */
int
touchoverlap(WINDOW *win1, WINDOW *win2)
{
	int     y, endy, endx, starty, startx;

#ifdef DEBUG
	__CTRACE(__CTRACE_WINDOW, "touchoverlap: (%p, %p);\n", win1, win2);
#endif
	starty = max(win1->begy, win2->begy);
	startx = max(win1->begx, win2->begx);
	endy = min(win1->maxy + win1->begy, win2->maxy + win2->begy);
	endx = min(win1->maxx + win1->begx, win2->maxx + win2->begx);
#ifdef DEBUG
	__CTRACE(__CTRACE_WINDOW, "touchoverlap: from (%d,%d) to (%d,%d)\n",
	    starty, startx, endy, endx);
	__CTRACE(__CTRACE_WINDOW, "touchoverlap: win1 (%d,%d) to (%d,%d)\n",
	    win1->begy, win1->begx, win1->begy + win1->maxy,
	    win1->begx + win1->maxx);
	__CTRACE(__CTRACE_WINDOW, "touchoverlap: win2 (%d,%d) to (%d,%d)\n",
	    win2->begy, win2->begx, win2->begy + win2->maxy,
	    win2->begx + win2->maxx);
#endif
	if (starty >= endy || startx >= endx)
		return OK;
	starty -= win2->begy;
	startx -= win2->begx;
	endy -= win2->begy;
	endx -= win2->begx;
	for (--endx, y = starty; y < endy; y++)
		__touchline(win2, y, startx, endx);
	return OK;
}
