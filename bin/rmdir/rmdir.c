/*
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
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

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)rmdir.c	5.3 (Berkeley) 5/31/90";*/
static char rcsid[] = "$Id: rmdir.c,v 1.11 1994/04/28 00:03:18 jtc Exp $";
#endif /* not lint */

/*
 * Remove directory
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <unistd.h>
#include <err.h>

static int rmpath __P((char *));
static void usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int errors;
	int ch;
	int delete_parent_directories = 0;

	setlocale(LC_ALL, "");

	while ((ch = getopt (argc, argv, "p")) != -1) {
		switch (ch) {
		case 'p':
			delete_parent_directories = 1;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if (!*(argv += optind)) {
		usage ();
		/* NOTREACHED */
	}

	for (errors = 0; *argv; argv++) {
		register char *slash;

		/* delete trailing slashes */
		slash = strrchr(*argv, '\0');
		while (--slash > *argv && *slash == '/')
			*slash = '\0';

		if (rmdir(*argv)) {
			warn ("%s", *argv);
			errors = 1;
		} else if (delete_parent_directories) {
			errors |= rmpath(*argv);
		} 
	}

	exit(errors);
}

static int
rmpath (path)
	char *path;
{
	register char *slash;

	while ((slash = strrchr(path, '/')) != NULL) {
		/* delete trailing slash characters */
		while (slash > path && *slash == '/')
			slash--;
		*++slash = '\0';

		if (rmdir (path)) {
			warn ("%s", path);
			return 1;
		}
	}

	return 0;
}

static void
usage()
{
	fprintf(stderr, "usage: rmdir [-p] directory ...\n");
	exit(1);
}
