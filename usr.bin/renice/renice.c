/*	$NetBSD: renice.c,v 1.6 1998/12/19 21:07:12 christos Exp $	*/

/*
 * Copyright (c) 1983, 1989, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)renice.c	8.1 (Berkeley) 6/9/93";*/
__RCSID("$NetBSD: renice.c,v 1.6 1998/12/19 21:07:12 christos Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <err.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int	getnum __P((const char *, const char *, int *));
static int	donice __P((int, int, int, int));
static void	usage __P((void)) __attribute__((__noreturn__));

int	main __P((int, char **));

/*
 * Change the priority (nice) of processes
 * or groups of processes which are already
 * running.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	int which = PRIO_PROCESS;
	int who = 0, prio, errs = 0, incr = 0;

	argc--, argv++;
	if (argc < 2)
		usage();
	if (strcmp(*argv, "-n") == 0) {
		incr = 1;
		argc--, argv++;
		if (argc == 0)
			usage();
	}
	if (getnum("priority", *argv, &prio))
		return 1;
	argc--, argv++;
	for (; argc > 0; argc--, argv++) {
		if (strcmp(*argv, "-g") == 0) {
			which = PRIO_PGRP;
			continue;
		}
		if (strcmp(*argv, "-u") == 0) {
			which = PRIO_USER;
			continue;
		}
		if (strcmp(*argv, "-p") == 0) {
			which = PRIO_PROCESS;
			continue;
		}
		if (which == PRIO_USER) {
			struct passwd *pwd = getpwnam(*argv);
			
			if (pwd == NULL) {
				warnx("%s: unknown user", *argv);
				continue;
			}
			who = pwd->pw_uid;
		} else {
			if (getnum("pid", *argv, &who))
				continue;
			if (who < 0) {
				warnx("%s: bad value", *argv);
				continue;
			}
		}
		errs += donice(which, who, prio, incr);
	}
	exit(errs != 0);
}

static int
getnum(com, str, val)
	const char *com, *str;
	int *val;
{
	long v;
	char *ep;

	v = strtol(str, &ep, NULL);

	if (*ep) {
		warnx("Bad %s argument: %s", com, str);
		return 1;
	}
	if ((v == LONG_MIN || v == LONG_MAX) && errno == ERANGE) {
		warn("Invalid %s argument: %s", com, str);
		return 1;
	}

	*val = (int)v;
	return 0;
}

static int
donice(which, who, prio, incr)
	int which, who, prio, incr;
{
	int oldprio;

	if ((oldprio = getpriority(which, who)) == -1) {
		warn("%d: getpriority", who);
		return (1);
	}

	if (incr)
		prio = oldprio + prio;

	if (prio > PRIO_MAX)
		prio = PRIO_MAX;
	if (prio < PRIO_MIN)
		prio = PRIO_MIN;

	if (setpriority(which, who, prio) == -1) {
		warn("%d: setpriority", who);
		return (1);
	}
	printf("%d: old priority %d, new priority %d\n", who, oldprio, prio);
	return (0);
}

static void
usage()
{
	extern char *__progname;

	(void)fprintf(stderr, "Usage: %s [<priority> | -n <incr>] ",
	    __progname);
	(void)fprintf(stderr, "[[-p] <pids>...] [-g <pgrp>...] ");
	(void)fprintf(stderr, "[-u <user>...]\n");
	exit(1);
}
