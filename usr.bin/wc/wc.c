/*
 * Copyright (c) 1980, 1987 Regents of the University of California.
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
"@(#) Copyright (c) 1980, 1987 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)wc.c	5.7 (Berkeley) 3/2/91";*/
static char rcsid[] = "$Id: wc.c,v 1.6 1993/10/12 22:39:43 jtc Exp $";
#endif /* not lint */

/* wc line, word and char count */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

static void	cnt();
static long	tlinect, twordct, tcharct;
static int	doline, doword, dochar;
static int 	rval = 0;

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	register int ch;

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
			fprintf(stderr, "usage: wc [-c | -m] [-lw] [files]\n");
			exit(1);
		}
	argv += optind;
	argc -= optind;

	/*
	 * wc is unusual in that its flags are on by default, so,
	 * if you don't get any arguments, you have to turn them
	 * all on.
	 */
	if (!doline && !doword && !dochar) {
		doline = doword = dochar = 1;
	}

	if (!*argv) {
		cnt((char *)NULL);
		putchar('\n');
	} else {
		int dototal = (argc > 1);

		do {
			cnt(*argv);
			printf(" %s\n", *argv);
		} while(*++argv);

		if (dototal) {
			if (doline)
				printf(" %7ld", tlinect);
			if (doword)
				printf(" %7ld", twordct);
			if (dochar)
				printf(" %7ld", tcharct);
			puts(" total");
		}
	}

	exit(rval);
}

static void
cnt(file)
	char *file;
{
	register u_char *C;
	register short gotsp;
	register int len;
	register long linect, wordct, charct;
	struct stat sbuf;
	int fd;
	u_char buf[MAXBSIZE];

	linect = wordct = charct = 0;
	if (file) {
		if ((fd = open(file, O_RDONLY, 0)) < 0) {
			fprintf (stderr, "wc: %s: %s\n", file, strerror(errno));
			rval = 1;
			return;
		}
		if (!doword) {
			/*
			 * line counting is split out because it's a lot
			 * faster to get lines than to get words, since
			 * the word count requires some logic.
			 */
			if (doline) {
				while((len = read(fd, buf, MAXBSIZE)) != 0) {
					if (len == -1) {
						fprintf (stderr, "wc: %s: %s\n",
							file, strerror(errno));
						rval = 1;
						close (fd);
						return;	
					}
					charct += len;
					for (C = buf; len--; ++C)
						if (*C == '\n')
							++linect;
				}
				tlinect += linect;
				printf(" %7ld", linect);
				if (dochar) {
					tcharct += charct;
					printf(" %7ld", charct);
				}
				close(fd);
				return;
			}
			/*
			 * if all we need is the number of characters and
			 * it's a directory or a regular or linked file, just
			 * stat the puppy.  We avoid testing for it not being
			 * a special device in case someone adds a new type
			 * of inode.
			 */
			if (dochar) {
				int ifmt;

				if (fstat(fd, &sbuf)) {
					fprintf (stderr, "wc: %s: %s\n",
						file, strerror(errno));
					rval = 1;
					close (fd);
					return;
				}

				ifmt = sbuf.st_mode & S_IFMT;
				if (ifmt == S_IFREG || ifmt == S_IFLNK
					|| ifmt == S_IFDIR) {
					printf(" %7ld", sbuf.st_size);
					tcharct += sbuf.st_size;
					close(fd);
					return;
				}
			}
		}
	}
	else
		fd = 0;

	/* do it the hard way... */
	for (gotsp = 1; (len = read(fd, buf, MAXBSIZE));) {
		if (len == -1) {
			fprintf (stderr, "wc: %s: %s\n", file, strerror(errno));
			break;
		}
		charct += len;
		for (C = buf; len--; ++C) {
			if (isspace (*C)) {
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
	if (doline) {
		tlinect += linect;
		printf(" %7ld", linect);
	}
	if (doword) {
		twordct += wordct;
		printf(" %7ld", wordct);
	}
	if (dochar) {
		tcharct += charct;
		printf(" %7ld", charct);
	}
	close(fd);
}
