/*	$NetBSD: timer.c,v 1.8 2003/08/07 09:37:06 agc Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Barry Brachman.
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
static char sccsid[] = "@(#)timer.c	8.2 (Berkeley) 2/22/94";
#else
__RCSID("$NetBSD: timer.c,v 1.8 2003/08/07 09:37:06 agc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/poll.h>

#include <curses.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "bog.h"
#include "extern.h"

static int waitch __P((int));

extern int tlimit;
extern time_t start_t;
extern jmp_buf env;

/*
 * Update the display of the remaining time while waiting for a character
 * If time runs out do a longjmp() to the game controlling routine, returning
 * non-zero; oth. return the character
 * Leave the cursor where it was initially
 */
int
timerch()
{
	time_t prevt, t;
	int col, remaining, row;

	getyx(stdscr, row, col);
	prevt = 0L;
	for (;;) {
		if (waitch(1) == 1)
			break;
		time(&t);
		if (t == prevt)
			continue;
		prevt = t;
		remaining = tlimit - (int) (t - start_t);
		if (remaining < 0) {
			longjmp(env, 1);
			/*NOTREACHED*/
		}
		move(TIMER_LINE, TIMER_COL);
		printw("%d:%02d", remaining / 60, remaining % 60);
		move(row, col);
		refresh();
	}
	return (getch() & 0177);
}

/*
 * Wait up to 'delay' microseconds for input to appear
 * Returns 1 if input is ready, 0 oth.
 */
static int
waitch(delay)
	int delay;
{
	struct pollfd set[1];

	set[0].fd = STDIN_FILENO;
	set[0].events = POLLIN;
	return (poll(set, 1, delay));
}

void
delay(tenths)
	int tenths;
{
	struct timespec duration;

	duration.tv_nsec = (tenths % 10 ) * 100000000L;
	duration.tv_sec = (long) (tenths / 10);
	nanosleep(&duration, NULL);
}
