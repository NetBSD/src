/*	$NetBSD: pl_main.c,v 1.27 2010/08/06 09:14:40 dholland Exp $	*/

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
static char sccsid[] = "@(#)pl_main.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: pl_main.c,v 1.27 2010/08/06 09:14:40 dholland Exp $");
#endif
#endif /* not lint */

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "display.h"
#include "extern.h"
#include "player.h"

char myname[MAXNAMESIZE];

static void initialize(void);

void
pl_main_init(void)
{
	int nat[NNATION];
	int n;
	struct ship *sp;

	if (game < 0 || game >= NSCENE) {
		errx(1, "Very funny.");
	}
	cc = &scene[game];
	ls = SHIP(cc->vessels);

	for (n = 0; n < NNATION; n++)
		nat[n] = 0;
	foreachship(sp) {
		if (sp->file == NULL &&
		    (sp->file = calloc(1, sizeof (struct File))) == NULL) {
			err(1, "calloc");
		}
		sp->file->index = sp - SHIP(0);
		sp->file->stern = nat[sp->nationality]++;
		sp->file->dir = sp->shipdir;
		sp->file->row = sp->shiprow;
		sp->file->col = sp->shipcol;
	}
	windspeed = cc->windspeed;
	winddir = cc->winddir;

	signal(SIGHUP, choke);
	signal(SIGINT, choke);
	signal(SIGCHLD, child);
}

void
pl_main_uninit(void)
{
	struct ship *sp;

	foreachship(sp) {
		free(sp->file);
		sp->file = NULL;
	}
}

static void
initialize(void)
{
	char captain[MAXNAMESIZE];
	char message[60];

	if (game < 0 || game >= NSCENE) {
		errx(1, "Very funny.");
	}

	send_begin(ms);
	if (Sync() < 0)
		leave(LEAVE_SYNC);

#if 0
	printf("Your ship is the %s, a %d gun %s (%s crew).\n",
		ms->shipname, mc->guns, classname[mc->class],
		qualname[mc->qual]);
#endif

	strlcpy(captain, myname, sizeof(captain));
	send_captain(ms, captain);

	display_redraw();
	snprintf(message, sizeof message, "Captain %s assuming command",
			captain);
	send_signal(ms, message);
	newturn(0);
}

void
pl_main(void)
{
	initialize();
	Msg("Aye aye, Sir");
	play();
}
