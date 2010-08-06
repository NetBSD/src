/*	$NetBSD: dr_main.c,v 1.15 2010/08/06 09:14:40 dholland Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)dr_main.c	8.2 (Berkeley) 4/16/94";
#else
__RCSID("$NetBSD: dr_main.c,v 1.15 2010/08/06 09:14:40 dholland Exp $");
#endif
#endif /* not lint */

#include <err.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "extern.h"
#include "driver.h"
#include "player.h" /* XXX for LEAVE_FORK */
#include "restart.h"

static int driver_wait_fd = -1;

int
dr_main(void)
{
	int n;
	struct ship *sp;
	int nat[NNATION];
	int value = 0;

	/*
	 * XXX need a way to print diagnostics back to the player
	 * process instead of stomping on the curses screen.
	 */

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	if (game < 0 || game >= NSCENE) {
		errx(1, "\ndriver: Bad game number %d", game);
	}
	cc = &scene[game];
	ls = SHIP(cc->vessels);
	if (sync_open() < 0) {
		err(1, "\ndriver: syncfile");
	}
	for (n = 0; n < NNATION; n++)
		nat[n] = 0;
	foreachship(sp) {
		if (sp->file == NULL &&
		    (sp->file = calloc(1, sizeof (struct File))) == NULL) {
			fprintf(stderr, "\nDRIVER: Out of memory.\n");
			exit(1);
		}
		sp->file->index = sp - SHIP(0);
		sp->file->loadL = L_ROUND;
		sp->file->loadR = L_ROUND;
		sp->file->readyR = R_LOADED|R_INITIAL;
		sp->file->readyL = R_LOADED|R_INITIAL;
		sp->file->stern = nat[sp->nationality]++;
		sp->file->dir = sp->shipdir;
		sp->file->row = sp->shiprow;
		sp->file->col = sp->shipcol;
	}
	windspeed = cc->windspeed;
	winddir = cc->winddir;
	people = 0;

	/* report back to the player process that we've started */
	if (driver_wait_fd >= 0) {
		close(driver_wait_fd);
	}

	for (;;) {
		sleep(7);
		if (Sync() < 0) {
			value = 1;
			break;
		}
		if (next() < 0)
			break;
		unfoul();
		checkup();
		prizecheck();
		moveall();
		thinkofgrapples();
		boardcomp();
		compcombat();
		resolve();
		reload();
		checksails();
		if (Sync() < 0) {
			value = 1;
			break;
		}
	}
	sync_close(1);
	return value;
}

void
startdriver(void)
{
	int fds[2];
	char c;

	if (pipe(fds)) {
		warn("pipe");
		leave(LEAVE_FORK);
		return;
	}

	switch (fork()) {
	    case 0:
		close(fds[0]);
		driver_wait_fd = fds[1];
		longjmp(restart, MODE_DRIVER);
		/*NOTREACHED*/
	    case -1:
		warn("fork");
		close(fds[0]);
		close(fds[1]);
		leave(LEAVE_FORK);
		break;
	    default:
		hasdriver++;
		close(fds[1]);
		read(fds[0], &c, 1);
		close(fds[0]);
	}
}
