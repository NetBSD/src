/* $NetBSD: rmdir.c,v 1.20 2003/08/13 03:27:20 itojun Exp $ */

/*-
 * Copyright (c) 1992, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)rmdir.c	8.3 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: rmdir.c,v 1.20 2003/08/13 03:27:20 itojun Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>

#include <err.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

int	stdout_ok;			/* stdout connected to a terminal */

int	rm_path(char *);
void	usage(void);
int	main(int, char *[]);
char	*printescaped(const char *);

int
main(int argc, char *argv[])
{
	int ch, errors, pflag;

	setprogname(argv[0]);
	(void)setlocale(LC_ALL, "");

	pflag = 0;
	while ((ch = getopt(argc, argv, "p")) != -1)
		switch(ch) {
		case 'p':
			pflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	stdout_ok = isatty(STDIN_FILENO);

	for (errors = 0; *argv; argv++) {
		char *p;

		/* Delete trailing slashes, per POSIX. */
		p = *argv + strlen(*argv);
		while (--p > *argv && *p == '/')
			;
		*++p = '\0';

		if (rmdir(*argv) < 0) {
			char *dn;
			dn = printescaped(*argv);
			warn("%s", dn);
			free(dn);
			errors = 1;
		} else if (pflag)
			errors |= rm_path(*argv);
	}

	exit(errors);
	/* NOTREACHED */
}

int
rm_path(char *path)
{
	char *p;

	while ((p = strrchr(path, '/')) != NULL) {
		/* Delete trailing slashes. */
		while (--p > path && *p == '/')
			;
		*++p = '\0';

		if (rmdir(path) < 0) {
			char *pn;
			pn = printescaped(path);
			warn("%s", pn);
			free(pn);
			return (1);
		}
	}

	return (0);
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-p] directory ...\n", getprogname());
	exit(1);
	/* NOTREACHED */
}

char *
printescaped(const char *src)
{
	size_t len;
	char *retval;

	len = strlen(src);
	if (len != 0 && SIZE_T_MAX/len <= 4) {
		errx(EXIT_FAILURE, "%s: name too long", src);
		/* NOTREACHED */
	}

	retval = (char *)malloc(4*len+1);
	if (retval != NULL) {
		if (stdout_ok)
			(void)strvis(retval, src, VIS_NL | VIS_CSTYLE);
		else
			(void)strlcpy(retval, src, 4 * len + 1);
		return retval;
	} else
		errx(EXIT_FAILURE, "out of memory!");
		/* NOTREACHED */
}
