/* $NetBSD: ln.c,v 1.26 2003/09/14 19:20:21 jschauma Exp $ */

/*
 * Copyright (c) 1987, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1987, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ln.c	8.2 (Berkeley) 3/31/94";
#else
__RCSID("$NetBSD: ln.c,v 1.26 2003/09/14 19:20:21 jschauma Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

int	fflag;				/* Unlink existing files. */
int	hflag;				/* Check new name for symlink first. */
int	sflag;				/* Symbolic, not hard, link. */
int	vflag;                          /* Verbose output */
int	stdout_ok;			/* stdout connected to a terminal */

					/* System link call. */
int (*linkf)(const char *, const char *);
char   linkch;

int	linkit(char *, char *, int);
void	usage(void);
int	main(int, char *[]);

int
main(int argc, char *argv[])
{
	struct stat sb;
	int ch, exitval;
	char *sourcedir;

	setprogname(argv[0]);
	while ((ch = getopt(argc, argv, "fhnsv")) != -1)
		switch (ch) {
		case 'f':
			fflag = 1;
			break;
		case 'h':
		case 'n':
			hflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'v':               
		vflag = 1;      
		break;   
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}

	argv += optind;
	argc -= optind;

	if (sflag) {
		linkf  = symlink;
		linkch = '-';
	} else {
		linkf  = link;
		linkch = '=';
	}

	switch(argc) {
	case 0:
		usage();
		/* NOTREACHED */
	case 1:				/* ln target */
		exit(linkit(argv[0], ".", 1));
		/* NOTREACHED */
	case 2:				/* ln target source */
		exit(linkit(argv[0], argv[1], 0));
		/* NOTREACHED */
	}

	stdout_ok = isatty(STDOUT_FILENO);

					/* ln target1 target2 directory */
	sourcedir = argv[argc - 1];
	if (hflag && lstat(sourcedir, &sb) == 0 && S_ISLNK(sb.st_mode)) {
		/* we were asked not to follow symlinks, but found one at
		   the target--simulate "not a directory" error */
		errno = ENOTDIR;
		err(EXIT_FAILURE, "%s", sourcedir);
		/* NOTREACHED */
	}
	if (stat(sourcedir, &sb)) {
		err(EXIT_FAILURE, "%s", sourcedir);
		/* NOTREACHED */
	}
	if (!S_ISDIR(sb.st_mode)) {
		usage();
		/* NOTREACHED */
	}
	for (exitval = 0; *argv != sourcedir; ++argv)
		exitval |= linkit(*argv, sourcedir, 1);
	exit(exitval);
	/* NOTREACHED */
}

int
linkit(char *target, char *source, int isdir)
{
	struct stat sb;
	char *p, path[MAXPATHLEN];

	if (!sflag) {
		/* If target doesn't exist, quit now. */
		if (stat(target, &sb)) {
			warn("%s", target);
			return (1);
		}
	}

	/* If the source is a directory (and not a symlink if hflag),
	   append the target's name. */
	if (isdir ||
	    (!lstat(source, &sb) && S_ISDIR(sb.st_mode)) ||
	    (!hflag && !stat(source, &sb) && S_ISDIR(sb.st_mode))) {
		if ((p = strrchr(target, '/')) == NULL)
			p = target;
		else
			++p;
		(void)snprintf(path, sizeof(path), "%s/%s", source, p);
		source = path;
	}

	/*
	 * If the file exists, and -f was specified, unlink it.
	 * Attempt the link.
	 */
	if ((fflag && unlink(source) < 0 && errno != ENOENT) ||
	    (*linkf)(target, source)) {
		warn("%s", source);
		return (1);
	}
	if (vflag)
		(void)printf("%s %c> %s\n", source, linkch, target);

	return (0);
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage:\t%s [-fhns] file1 file2\n\t%s [-fhnsv] file ... directory\n",
	    getprogname(), getprogname());
	exit(1);
	/* NOTREACHED */
}
