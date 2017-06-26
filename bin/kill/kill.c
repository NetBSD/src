/* $NetBSD: kill.c,v 1.28 2017/06/26 22:09:16 kre Exp $ */

/*
 * Copyright (c) 1988, 1993, 1994
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
#if !defined(lint) && !defined(SHELL)
__COPYRIGHT("@(#) Copyright (c) 1988, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)kill.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: kill.c,v 1.28 2017/06/26 22:09:16 kre Exp $");
#endif
#endif /* not lint */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <locale.h>
#include <sys/ioctl.h>

#ifdef SHELL            /* sh (aka ash) builtin */
int killcmd(int, char *argv[]);
#define main killcmd
#include "../../bin/sh/bltin/bltin.h"
#endif /* SHELL */ 

__dead static void nosig(const char *);
static void printsignals(FILE *);
static int signum(const char *);
static pid_t processnum(const char *);
__dead static void usage(void);

int
main(int argc, char *argv[])
{
	int errors;
	int numsig;
	pid_t pid;
	const char *sn;

	setprogname(argv[0]);
	setlocale(LC_ALL, "");
	if (argc < 2)
		usage();

	numsig = SIGTERM;

	argc--, argv++;

	/*
	 * Process exactly 1 option, if there is one.
	 */
	if (argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 'l':
			if (argv[0][2] != '\0')
				sn = argv[0] + 2;
			else {
				argc--; argv++;
				sn = argv[0];
			}
			if (argc > 1)
				usage();
			if (argc == 1) {
				if (isdigit((unsigned char)*sn) == 0)
					usage();
				numsig = signum(sn);
				if (numsig >= 128)
					numsig -= 128;
				if (numsig == 0 || signalnext(numsig) == -1)
					nosig(sn);
				sn = signalname(numsig);
				if (sn == NULL)
					errx(EXIT_FAILURE,
					   "unknown signal number: %d", numsig);
				printf("%s\n", sn);
				exit(0);
			}
			printsignals(stdout);
			exit(0);

		case 's':
			if (argv[0][2] != '\0')
				sn = argv[0] + 2;
			else {
				argc--, argv++;
				if (argc < 1) {
					warnx(
					    "option requires an argument -- s");
					usage();
				}
				sn = argv[0];
			}
			if (strcmp(sn, "0") == 0)
				numsig = 0;
			else if ((numsig = signalnumber(sn)) == 0) {
				if (sn != argv[0])
					goto trysignal;
				nosig(sn);
			}
			argc--, argv++;
			break;

		case '-':
			if (argv[0][2] == '\0') {
				/* process this one again later */
				break;
			}
			/* FALL THROUGH */
		case '\0':
			usage();
			break;

		default:
 trysignal:
			sn = *argv + 1;
			if (((numsig = signalnumber(sn)) == 0)) {
				if (isdigit((unsigned char)*sn))
					numsig = signum(sn);
				else
					nosig(sn);
			}

			if (numsig != 0 && signalnext(numsig) == -1)
				nosig(sn);
			argc--, argv++;
			break;
		}
	}

	/* deal with the optional '--' end of options option */
	if (argc > 0 && strcmp(*argv, "--") == 0)
		argc--, argv++;

	if (argc == 0)
		usage();

	for (errors = 0; argc; argc--, argv++) {
#ifdef SHELL
		extern int getjobpgrp(const char *);
		if (*argv[0] == '%') {
			pid = getjobpgrp(*argv);
			if (pid == 0) {
				warnx("illegal job id: %s", *argv);
				errors = 1;
				continue;
			}
		} else 
#endif
			if ((pid = processnum(*argv)) == (pid_t)-1) {
				errors = 1;
				continue;
			}

		if (kill(pid, numsig) == -1) {
			warn("%s", *argv);
			errors = 1;
		}
#ifdef SHELL
		/*
		 * Wakeup the process if it was suspended, so it can
		 * exit without an explicit 'fg'.
		 *	(kernel handles this for SIGKILL)
		 */
		if (numsig == SIGTERM || numsig == SIGHUP)
			kill(pid, SIGCONT);
#endif
	}

	exit(errors);
	/* NOTREACHED */
}

static int
signum(const char *sn)
{
	intmax_t n;
	char *ep;

	n = strtoimax(sn, &ep, 10);

	/* check for correctly parsed number */
	if (*ep || n <= INT_MIN || n >= INT_MAX )
		errx(EXIT_FAILURE, "illegal signal number: %s", sn);
		/* NOTREACHED */

	return (int)n;
}

static pid_t
processnum(const char *s)
{
	intmax_t n;
	char *ep;

	n = strtoimax(s, &ep, 10);

	/* check for correctly parsed number */
	if (*ep || n == INTMAX_MIN || n == INTMAX_MAX || (pid_t)n != n ||
	    n == -1) {
		warnx("illegal process%s id: %s", (n < 0 ? " group" : ""), s);
		n = -1;
	}

	return (pid_t)n;
}

static void
nosig(const char *name)
{

	warnx("unknown signal %s; valid signals:", name);
	printsignals(stderr);
	exit(1);
	/* NOTREACHED */
}

static void
printsignals(FILE *fp)
{
	int sig;
	int len, nl, pad;
	const char *name;
	int termwidth = 80;

	if ((name = getenv("COLUMNS")) != 0)
		termwidth = atoi(name);
	else if (isatty(fileno(fp))) {
		struct winsize win;

		if (ioctl(fileno(fp), TIOCGWINSZ, &win) == 0 && win.ws_col > 0)
			termwidth = win.ws_col;
	}

	for (pad = 0, len = 0, sig = 0; (sig = signalnext(sig)) != 0; ) {
		name = signalname(sig);
		if (name == NULL)
			continue;

		nl = strlen(name);

		if (len > 0 && nl + len + pad >= termwidth) {
			fprintf(fp, "\n");
			len = 0;
			pad = 0;
		} else if (pad > 0 && len != 0)
			fprintf(fp, "%*s", pad, "");
		else
			pad = 0;

		len += nl + pad;
		pad = (nl | 7) + 1 - nl;

		fprintf(fp, "%s", name);
	}
	if (len != 0)
		fprintf(fp, "\n");
}

static void
usage(void)
{
	const char *pn = getprogname();

	fprintf(stderr, "usage: %s [-s signal_name] pid ...\n"
			"       %s -l [exit_status]\n"
			"       %s -signal_name pid ...\n"
			"       %s -signal_number pid ...\n",
	    pn, pn, pn, pn);
	exit(1);
	/* NOTREACHED */
}
