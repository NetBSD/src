/*	$NetBSD: env.c,v 1.23.8.1 2024/11/01 14:39:50 martin Exp $	*/
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
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1988, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "@(#)env.c	8.3 (Berkeley) 4/2/94";*/
__RCSID("$NetBSD: env.c,v 1.23.8.1 2024/11/01 14:39:50 martin Exp $");
#endif /* not lint */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>

static void usage(void) __dead;

extern char **environ;

int
main(int argc, char **argv)
{
	char **ep, term;
	char *cleanenv[1];
	int ch;

	setprogname(*argv);
	(void)setlocale(LC_ALL, "");

	term = '\n';
	while ((ch = getopt(argc, argv, "-0C:iu:")) != -1)
		switch((char)ch) {
		case '0':
			term = '\0';
			break;
		case 'C':
			if (chdir(optarg) == -1)
				err(EXIT_FAILURE, "chdir '%s'", optarg);
			break;
		case '-':			/* obsolete */
		case 'i':
			environ = cleanenv;
			cleanenv[0] = NULL;
			break;
		case 'u':
			if (unsetenv(optarg) == -1)
				err(EXIT_FAILURE, "unsetenv '%s'", optarg);
			break;
		case '?':
		default:
			usage();
		}

	for (argv += optind; *argv && strchr(*argv, '=') != NULL; ++argv)
		(void)putenv(*argv);

	if (*argv) {
		/* return 127 if the command to be run could not be found; 126
		   if the command was found but could not be invoked; 125 if
		   -0 was specified with utility.*/

		if (term == '\0')
			errx(125, "cannot specify command with -0");

		(void)execvp(*argv, argv);
		err((errno == ENOENT) ? 127 : 126, "%s", *argv);
		/* NOTREACHED */
	}

	for (ep = environ; *ep; ep++)
		(void)printf("%s%c", *ep, term);

	exit(0);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-0i] [-C dir] [-u name] [name=value ...] [command]\n",
	    getprogname());
	exit(1);
}
