/*	$NetBSD: wc.c,v 1.16 1999/02/14 18:03:18 mjacob Exp $	*/

/*
 * Copyright (c) 1980, 1987, 1991, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1987, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)wc.c	8.2 (Berkeley) 5/2/95";
#else
__RCSID("$NetBSD: wc.c,v 1.16 1999/02/14 18:03:18 mjacob Exp $");
#endif
#endif /* not lint */

/* wc line, word and char count */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <err.h>

static u_int64_t	tlinect, twordct, tcharct;
static int		doline, doword, dochar;
static int 		rval = 0;
static char *qfmt;

static void	cnt __P((char *));
static void	print_counts __P((u_int64_t, u_int64_t, u_int64_t, char *));
static void	usage __P((void));
int	main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;

	setlocale(LC_ALL, "");
	if (sizeof (u_int64_t) > 4)
		qfmt = " %7u";
	else
		qfmt = " %7qu";

	while ((ch = getopt(argc, argv, "lwcm")) != -1)
		switch((char)ch) {
		case 'l':
			doline = 1;
			break;
		case 'w':
			doword = 1;
			break;
		case 'c':
		case 'm':
			dochar = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	/* Wc's flags are on by default. */
	if (doline + doword + dochar == 0)
		doline = doword = dochar = 1;

	if (!*argv) {
		cnt(NULL);
	} else {
		int dototal = (argc > 1);

		do {
			cnt(*argv);
		} while(*++argv);

		if (dototal) {
			print_counts(tlinect, twordct, tcharct, "total"); 
		}
	}

	exit(rval);
}

static void
cnt(file)
	char *file;
{
	u_char *C;
	short gotsp;
	int len;
	u_int64_t linect, wordct, charct;
	struct stat sb;
	int fd;
	u_char buf[MAXBSIZE];

	linect = wordct = charct = 0;
	if (file) {
		if ((fd = open(file, O_RDONLY, 0)) < 0) {
			warn("%s", file);
			rval = 1;
			return;
		}
	} else  {
		fd = STDIN_FILENO;
	}
	
	if (!doword) {
		/*
		 * line counting is split out because it's a lot
		 * faster to get lines than to get words, since
		 * the word count requires some logic.
		 */
		if (doline) {
			while ((len = read(fd, buf, MAXBSIZE)) > 0) {
				charct += len;
				for (C = buf; len--; ++C)
					if (*C == '\n')
						++linect;
			}
			if (len == -1) {
				warn ("%s", file);
				rval = 1;
			}
		}

		/*
		 * if all we need is the number of characters and
		 * it's a directory or a regular or linked file, just
		 * stat the puppy.  We avoid testing for it not being
		 * a special device in case someone adds a new type
		 * of inode.
		 */
		else if (dochar) {
			if (fstat(fd, &sb)) {
				warn("%s", file);
				rval = 1;
			} else {
				if (S_ISREG(sb.st_mode) ||
				    S_ISLNK(sb.st_mode) ||
				    S_ISDIR(sb.st_mode)) {
					charct = sb.st_size;
				} else {
					while ((len = read(fd, buf, MAXBSIZE)) > 0)
						charct += len;
					if (len == -1) {
						warn ("%s", file);
						rval = 1;
					}
				}
			}
		}
	}
	else
	{
		/* do it the hard way... */
		gotsp = 1;
		while ((len = read(fd, buf, MAXBSIZE)) > 0) {
			charct += len;
			for (C = buf; len--; ++C) {
				if (isspace(*C)) {
					gotsp = 1;
					if (*C == '\n') {
						++linect;
					}
				} else {
					/*
					 * This line implements the POSIX
					 * spec, i.e. a word is a "maximal
					 * string of characters delimited by
					 * whitespace."  Notice nothing was
					 * said about a character being
					 * printing or non-printing.
					 */
					if (gotsp) {
						gotsp = 0;
						++wordct;
					}
				}
			}
		}
		if (len == -1) {
			warn("%s", file);
			rval = 1;
		}
	}

	print_counts(linect, wordct, charct, file ? file : "");

	/* don't bother checkint doline, doword, or dochar --- speeds
           up the common case */
	tlinect += linect;
	twordct += wordct;
	tcharct += charct;

	if (close(fd)) {
		warn ("%s", file);
		rval = 1;
	}
}

static void
print_counts(lines, words, chars, name)
	u_int64_t lines;
	u_int64_t words;
	u_int64_t chars;
	char *name;
{

	if (doline)
		printf(qfmt, lines);
	if (doword)
		printf(qfmt, words);
	if (dochar)
		printf(qfmt, chars);

	printf(" %s\n", name);
}

static void
usage()
{
	(void)fprintf(stderr, "usage: wc [-clw] [files]\n");
	exit(1);
}
