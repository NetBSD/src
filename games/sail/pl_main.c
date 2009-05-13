/*	$NetBSD: pl_main.c,v 1.17.28.1 2009/05/13 19:18:06 jym Exp $	*/

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
__RCSID("$NetBSD: pl_main.c,v 1.17.28.1 2009/05/13 19:18:06 jym Exp $");
#endif
#endif /* not lint */

#include <err.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "display.h"
#include "extern.h"
#include "player.h"
#include "restart.h"

static void initialize(void);

/*ARGSUSED*/
int
pl_main(void)
{
	initialize();
	Msg("Aye aye, Sir");
	play();
	return 0;			/* for lint,  play() never returns */
}

static void
initialize(void)
{
	struct File *fp;
	struct ship *sp;
	char captain[80];
	char message[60];
	int load;
	int n;
	char *nameptr;
	int nat[NNATION];

	if (game < 0) {
		puts("Choose a scenario:\n");
		puts("\n\tNUMBER\tSHIPS\tIN PLAY\tTITLE");
		for (n = 0; n < NSCENE; n++) {
			/* ( */
			printf("\t%d):\t%d\t%s\t%s\n", n, scene[n].vessels,
				sync_exists(n) ? "YES" : "no",
				scene[n].name);
		}
reprint:
		printf("\nScenario number? ");
		fflush(stdout);
		scanf("%d", &game);
		while (getchar() != '\n' && !feof(stdin))
			;
	}
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

	hasdriver = sync_exists(game);
	if (sync_open() < 0) {
		err(1, "syncfile");
	}

	if (hasdriver) {
		puts("Synchronizing with the other players...");
		fflush(stdout);
		if (Sync() < 0)
			leave(LEAVE_SYNC);
	}
	for (;;) {
		foreachship(sp)
			if (sp->file->captain[0] == 0 && !sp->file->struck
			    && sp->file->captured == 0)
				break;
		if (sp >= ls) {
			puts("All ships taken in that scenario.");
			foreachship(sp)
				free(sp->file);
			sync_close(0);
			people = 0;
			goto reprint;
		}
		if (randomize) {
			player = sp - SHIP(0);
		} else {
			printf("%s\n\n", cc->name);
			foreachship(sp)
				printf("  %2d:  %-10s %-15s  (%-2d pts)   %s\n",
					sp->file->index,
					countryname[sp->nationality],
					sp->shipname,
					sp->specs->pts,
					saywhat(sp, 1));
			printf("\nWhich ship (0-%d)? ", cc->vessels-1);
			fflush(stdout);
			if (scanf("%d", &player) != 1 || player < 0
			    || player >= cc->vessels) {
				while (getchar() != '\n' && !feof(stdin))
					;
				puts("Say what?");
				player = -1;
			} else
				while (getchar() != '\n' && !feof(stdin))
					;
			if (feof(stdin)) {
				printf("\nExiting...\n");
				leave(LEAVE_QUIT);
			}
		}
		if (player < 0)
			continue;
		if (Sync() < 0)
			leave(LEAVE_SYNC);
		fp = SHIP(player)->file;
		if (fp->captain[0] || fp->struck || fp->captured != 0)
			puts("That ship is taken.");
		else
			break;
	}

	ms = SHIP(player);
	mf = ms->file;
	mc = ms->specs;

	send_begin(ms);
	if (Sync() < 0)
		leave(LEAVE_SYNC);

	signal(SIGCHLD, child);
	if (!hasdriver)
		switch (fork()) {
		case 0:
			longjmp(restart, MODE_DRIVER);
			/*NOTREACHED*/
		case -1:
			warn("fork");
			leave(LEAVE_FORK);
			break;
		default:
			hasdriver++;
		}

	printf("Your ship is the %s, a %d gun %s (%s crew).\n",
		ms->shipname, mc->guns, classname[mc->class],
		qualname[mc->qual]);
	if ((nameptr = getenv("SAILNAME")) && *nameptr)
		strlcpy(captain, nameptr, sizeof captain);
	else {
		printf("Your name, Captain? ");
		fflush(stdout);
		if (fgets(captain, sizeof captain, stdin) == NULL)
			strcpy(captain, "no name");
		else if (*captain == '\0' || *captain == '\n')
			strcpy(captain, "no name");
		else
		    captain[strlen(captain) - 1] = '\0';
	}
	send_captain(ms, captain);
	for (n = 0; n < 2; n++) {
		char buf[10];

		printf("\nInitial broadside %s (grape, chain, round, double): ",
			n ? "right" : "left");
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		switch (*buf) {
		case 'g':
			load = L_GRAPE;
			break;
		case 'c':
			load = L_CHAIN;
			break;
		case 'r':
			load = L_ROUND;
			break;
		case 'd':
			load = L_DOUBLE;
			break;
		default:
			load = L_ROUND;
		}
		if (n) {
			mf->loadR = load;
			mf->readyR = R_LOADED|R_INITIAL;
		} else {
			mf->loadL = load;
			mf->readyL = R_LOADED|R_INITIAL;
		}
	}

	printf("\n");
	fflush(stdout);
	initscreen();
	display_redraw();
	snprintf(message, sizeof message, "Captain %s assuming command",
			captain);
	send_signal(ms, message);
	newturn(0);
}
