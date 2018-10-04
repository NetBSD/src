/*	$NetBSD: ripoffline.c,v 1.3.6.1 2018/10/04 10:20:12 martin Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: ripoffline.c,v 1.3.6.1 2018/10/04 10:20:12 martin Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/* List of ripoffline calls */
static struct ripoff {
	int	nlines;
	int	(*init)(WINDOW *, int);
} ripoffs[MAX_RIPS];
static int nrips;

/*
 * ripoffline --
 *	Ripoff a line from the top of bottom of stdscr.
 *	Must be called before initscr or newterm.
 */
int
ripoffline(int line, int (*init)(WINDOW *, int))
{

#ifdef DEBUG
	__CTRACE(__CTRACE_WINDOW, "ripoffline: %d\n", line);
#endif

	if (nrips >= MAX_RIPS || init == NULL)
		return ERR; /* This makes sense, but not standards compliant. */
	if (line == 0)
		return OK;
	ripoffs[nrips].nlines = line < 0 ? -1 : 1;
	ripoffs[nrips++].init = init;
	return OK;
}

/*
 * __rippedlines --
 *	Returns the number of ripped lines from the screen.
 */
int
__rippedlines(const SCREEN *screen, int line)
{
	const struct __ripoff *rip;
	int i, n;

	n = 0;
	for (i = 0, rip = screen->ripped; i < screen->nripped; i++, rip++) {
		if (line < 1 && rip->nlines < 0)
			n += -rip->nlines;
		else if (line > 0 && rip->nlines > 0)
			n += rip->nlines;
	}
	return n;
}

/*
 * __ripoffscreen --
 *	Rips lines from the screen by creating a WINDOW per ripoffline call.
 *	Although the POSIX API only allows for one line WINDOWS to be created,
 *	this implemenation allows for N lines if needed.
 */
int
__ripoffscreen(SCREEN *screen)
{
	int i, nlines, rbot, rtop;
	const struct ripoff *srip;
	struct __ripoff *rip;
	WINDOW *w;

	rip = screen->ripped;
	rbot = LINES;
	rtop = 0;
	for (i = 0, srip = ripoffs; i < nrips; i++, srip++) {
		if (srip->nlines == 0)
			continue;
		nlines = srip->nlines < 0 ? -srip->nlines : srip->nlines;
		w = __newwin(screen, nlines, 0,
		    srip->nlines < 0 ? rbot - nlines : rtop,
		    0, FALSE, FALSE);
		if (w != NULL) {
			rip->win = w;
			rip->nlines = srip->nlines;
			rip++;
			screen->nripped++;
			if (srip->nlines > 0)
				rtop += nlines;
			else
				rbot -= nlines;
		}
		if (srip->init(w, COLS) == ERR)
			return ERR;
#ifdef DEBUG
		if (w != NULL)
			__CTRACE(__CTRACE_WINDOW,
			    "newterm: %p ripped %d line(s) from the %s\n",
			    w, nlines, srip->nlines < 0 ? "bottom" : "top");
#endif
	}
	nrips = 0; /* Reset the stack. */
	return OK;
}

/*
 * __ripoffresize --
 *	Called from resizeterm to ensure the ripped off lines are correctly
 *	placed and refreshed.
 */
int
__ripoffresize(SCREEN *screen)
{
	int rbot = screen->LINES, i, nlines, ret = OK;
	struct __ripoff *rip;

	for (i = 0, rip = screen->ripped; i < screen->nripped; i++, rip++) {
		if (rip->nlines == 0)
			continue;
		nlines = rip->nlines < 0 ? -rip->nlines : rip->nlines;
		if (wresize(rip->win, nlines, screen->COLS) == ERR)
			ret = ERR;
		if (rip->nlines < 0) {
			/* Reposition the lower windows. */
			if (mvwin(rip->win, rbot + rip->nlines, 0) == ERR)
				ret = ERR;
			else
				rbot += rip->nlines;
		}
	}

	return ret;
}

/*
 *  __ripofftouch --
 *	Displays the ripped off lines from initscr.
 */
void
__ripofftouch(SCREEN *screen)
{
	int i;
	struct __ripoff *rip;

	for (i = 0, rip = screen->ripped; i < screen->nripped; i++, rip++) {
		touchwin(rip->win);
		wnoutrefresh(rip->win);
	}
}

/*
 * __unripoffline --
 *	Used by __slk_init to remove the ripoffline reservation it made
 *	because the terminal natively supports soft label keys.
 */
int
__unripoffline(int (*init)(WINDOW *, int))
{
	struct ripoff *rip;
	int i, unripped = 0;

	for (i = 0, rip = ripoffs; i < nrips; i++, rip++) {
		if (rip->init == init) {
			rip->nlines = 0;
			unripped++;
		}
	}
	return unripped;
}
