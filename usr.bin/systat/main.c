/*	$NetBSD: main.c,v 1.41 2007/12/31 00:22:15 christos Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)main.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: main.c,v 1.41 2007/12/31 00:22:15 christos Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "systat.h"
#include "extern.h"

static struct nlist namelist[] = {
#define X_FIRST		0
#define	X_HZ		0
	{ .n_name = "_hz" },
#define	X_STATHZ		1
	{ .n_name = "_stathz" },
#define	X_MAXSLP		2
	{ .n_name = "_maxslp" },
	{ .n_name = NULL }
};
static int     dellave;

kvm_t *kd;
char	*memf = NULL;
char	*nlistf = NULL;
sig_t	sigtstpdfl;
double avenrun[3];
int     col;
int	naptime = 5;
int     verbose = 1;                    /* to report kvm read errs */
int     hz, stathz, maxslp;
char    c;
char    *namp;
char    hostname[MAXHOSTNAMELEN + 1];
WINDOW  *wnd;
int     CMDLINE;
int     turns = 2;	/* stay how many refresh-turns in 'all' mode? */
int     allflag;
int     allcounter;
sig_atomic_t needsredraw = 0;

static	WINDOW *wload;			/* one line window for load average */

static void (*sv_stop_handler)(int);

static void stop(int);
static void usage(void);
int main(int, char **);

gid_t egid; /* XXX needed by initiostat() and initkre() */

int
main(int argc, char **argv)
{
	int ch;
	char errbuf[_POSIX2_LINE_MAX];

	egid = getegid();
	(void)setegid(getgid());

	while ((ch = getopt(argc, argv, "M:N:nw:t:")) != -1)
		switch(ch) {
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			nflag = !nflag;
			break;
		case 'w':
			if ((naptime = atoi(optarg)) <= 0)
				errx(1, "interval <= 0.");
			break;
		case 't':
			if ((turns = atoi(optarg)) <= 0)
				errx(1, "turns <= 0.");
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;


	for ( ; argc > 0; argc--, argv++) {
		struct mode *p;
		int modefound = 0;

		if (isdigit((unsigned char)argv[0][0])) {
			naptime = atoi(argv[0]);
			if (naptime <= 0)
				naptime = 5;
			continue;
		}

		for (p = modes; p->c_name ; p++) {
			if (strstr(p->c_name, argv[0]) == p->c_name) {
				curmode = p;
				modefound++;
				break;
			}

			if(strstr("all",argv[0]) == "all"){
				allcounter=0;
				allflag=1;
			}
		}


		if (!modefound && !allflag)
			error("%s: Unknown command.", argv[0]);
	}

	/*
	 * Discard setgid privileges.  If not the running kernel, we toss
	 * them away totally so that bad guys can't print interesting stuff
	 * from kernel memory, otherwise switch back to kmem for the
	 * duration of the kvm_openfiles() call.
	 */
	if (nlistf != NULL || memf != NULL)
		(void)setgid(getgid());
	else
		(void)setegid(egid);

	kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf);
	if (kd == NULL) {
		error("%s", errbuf);
		exit(1);
	}

	/* Get rid of privs for now. */
	if (nlistf == NULL && memf == NULL)
		(void)setegid(getgid());

	if (kvm_nlist(kd, namelist)) {
		if (nlistf)
			errx(1, "%s: no namelist", nlistf);
		else
			errx(1, "no namelist");
	}

	signal(SIGINT, die);
	signal(SIGQUIT, die);
	signal(SIGTERM, die);
	sv_stop_handler = signal(SIGTSTP, stop);

	/*
	 * Initialize display.  Load average appears in a one line
	 * window of its own.  Current command's display appears in
	 * an overlapping sub-window of stdscr configured by the display
	 * routines to minimize update work by curses.
	 */
	if (initscr() == NULL)
	{
		warnx("couldn't initialize screen");
		exit(0);
	}

	CMDLINE = LINES - 1;
	wnd = (*curmode->c_open)();
	if (wnd == NULL) {
		warnx("couldn't initialize display");
		die(0);
	}
	wload = newwin(1, 0, 3, 20);
	if (wload == NULL) {
		warnx("couldn't set up load average window");
		die(0);
	}
	gethostname(hostname, sizeof (hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	NREAD(X_HZ, &hz, sizeof hz);
	NREAD(X_STATHZ, &stathz, sizeof stathz);
	NREAD(X_MAXSLP, &maxslp, sizeof maxslp);
	(*curmode->c_init)();
	curmode->c_flags |= CF_INIT;
	labels();

	dellave = 0.0;

	display(0);
	noecho();
	cbreak();
	keyboard();
	/*NOTREACHED*/
}

static void
usage(void)
{
	fprintf(stderr, "usage: systat [-n] [-M core] [-N system] [-w wait] "
		"[-t turns]\n\t\t[display] [refresh-interval]\n");
	exit(1);
}


void
labels(void)
{
	if (curmode->c_flags & CF_LOADAV) {
		mvaddstr(2, 20,
		    "/0   /1   /2   /3   /4   /5   /6   /7   /8   /9   /10");
		mvaddstr(3, 5, "Load Average");
	}
	(*curmode->c_label)();
#ifdef notdef
	mvprintw(21, 25, "CPU usage on %s", hostname);
#endif
	refresh();
}

void
display(int signo)
{
	int j;
	struct mode *p;

	/* Get the load average over the last minute. */
	(void)getloadavg(avenrun, sizeof(avenrun) / sizeof(avenrun[0]));
	(*curmode->c_fetch)();
	if (curmode->c_flags & CF_LOADAV) {
		j = 5.0*avenrun[0] + 0.5;
		dellave -= avenrun[0];
		if (dellave >= 0.0)
			c = '<';
		else {
			c = '>';
			dellave = -dellave;
		}
		if (dellave < 0.1)
			c = '|';
		dellave = avenrun[0];
		wmove(wload, 0, 0);
		wclrtoeol(wload);
		whline(wload, c, (j > 50) ? 50 : j);
		if (j > 50)
			wprintw(wload, " %4.1f", avenrun[0]);
	}
	(*curmode->c_refresh)();
	if (curmode->c_flags & CF_LOADAV)
		wrefresh(wload);
	wrefresh(wnd);
	move(CMDLINE, col);
	refresh();
	
	if (allflag && signo==SIGALRM) {
		if (allcounter >= turns){
			p = curmode;
			p++;
			if (p->c_name == NULL)
				p = modes;
			switch_mode(p);
			allcounter=0;
		} else
			allcounter++;
       }

	timeout(naptime * 1000);
}

void
redraw(void)
{
	resizeterm(LINES, COLS);
	CMDLINE = LINES - 1;
	labels();

	display(0);
	needsredraw = 0;
}

static void
stop(int signo)
{
	sigset_t set;

	signal(SIGTSTP, sv_stop_handler);
	/* unblock SIGTSTP */
	sigemptyset(&set);
	sigaddset(&set, SIGTSTP);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	/* stop ourselves */
	kill(0, SIGTSTP);
	/* must have been restarted */
	signal(SIGTSTP, stop);
	needsredraw = signo;
}

void
die(int signo)
{
	move(CMDLINE, 0);
	clrtoeol();
	refresh();
	endwin();
	exit(0);
}

void
error(const char *fmt, ...)
{
	va_list ap;
	char buf[255];
	int oy, ox;

	va_start(ap, fmt);

	if (wnd) {
		getyx(stdscr, oy, ox);
		(void) vsnprintf(buf, sizeof(buf), fmt, ap);
		clrtoeol();
		standout();
		mvaddstr(CMDLINE, 0, buf);
		standend();
		move(oy, ox);
		refresh();
	} else {
		(void) vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
	}
	va_end(ap);
}

void
nlisterr(struct nlist name_list[])
{
	int i, n;

	n = 0;
	clear();
	mvprintw(2, 10, "systat: nlist: can't find following symbols:");
	for (i = 0;
	    name_list[i].n_name != NULL && *name_list[i].n_name != '\0'; i++)
		if (name_list[i].n_value == 0)
			mvprintw(2 + ++n, 10, "%s", name_list[i].n_name);
	move(CMDLINE, 0);
	clrtoeol();
	refresh();
	sleep(5);
	endwin();
	exit(1);
}
