/*	$NetBSD: env.c,v 1.25 2025/02/09 14:25:26 kre Exp $	*/
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
__RCSID("$NetBSD: env.c,v 1.25 2025/02/09 14:25:26 kre Exp $");
#endif /* not lint */

#include <sys/stat.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include <paths.h>

static void usage(void) __dead;
static const char *path_find(const char *);

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
				err(125, "chdir '%s'", optarg);
			/* better to do this than have it be invalid */
			(void)unsetenv("PWD");
			break;
		case '-':			/* obsolete */
		case 'i':
			environ = cleanenv;
			cleanenv[0] = NULL;
			break;
		case 'u':
			if (unsetenv(optarg) == -1)
				err(125, "unsetenv '%s'", optarg);
			break;
		case '?':
		default:
			usage();
		}

	for (argv += optind; *argv && strchr(*argv, '=') != NULL; ++argv)
		if (putenv(*argv) == -1)
			err(125, "putenv '%s'", *argv);

	/*
	 * Allow an extra "--" to allow utility names to contain '=' chars
	 */
	if (*argv && strcmp(*argv, "--") == 0)
		argv++;

	if (!*argv) {
		/*
		 * No utility name is present, simply dump the environment
		 * to stdout, and we're done.
		 */
		for (ep = environ; *ep; ep++)
			(void)printf("%s%c", *ep, term);

		(void)fflush(stdout);
		if (ferror(stdout))
			err(125, "write to standard output");

		exit(0);
	}

	/*
	 * Run the utility, if this succeeds, it doesn't return,
	 * and we no longer exist!   No need to check for errors,
	 * if we have the opportunity to do so, there must be one.
	 */
	(void)execvp(*argv, argv);

	/*
	 * Return 127 if the command to be run could not be found;
	 * or 126 if the command was found but could not be invoked.
	 *
	 * Working out which happened is hard, and impossible to
	 * truly get correct, but let's try.
	 *
	 * First we need to discover if the utility exists, that
	 * means duplicating much of the work of execvp() without
	 * the actual exec attempt.
	 */

	if (path_find(*argv) == NULL)	/* Could not be found */
		err(127, "%s", *argv);

	/*
	 * We could (should) free the return value from path_find()
	 * if it is not identical to *argv, but since all we are
	 * going to do is exit anyway, why bother?
	 */

	/*
	 * The file does exist, so the "could not be found" is
	 * false, this must be the 126 exit case.
	 */

	err(126, "%s", *argv);
	/* NOTREACHED */
}

/*
 * search for name in directories given in env var PATH
 *
 * return the location found, or none if there is none.
 * (note this return value is either NULL, or == name,
 * or is a pointer to malloc()'d memory)
 *
 * The value of errno on entry *must* be preserved.
 */
static const char *
path_find(const char *name)
{
	int e = errno;
	struct stat sb;
	const char *path;
	const char *firstfound = NULL;

	if (strchr(name, '/') != NULL) {
		/*
		 * name contains a '/', no PATH search,
		 * just work out if it exists or not.
		 *
		 * nb: stat, not lstat, the thing must
		 * resolve to something we can exec(),
		 * not just a symlink.
		 */
		if (stat(name, &sb) == -1) {
			errno = e;
			return NULL;
		}
		errno = e;
		return name;
	}

	/* borrow the outline of this loop from execvp() */

	path = getenv("PATH");
	if (path == NULL)
		path = _PATH_DEFPATH;

	do {
		const char *p;
		char *pathname;
		ptrdiff_t lp;

		/* Find the end of this path element. */
		for (p = path; *path != 0 && *path != ':'; path++)
			continue;
		/*
		 * It's a SHELL path -- double, leading and trailing colons
		 * mean the current directory.
		 */
		if (p == path) {
			p = ".";
			lp = 1;
		} else
			lp = path - p;

		if (asprintf(&pathname, "%.*s/%s", (int)lp, p, name) == -1) {
			/*
			 * This is very unlikely, and usually means ENOMEM
			 * from malloc() - just give up, and say the file could
			 * not be found (which is more or less correct, given
			 * a slightly slanted view on what just happened).
			 */
			errno = e;
			return NULL;
		}

		if (stat(pathname, &sb) == -1) {
			free(pathname);
			continue;
		}

		if ((sb.st_mode & 0111) != 0) {
			/*
			 * We located an existing file with
			 * the correct name in PATH, and it
			 * has (someone's) execute permission.
			 *
			 * Done.
			 */
			errno = e;
			return (const char *)pathname;
		}

		/*
		 * No execute permission, but a file is located.
		 * Continue looking for one which does have execute
		 * permission, which is how execvp() works,
		 * more or less.   (Close enough for our purposes).
		 */

		if (firstfound == NULL)
			firstfound = pathname;
		else
			free(pathname);

	} while (*path++ == ':');	/* Otherwise, *path was '\0' */

	errno = e;
	return firstfound;
}

static void
usage(void)
{
	const char *me = getprogname();
	int howwide = (int)strlen(me);

	howwide += 7; /* "Usage: " */

	(void)fprintf(stderr,
	    "\nUsage: %s %s \\\n%*s %s \\\n%*s %s\n",
	        me, "[-0i] [-C dir] [-u name] [--]",
	        howwide, "", "[name=value ...] [--]",
	        howwide, "", "[utility [arg ...] ]");

	exit(125);
}
