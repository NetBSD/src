/*	$NetBSD: score.c,v 1.22 2009/08/03 06:04:12 dholland Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
static char sccsid[] = "@(#)score.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: score.c,v 1.22 2009/08/03 06:04:12 dholland Exp $");
#endif
#endif /* not lint */

#include <curses.h>
#include <err.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "robots.h"
#include "pathnames.h"

const char *Scorefile = _PATH_SCORE;

int Max_per_uid = MAX_PER_UID;

static SCORE Top[MAXSCORES];

static uint32_t numscores, max_uid;

/*
 * read_score:
 *	Read the score file in MI format
 */
static void
read_score(int inf)
{
	SCORE *scp;

	if (read(inf, &max_uid, sizeof max_uid) == sizeof max_uid &&
	    read(inf, Top, sizeof Top) == sizeof Top) {
		max_uid = ntohl(max_uid);
		for (scp = Top; scp < &Top[MAXSCORES]; scp++) {
			 scp->s_uid = ntohl(scp->s_uid);
			 scp->s_score = ntohl(scp->s_score);
			 scp->s_auto = ntohl(scp->s_auto);
			 scp->s_level = ntohl(scp->s_level);
		}
	}
	else {
		for (scp = Top; scp < &Top[MAXSCORES]; scp++)
			scp->s_score = 0;
		max_uid = Max_per_uid;
	}
}

/*
 * write_score:
 *	Write the score file in MI format
 */
static void
write_score(int inf)
{
	SCORE *scp;

	lseek(inf, 0L, SEEK_SET);

	max_uid = htonl(max_uid);
	write(inf, &max_uid, sizeof max_uid);

	for (scp = Top; scp < &Top[MAXSCORES]; scp++) {
		 scp->s_uid = htonl(scp->s_uid);
		 scp->s_score = htonl(scp->s_score);
		 scp->s_auto = htonl(scp->s_auto);
		 scp->s_level = htonl(scp->s_level);
	}

	write(inf, Top, sizeof Top);
}


/*
 * score:
 *	Post the player's score, if reasonable, and then print out the
 *	top list.
 */
void
score(int score_wfd)
{
	int inf = score_wfd;
	SCORE *scp;
	uint32_t uid;
	bool done_show = false;

	Newscore = false;
	if (inf < 0)
		return;

	read_score(inf);

	uid = getuid();
	if (Top[MAXSCORES-1].s_score <= Score) {
		numscores = 0;
		for (scp = Top; scp < &Top[MAXSCORES]; scp++)
			if ((scp->s_uid == uid && ++numscores == max_uid)) {
				if (scp->s_score > Score)
					break;
				scp->s_score = Score;
				scp->s_uid = uid;
				scp->s_auto = Auto_bot;
				scp->s_level = Level;
				set_name(scp);
				Newscore = true;
				break;
			}
		if (scp == &Top[MAXSCORES]) {
			Top[MAXSCORES-1].s_score = Score;
			Top[MAXSCORES-1].s_uid = uid;
			Top[MAXSCORES-1].s_auto = Auto_bot;
			Top[MAXSCORES-1].s_level = Level;
			set_name(&Top[MAXSCORES-1]);
			Newscore = true;
		}
		if (Newscore)
			qsort(Top, MAXSCORES, sizeof Top[0], cmp_sc);
	}

	if (!Newscore) {
		Full_clear = false;
		lseek(inf, 0, SEEK_SET);
		return;
	}
	else
		Full_clear = true;

	move(1, 15);
	printw("%5.5s %5.5s %-9.9s %-8.8s %5.5s", "Rank", "Score", "User",
	    " ", "Level");

	for (scp = Top; scp < &Top[MAXSCORES]; scp++) {
		if (scp->s_score == 0)
			break;
		move((scp - Top) + 2, 15);
		if (!done_show && scp->s_uid == uid && scp->s_score == Score)
			standout();
		printw("%5ld %5d %-8.8s %-9.9s %5d",
		    (long)(scp - Top) + 1, scp->s_score, scp->s_name,
		    scp->s_auto ? "(autobot)" : "", scp->s_level);
		if (!done_show && scp->s_uid == uid && scp->s_score == Score) {
			standend();
			done_show = true;
		}
	}
	Num_scores = scp - Top;
	refresh();

	if (Newscore) {
		write_score(inf);
	}
	lseek(inf, 0, SEEK_SET);
}

void
set_name(SCORE *scp)
{
	struct passwd *pp;

	if ((pp = getpwuid(scp->s_uid)) == NULL)
		strncpy(scp->s_name, "???", MAXNAME);
	else
		strncpy(scp->s_name, pp->pw_name, MAXNAME);
}

/*
 * cmp_sc:
 *	Compare two scores.
 */
int
cmp_sc(const void *s1, const void *s2)
{
	return ((const SCORE *)s2)->s_score - ((const SCORE *)s1)->s_score;
}

/*
 * show_score:
 *	Show the score list for the '-s' option.
 */
void
show_score(void)
{
	SCORE *scp;
	int inf;

	if ((inf = open(Scorefile, O_RDONLY)) < 0) {
		warn("opening `%s'", Scorefile);
		return;
	}

	read_score(inf);
	close(inf);
	inf = 1;
	printf("%5.5s %5.5s %-9.9s %-8.8s %5.5s\n", "Rank", "Score", "User",
	    " ", "Level");
	for (scp = Top; scp < &Top[MAXSCORES]; scp++)
		if (scp->s_score > 0)
			printf("%5d %5d %-8.8s %-9.9s %5d\n",
			    inf++, scp->s_score, scp->s_name,
			    scp->s_auto ? "(autobot)" :  "", scp->s_level);
}
