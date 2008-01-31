/*	$NetBSD: main.c,v 1.18 2008/01/31 05:19:44 dholland Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ed James.
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

/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: main.c,v 1.18 2008/01/31 05:19:44 dholland Exp $");
#endif
#endif /* not lint */

#include "include.h"
#include "pathnames.h"

extern FILE	*yyin;

int
main(int argc, char *argv[])
{
	unsigned long		seed;
	int			f_usage = 0, f_list = 0, f_showscore = 0;
	int			f_printpath = 0;
	const char		*file = NULL;
	int			ch;
	struct sigaction	sa;
#ifdef BSD
	struct itimerval	itv;
#endif

	/* Open the score file then revoke setgid privileges */
	open_score_file();
	(void)setgid(getgid());

	start_time = time(NULL);
	seed = start_time;

	while ((ch = getopt(argc, argv, ":u?lstpg:f:r:")) != -1) {
		switch (ch) {
		case '?':
		case 'u':
		default: 
			f_usage++;
			break;
		case 'l':
			f_list++;
			break;
		case 's':
		case 't':
			f_showscore++;
			break;
		case 'p':
			f_printpath++;
			break;
		case 'r':
			seed = atoi(optarg);
			break;
		case 'f':
		case 'g':
			file = optarg;
			break;
		}
	}
	if (optind < argc)
		f_usage++;
	srandom(seed);

	if (f_usage)
		(void)fprintf(stderr, 
		    "Usage: %s -[u?lstp] [-[gf] game_name] [-r random seed]\n",
		    argv[0]);
	if (f_showscore)
		(void)log_score(1);
	if (f_list)
		(void)list_games();
	if (f_printpath) {
		char	buf[100];

		(void)strlcpy(buf, _PATH_GAMES, 100);
		(void)puts(buf);
	}
		
	if (f_usage || f_showscore || f_list || f_printpath)
		exit(0);

	if (file == NULL)
		file = default_game();
	else
		file = okay_game(file);

	if (file == NULL || read_file(file) < 0)
		exit(1);

	init_gr();
	setup_screen(sp);

	(void)addplane();

	(void)signal(SIGINT, quit);
	(void)signal(SIGQUIT, quit);
#ifdef BSD
	(void)signal(SIGTSTP, SIG_IGN);
#endif
	(void)signal(SIGHUP, log_score_quit);
	(void)signal(SIGTERM, log_score_quit);

	(void)tcgetattr(fileno(stdin), &tty_start);
	tty_new = tty_start;
	tty_new.c_lflag &= ~(ICANON|ECHO);
	tty_new.c_iflag |= ICRNL;
	tty_new.c_cc[VMIN] = 1;
	tty_new.c_cc[VTIME] = 0;
	(void)tcsetattr(fileno(stdin), TCSADRAIN, &tty_new);

	sa.sa_handler = update;
	(void)sigemptyset(&sa.sa_mask);
	(void)sigaddset(&sa.sa_mask, SIGALRM);
	(void)sigaddset(&sa.sa_mask, SIGINT);
	sa.sa_flags = 0;
	(void)sigaction(SIGALRM, &sa, (struct sigaction *)0);

#ifdef BSD
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 1;
	itv.it_interval.tv_sec = sp->update_secs;
	itv.it_interval.tv_usec = 0;
	(void)setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
	alarm(sp->update_secs);
#endif

	for (;;) {
		if (getcommand() != 1)
			planewin();
		else {
#ifdef BSD
			itv.it_value.tv_sec = 0;
			itv.it_value.tv_usec = 0;
			(void)setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
			alarm(0);
#endif

			update(0);

#ifdef BSD
			itv.it_value.tv_sec = sp->update_secs;
			itv.it_value.tv_usec = 0;
			itv.it_interval.tv_sec = sp->update_secs;
			itv.it_interval.tv_usec = 0;
			(void)setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
			alarm(sp->update_secs);
#endif
		}
	}
}

int
read_file(const char *s)
{
	int		retval;

	filename = s;
	yyin = fopen(s, "r");
	if (yyin == NULL) {
		warn("fopen %s", s);
		return (-1);
	}
	retval = yyparse();
	(void)fclose(yyin);

	if (retval != 0)
		return (-1);
	else
		return (0);
}

const char *
default_game(void)
{
	FILE		*fp;
	static char	file[256];
	char		line[256], games[256];

	(void)strlcpy(games, _PATH_GAMES, 256);
	(void)strlcat(games, GAMES, 256);

	if ((fp = fopen(games, "r")) == NULL) {
		warn("fopen %s", games);
		return (NULL);
	}
	if (fgets(line, sizeof(line), fp) == NULL) {
		(void)fprintf(stderr, "%s: no default game available\n", games);
		fclose(fp);
		return (NULL);
	}
	(void)fclose(fp);
	line[strlen(line) - 1] = '\0';
	(void)strlcpy(file, _PATH_GAMES, 256);
	(void)strlcat(file, line, 256);
	return (file);
}

const char *
okay_game(const char *s)
{
	FILE		*fp;
	static char	file[256];
	const char	*ret = NULL;
	char		line[256], games[256];

	(void)strlcpy(games, _PATH_GAMES, 256);
	(void)strlcat(games, GAMES, 256);

	if ((fp = fopen(games, "r")) == NULL) {
		warn("fopen %s", games);
		return (NULL);
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		line[strlen(line) - 1] = '\0';
		if (strcmp(s, line) == 0) {
			(void)strlcpy(file, _PATH_GAMES, 256);
			(void)strlcat(file, line, 256);
			ret = file;
			break;
		}
	}
	(void)fclose(fp);
	if (ret == NULL) {
		test_mode = 1;
		ret = s;
		(void)fprintf(stderr, "%s: %s: game not found\n", games, s);
		(void)fprintf(stderr, "Your score will not be logged.\n");
		(void)sleep(2);	/* give the guy time to read it */
	}
	return (ret);
}

int
list_games(void)
{
	FILE		*fp;
	char		line[256], games[256];
	int		num_games = 0;

	(void)strlcpy(games, _PATH_GAMES, 256);
	(void)strlcat(games, GAMES, 256);

	if ((fp = fopen(games, "r")) == NULL) {
		warn("fopen %s", games);
		return (-1);
	}
	(void)puts("available games:");
	while (fgets(line, sizeof(line), fp) != NULL) {
		(void)printf("	%s", line);
		num_games++;
	}
	(void)fclose(fp);
	if (num_games == 0) {
		(void)fprintf(stderr, "%s: no games available\n", games);
		return (-1);
	}
	return (0);
}
