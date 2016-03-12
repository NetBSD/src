/*	$NetBSD: apply.c,v 1.19 2016/03/12 22:28:04 dholland Exp $	*/

/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
static char sccsid[] = "@(#)apply.c	8.4 (Berkeley) 4/4/94";
#else
__RCSID("$NetBSD: apply.c,v 1.19 2016/03/12 22:28:04 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static __dead void usage(void);
static int shell_system(const char *);

int
main(int argc, char *argv[])
{
	size_t clen, l;
	int ch, debug, i, magic, n, nargs, rval;
	char *c, *cmd, *p, *q, *nc;

	(void)setprogname(argv[0]);	/* for portability */

	/* Option defaults */
	debug = 0;
	magic = '%';
	nargs = -1;

	while ((ch = getopt(argc, argv, "a:d0123456789")) != -1) {
		switch (ch) {
		case 'a':
			if (optarg[1] != '\0')
				errx(EXIT_FAILURE,
				    "Illegal magic character specification.");
			magic = optarg[0];
			break;
		case 'd':
			debug = 1;
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (nargs != -1)
				errx(EXIT_FAILURE,
				    "Only one -# argument may be specified.");
			nargs = optopt - '0';
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	/*
	 * The command to run is now argv[0], and the args are argv[1+].
	 * Look for %digit references in the command, remembering the
	 * largest one.
	 */
	n = 0;
	for (p = argv[0]; p[0] != '\0'; ++p) {
		if (p[0] == magic && p[1] != '\0' &&
		    isdigit((unsigned char)p[1]) && p[1] != '0') {
			++p;
			if (p[0] - '0' > n)
				n = p[0] - '0';
		}
	}

	/*
	 * If there were any %digit references, then use those, otherwise
	 * build a new command string with sufficient %digit references at
	 * the end to consume (nargs) arguments each time round the loop.
	 * Allocate enough space to hold the maximum command.
	 */
	if ((cmd = malloc(sizeof("exec ") - 1 +
	    strlen(argv[0]) + 9 * (sizeof(" %1") - 1) + 1)) == NULL)
		err(EXIT_FAILURE, "malloc");

	if (n == 0) {
		/* If nargs not set, default to a single argument. */
		if (nargs == -1)
			nargs = 1;

		p = cmd;
		p += sprintf(cmd, "exec %s", argv[0]);
		for (i = 1; i <= nargs; i++)
			p += sprintf(p, " %c%d", magic, i);

		/*
		 * If nargs set to the special value 0, eat a single
		 * argument for each command execution.
		 */
		if (nargs == 0)
			nargs = 1;
	} else {
		(void)sprintf(cmd, "exec %s", argv[0]);
		nargs = n;
	}

	/*
	 * Grab some space in which to build the command.  Allocate
	 * as necessary later, but no reason to build it up slowly
	 * for the normal case.
	 */
	if ((c = malloc(clen = 1024)) == NULL)
		err(EXIT_FAILURE, "malloc");

	/*
	 * (argc) and (argv) are still offset by one to make it simpler to
	 * expand %digit references.  At the end of the loop check for (argc)
	 * equals 1 means that all the (argv) has been consumed.
	 */
	for (rval = 0; argc > nargs; argc -= nargs, argv += nargs) {
		/*
		 * Find a max value for the command length, and ensure
		 * there's enough space to build it.
		 */
		for (l = strlen(cmd), i = 0; i < nargs; i++)
			l += strlen(argv[i+1]);
		if (l > clen) {
			nc = realloc(c, l);
			if (nc == NULL)
				err(EXIT_FAILURE, "malloc");
			c = nc;
			clen = l;
		}

		/* Expand command argv references. */
		for (p = cmd, q = c; *p != '\0'; ++p) {
			if (p[0] == magic && isdigit((unsigned char)p[1]) &&
			    p[1] != '0')
				q += sprintf(q, "%s", argv[(++p)[0] - '0']);
			else
				*q++ = *p;
		}

		/* Terminate the command string. */
		*q = '\0';

		/* Run the command. */
		if (debug)
			(void)printf("%s\n", c);
		else if (shell_system(c))
			rval = 1;
	}

	if (argc != 1)
		errx(EXIT_FAILURE,
		    "Expecting additional argument%s after \"%s\"",
		    (nargs - argc) ? "s" : "", argv[argc - 1]);
	return rval;
}

/*
 * shell_system --
 * 	Private version of system(3).  Use the user's SHELL environment
 *	variable as the shell to execute.
 */
static int
shell_system(const char *command)
{
	static const char *name, *shell;
	int status;
	int omask;
	pid_t pid;
	sig_t intsave, quitsave;

	if (shell == NULL) {
		if ((shell = getenv("SHELL")) == NULL)
			shell = _PATH_BSHELL;
		if ((name = strrchr(shell, '/')) == NULL)
			name = shell;
		else
			++name;
	}

	if (!command) {
		/* just checking... */
		return(1);
	}

	omask = sigblock(sigmask(SIGCHLD));
	switch (pid = vfork()) {
	case -1:
		/* error */
		err(EXIT_FAILURE, "vfork");
		/*NOTREACHED*/
	case 0:
		/* child */
		(void)sigsetmask(omask);
		(void)execl(shell, name, "-c", command, (char *)NULL);
		warn("%s", shell);
		_exit(1);
		/*NOTREACHED*/
	default:
		/* parent */
		intsave = signal(SIGINT, SIG_IGN);
		quitsave = signal(SIGQUIT, SIG_IGN);
		pid = waitpid(pid, &status, 0);
		(void)sigsetmask(omask);
		(void)signal(SIGINT, intsave);
		(void)signal(SIGQUIT, quitsave);
		return pid == -1 ? -1 : status;
	}
	/*NOTREACHED*/
}

static __dead void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-a magic] [-0123456789] command arguments ...\n",
	    getprogname());
	exit(1);
}
