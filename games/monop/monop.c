/*	$NetBSD: monop.c,v 1.12 2001/09/18 18:15:49 wiz Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)monop.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: monop.c,v 1.12 2001/09/18 18:15:49 wiz Exp $");
#endif
#endif /* not lint */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "monop.def"

int main __P((int, char *[]));
static void getplayers __P((void));
static void init_players __P((void));
static void init_monops __P((void));
static void do_quit __P((int));

/*
 *	This program implements a monopoly game
 */
int
main(ac, av)
	int ac;
	char *av[];
{
	/* Revoke setgid privileges */
	setgid(getgid());

	srand(getpid());
	if (ac > 1) {
		if (!rest_f(av[1]))
			restore();
	}
	else {
		getplayers();
		init_players();
		init_monops();
	}
	num_luck = sizeof lucky_mes / sizeof (char *);
	init_decks();
	signal(SIGINT, do_quit);
	for (;;) {
		printf("\n%s (%d) (cash $%d) on %s\n", cur_p->name, player + 1,
			cur_p->money, board[cur_p->loc].name);
		printturn();
		force_morg();
		execute(getinp("-- Command: ", comlist));
	}
}

/*ARGSUSED*/
static void
do_quit(n)
	int n __attribute__((__unused__));
{
	quit();
}

/*
 *	This routine gets the names of the players
 */
static void
getplayers()
{
	char *sp;
	int i, j;
	char buf[257];

blew_it:
	for (;;) {
		if ((num_play=get_int("How many players? ")) <= 0 ||
		    num_play > MAX_PL)
			printf("Sorry. Number must range from 1 to 9\n");
		else
			break;
	}
	cur_p = play = (PLAY *) calloc(num_play, sizeof (PLAY));
	if (play == NULL)
		err(1, NULL);
	for (i = 0; i < num_play; i++) {
over:
		printf("Player %d's name: ", i + 1);
		for (sp = buf; (*sp=getchar()) != '\n'; sp++)
			continue;
		if (sp == buf)
			goto over;
		*sp++ = '\0';
		name_list[i] = play[i].name = (char *)calloc(1, sp - buf);
		if (name_list[i] == NULL)
			err(1, NULL);
		strcpy(play[i].name, buf);
		play[i].money = 1500;
	}
	name_list[i++] = "done";
	name_list[i] = 0;
	for (i = 0; i < num_play; i++)
		for (j = i + 1; j < num_play; j++)
			if (strcasecmp(name_list[i], name_list[j]) == 0) {
				if (i != num_play - 1)
					printf("Hey!!! Some of those are "
					    "IDENTICAL!!  Let's try that "
					    "again....\n");
				else
					printf("\"done\" is a reserved word.  "
					    "Please try again\n");
				for (i = 0; i < num_play; i++)
					free(play[i].name);
				free(play);
				goto blew_it;
			}
}

/*
 *	This routine figures out who goes first
 */
static void
init_players()
{
	int i, rl, cur_max;
	bool over = 0;
	int max_pl = 0;

again:
	putchar('\n');
	for (cur_max = i = 0; i < num_play; i++) {
		printf("%s (%d) rolls %d\n", play[i].name, i+1, rl=roll(2, 6));
		if (rl > cur_max) {
			over = FALSE;
			cur_max = rl;
			max_pl = i;
		}
		else if (rl == cur_max)
			over++;
	}
	if (over) {
		printf("%d people rolled the same thing, so we'll try again\n",
		    over + 1);
		goto again;
	}
	player = max_pl;
	cur_p = &play[max_pl];
	printf("%s (%d) goes first\n", cur_p->name, max_pl + 1);
}

/*
 *	This routine initializes the monopoly structures.
 */
static void
init_monops() 
{
	MON *mp;
	int i;

	for (mp = mon; mp < &mon[N_MON]; mp++) {
		mp->name = mp->not_m;
		for (i = 0; i < mp->num_in; i++)
			mp->sq[i] = &board[mp->sqnums[i]];
	}
}
