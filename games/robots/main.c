/*	$NetBSD: main.c,v 1.13 2000/01/19 19:02:27 jsm Exp $	*/

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
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: main.c,v 1.13 2000/01/19 19:02:27 jsm Exp $");
#endif
#endif /* not lint */

# include	"robots.h"

int main __P((int, char **));

int
main(ac, av)
	int	ac;
	char	**av;
{
	const char	*sp;
	bool	bad_arg;
	bool	show_only;
	extern const char	*Scorefile;
	extern int	Max_per_uid;
	int		score_wfd; /* high score writable file descriptor */
	int		score_err = 0; /* hold errno from score file open */

	score_wfd = open(Scorefile, O_RDWR);
	if (score_wfd < 0)
		score_err = errno;
	else if (score_wfd < 3)
		exit(1);

	/* Revoke setgid privileges */
	setregid(getgid(), getgid());

	show_only = FALSE;
	Num_games = 1;
	if (ac > 1) {
		bad_arg = FALSE;
		for (++av; ac > 1 && *av[0]; av++, ac--)
			if (av[0][0] != '-')
				if (isdigit(av[0][0]))
					Max_per_uid = atoi(av[0]);
				else {
					Scorefile = av[0];
					if (score_wfd >= 0)
						close(score_wfd);
					score_wfd = open(Scorefile, O_RDWR);
					if (score_wfd < 0)
						score_err = errno;
# ifdef	FANCY
					sp = strrchr(Scorefile, '/');
					if (sp == NULL)
						sp = Scorefile;
					if (strcmp(sp, "pattern_roll") == 0)
						Pattern_roll = TRUE;
					else if (strcmp(sp, "stand_still") == 0)
						Stand_still = TRUE;
					if (Pattern_roll || Stand_still)
						Teleport = TRUE;
# endif
				}
			else
				for (sp = &av[0][1]; *sp; sp++)
					switch (*sp) {
					  case 'A':
						Auto_bot = TRUE;
						break;
					  case 's':
						show_only = TRUE;
						break;
					  case 'r':
						Real_time = TRUE;
						break;
					  case 'a':
						Start_level = 4;
						break;
					  case 'n':
						Num_games++;
						break;
					  case 'j':
						Jump = TRUE;
						break;
					  case 't':
						Teleport = TRUE;
						break;
					  
					  default:
						fprintf(stderr, "robots: uknown option: %c\n", *sp);
						bad_arg = TRUE;
						break;
					}
		if (bad_arg) {
			exit(1);
			/* NOTREACHED */
		}
	}

	if (show_only) {
		show_score();
		exit(0);
		/* NOTREACHED */
	}

	if (score_wfd < 0) {
		errno = score_err;
		warn("%s", Scorefile);
		warnx("High scores will not be recorded!");
		sleep(2);
	}

	initscr();
	signal(SIGINT, quit);
	crmode();
	noecho();
	nonl();
	if (LINES != Y_SIZE || COLS != X_SIZE) {
		if (LINES < Y_SIZE || COLS < X_SIZE) {
			endwin();
			printf("Need at least a %dx%d screen\n",
			    Y_SIZE, X_SIZE);
			exit(1);
		}
		delwin(stdscr);
		stdscr = newwin(Y_SIZE, X_SIZE, 0, 0);
	}

	srand(getpid());
	if (Real_time)
		signal(SIGALRM, move_robots);
	do {
		while (Num_games--) {
			init_field();
			for (Level = Start_level; !Dead; Level++) {
				make_level();
				play_level();
				if (Auto_bot)
					sleep(1);
			}
			move(My_pos.y, My_pos.x);
			printw("AARRrrgghhhh....");
			refresh();
			if (Auto_bot)
				sleep(1);
			score(score_wfd);
			if (Auto_bot)
				sleep(1);
			refresh();
		}
	} while (!Auto_bot && another());
	quit(0);
	/* NOTREACHED */
	return(0);
}

/*
 * quit:
 *	Leave the program elegantly.
 */
void
quit(dummy)
	int dummy __attribute__((__unused__));
{
	endwin();
	exit(0);
	/* NOTREACHED */
}

/*
 * another:
 *	See if another game is desired
 */
bool
another()
{
	int	y;

#ifdef	FANCY
	if ((Stand_still || Pattern_roll) && !Newscore)
		return TRUE;
#endif

	if (query("Another game?")) {
		if (Full_clear) {
			for (y = 1; y <= Num_scores; y++) {
				move(y, 1);
				clrtoeol();
			}
			refresh();
		}
		return TRUE;
	}
	return FALSE;
}
