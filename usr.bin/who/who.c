/*	$NetBSD: who.c,v 1.11 2003/02/17 13:56:28 christos Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
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
__COPYRIGHT(
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)who.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: who.c,v 1.11 2003/02/17 13:56:28 christos Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "utmpentry.h"

static void output_labels(void);
static void who_am_i(const char *, int);
static void usage(void);
static void process(const char *, int);
static void print(const char *, const char *, time_t, const char *);

int main(int, char **);

static int show_term;			/* show term state */
static int show_idle;			/* show idle time */

extern int maxname, maxline, maxhost;

int
main(int argc, char **argv)
{
	int c, only_current_term, show_labels;

	setlocale(LC_ALL, "");

	only_current_term = show_term = show_idle = show_labels = 0;
	while ((c = getopt(argc, argv, "mTuH")) != -1) {
		switch (c) {
		case 'm':
			only_current_term = 1;
			break;
		case 'T':
			show_term = 1;
			break;
		case 'u':
			show_idle = 1;
			break;
		case 'H':
			show_labels = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (chdir("/dev")) {
		err(1, "cannot change directory to /dev");
		/* NOTREACHED */
	}

	switch (argc) {
	case 0:					/* who */
		if (only_current_term) {
			who_am_i(NULL, show_labels);
		} else {
			process(NULL, show_labels);
		}
		break;
	case 1:					/* who utmp_file */
		if (only_current_term) {
			who_am_i(*argv, show_labels);
		} else {
			process(*argv, show_labels);
		}
		break;
	case 2:					/* who am i */
		who_am_i(NULL, show_labels);
		break;
	default:
		usage();
		/* NOTREACHED */
	}
	exit(0);
}

static void
who_am_i(const char *fname, int show_labels)
{
	struct passwd *pw;
	char *p;
	char *t;
	time_t now;
	struct utmpentry *ehead, *ep;

	/* search through the utmp and find an entry for this tty */
	if ((p = ttyname(STDIN_FILENO)) != NULL) {

		/* strip any directory component */
		if ((t = strrchr(p, '/')) != NULL)
			p = t + 1;

		(void)getutentries(fname, &ehead);
		for (ep = ehead; ep; ep = ep->next)
			if (strcmp(ep->line, p) == 0) {
				if (show_labels)
					output_labels();
				print(ep->name, ep->line, (time_t)ep->tv.tv_sec,
				    ep->host);
				return;
			}
	} else
		p = "tty??";

	(void)time(&now);
	pw = getpwuid(getuid());
	if (show_labels)
		output_labels();
	print(pw ? pw->pw_name : "?", p, now, "");
}

static void
process(const char *fname, int show_labels)
{
	struct utmpentry *ehead, *ep;
	(void)getutentries(fname, &ehead);
	if (show_labels)
		output_labels();
	for (ep = ehead; ep != NULL; ep = ep->next)
		print(ep->name, ep->line, (time_t)ep->tv.tv_sec, ep->host);
}

static void
print(const char *name, const char *line, time_t t, const char *host)
{
	struct stat sb;
	char state;
	static time_t now = 0;
	time_t idle;

	state = '?';
	idle = 0;

	if (show_term || show_idle) {
		if (now == 0)
			time(&now);
		
		if (stat(line, &sb) == 0) {
			state = (sb.st_mode & 020) ? '+' : '-';
			idle = now - sb.st_atime;
		}
		
	}

	(void)printf("%-*.*s ", maxname, maxname, name);

	if (show_term) {
		(void)printf("%c ", state);
	}

	(void)printf("%-*.*s ", maxline, maxline, line);
	(void)printf("%.12s ", ctime(&t) + 4);

	if (show_idle) {
		if (idle < 60) 
			(void)printf("  .   ");
		else if (idle < (24 * 60 * 60))
			(void)printf("%02ld:%02ld ", 
				     (long)(idle / (60 * 60)),
				     (long)(idle % (60 * 60)) / 60);
		else
			(void)printf(" old  ");
	}
	
	if (*host)
		printf("\t(%.*s)", maxhost, host);
	(void)putchar('\n');
}

static void
output_labels()
{
	(void)printf("%-*.*s ", maxname, maxname, "USER");

	if (show_term)
		(void)printf("S ");
	
	(void)printf("%-*.*s ", maxline, maxline, "LINE");
	(void)printf("WHEN         ");

	if (show_idle)
		(void)printf("IDLE  ");
	
	(void)printf("\t%.*s", maxhost, "FROM");

	(void)putchar('\n');
}

static void
usage()
{
	(void)fprintf(stderr, "Usage: %s [-mTuH] [ file ]\n       %s am i\n",
	    getprogname(), getprogname());
	exit(1);
}
