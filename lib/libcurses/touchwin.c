/*	$NetBSD: touchwin.c,v 1.9.6.1 2000/01/09 20:43:22 jdc Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)touchwin.c	8.2 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: touchwin.c,v 1.9.6.1 2000/01/09 20:43:22 jdc Exp $");
#endif
#endif				/* not lint */

#include "curses.h"

/*
 * touchline --
 *	Touch a given line.
 */
int
bsd_touchline(win, y, sx, ex)
	WINDOW *win;
	int     y, sx, ex;
{
	return (__touchline(win, y, sx, ex, 1));
}

/*
 * Touch count lines starting at start.  This is the SUS v2 compliant
 * version.
 */

int
touchline(win, start, count)
	WINDOW *win;
	int     start, count;
{
	int y;

	for (y = start; y < start + count; y++) {
		__touchline(win, y, 0, (int) win->maxx - 1, 1);
	}

	return OK;
}

/*
 * touchwin --
 *	Make it look like the whole window has been changed.
 */
int
touchwin(win)
	WINDOW *win;
{
	int     y, maxy;

#ifdef DEBUG
	__CTRACE("touchwin: (%0.2o)\n", win);
#endif
	maxy = win->maxy;
	for (y = 0; y < maxy; y++)
		__touchline(win, y, 0, (int) win->maxx - 1, 1);
	return (OK);
}


int
__touchwin(win)
	WINDOW *win;
{
	int     y, maxy;

#ifdef DEBUG
	__CTRACE("touchwin: (%0.2o)\n", win);
#endif
	maxy = win->maxy;
	for (y = 0; y < maxy; y++)
		__touchline(win, y, 0, (int) win->maxx - 1, 0);
	return (OK);
}

int
__touchline(win, y, sx, ex, force)
	WINDOW *win;
	int     y, sx, ex;
	int     force;
{
#ifdef DEBUG
	__CTRACE("touchline: (%0.2o, %d, %d, %d, %d)\n", win, y, sx, ex, force);
	__CTRACE("touchline: first = %d, last = %d\n",
	    *win->lines[y]->firstchp, *win->lines[y]->lastchp);
#endif
	if (force)
		win->lines[y]->flags |= __FORCEPAINT;
	sx += win->ch_off;
	ex += win->ch_off;
	if (!(win->lines[y]->flags & __ISDIRTY)) {
		win->lines[y]->flags |= __ISDIRTY;
		*win->lines[y]->firstchp = sx;
		*win->lines[y]->lastchp = ex;
	} else {
		if (*win->lines[y]->firstchp > sx)
			*win->lines[y]->firstchp = sx;
		if (*win->lines[y]->lastchp < ex)
			*win->lines[y]->lastchp = ex;
	}
#ifdef DEBUG
	__CTRACE("touchline: first = %d, last = %d\n",
	    *win->lines[y]->firstchp, *win->lines[y]->lastchp);
#endif
	return (OK);
}
