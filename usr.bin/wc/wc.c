/*	$NetBSD: wc.c,v 1.24 2002/03/23 21:27:14 enami Exp $	*/

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
__RCSID("$NetBSD: wc.c,v 1.24 2002/03/23 21:27:14 enami Exp $");
#endif
#endif /* not lint */

/* wc line, word and char count */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <ctype.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#ifdef NO_QUAD
typedef u_long wc_count_t;
# define WCFMT	" %7lu"
# define WCCAST unsigned long
#else
typedef u_quad_t wc_count_t;
# define WCFMT	" %7llu"
# define WCCAST	unsigned long long
#endif

static wc_count_t	tlinect, twordct, tcharct;
static int		doline, doword, dobyte, dochar;
static int 		rval = 0;

static void	cnt __P((char *));
static void	print_counts __P((wc_count_t, wc_count_t, wc_count_t, char *));
static void	usage __P((void));
static size_t	do_mb __P((wchar_t *, const char *, size_t, mbstate_t *,
		    size_t *, const char *));
int	main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;

	setlocale(LC_ALL, "");

	while ((ch = getopt(argc, argv, "lwcm")) != -1)
		switch (ch) {
		case 'l':
			doline = 1;
			break;
		case 'w':
			doword = 1;
			break;
		case 'm':
			dochar = 1;
			dobyte = 0;
			break;
		case 'c':
			dochar = 0;
			dobyte = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	/* Wc's flags are on by default. */
	if (doline + doword + dobyte + dochar == 0)
		doline = doword = dobyte = 1;

	if (!*argv) {
		cnt(NULL);
	} else {
		int dototal = (argc > 1);

		do {
			cnt(*argv);
		} while(*++argv);

		if (dototal)
			print_counts(tlinect, twordct, tcharct, "total");
	}

	exit(rval);
}

static size_t
do_mb(wc, p, mblen, st, cnt, file)
	wchar_t *wc;
	const char *p;
	size_t mblen;
	mbstate_t *st;
	size_t *cnt;
	const char *file;
{
	size_t r;
	size_t c = 0;

	do {
		r = mbrtowc(wc, p, mblen, st);
		if (r == (size_t)-1) {
			warnx("%s: invalid byte sequence", file);
			rval = 1;

			/* XXX skip 1 byte */
			mblen--;
			p++;
			memset(st, 0, sizeof(*st));
		} else if (r == (size_t)-2)
			break;
		else if (r == 0)
			r = 1;
		c++;
		if (wc)
			wc++;
		mblen -= r;
		p += r;
	} while (mblen > 0);

	*cnt = c;

	return (r);
}

static void
cnt(file)
	char *file;
{
	u_char buf[MAXBSIZE];
	wchar_t wbuf[MAXBSIZE];
	struct stat sb;
	wc_count_t charct, linect, wordct;
	mbstate_t st;
	u_char *C;
	wchar_t *WC;
	size_t r = 0;
	int fd, gotsp, len = 0;

	linect = wordct = charct = 0;
	if (file) {
		if ((fd = open(file, O_RDONLY, 0)) < 0) {
			warn("%s", file);
			rval = 1;
			return;
		}
	} else {
		fd = STDIN_FILENO;
	}

	if (dochar || doword)
		memset(&st, 0, sizeof(st));

	if (!doword) {
		/*
		 * line counting is split out because it's a lot
		 * faster to get lines than to get words, since
		 * the word count requires some logic.
		 */
		if (doline || dochar) {
			while ((len = read(fd, buf, MAXBSIZE)) > 0) {
				if (dochar) {
					size_t wlen;

					r = do_mb(0, (char *)buf, (size_t)len,
					    &st, &wlen, file);
					charct += wlen;
				} else if (dobyte)
					charct += len;
				if (doline)
					for (C = buf; len--; ++C)
						if (*C == '\n')
							++linect;
			}
		}

		/*
		 * if all we need is the number of characters and
		 * it's a directory or a regular or linked file, just
		 * stat the puppy.  We avoid testing for it not being
		 * a special device in case someone adds a new type
		 * of inode.
		 */
		else if (dobyte) {
			if (fstat(fd, &sb)) {
				warn("%s", file);
				rval = 1;
			} else {
				if (S_ISREG(sb.st_mode) ||
				    S_ISLNK(sb.st_mode) ||
				    S_ISDIR(sb.st_mode)) {
					charct = sb.st_size;
				} else {
					while ((len =
					    read(fd, buf, MAXBSIZE)) > 0)
						charct += len;
				}
			}
		}
	} else {
		/* do it the hard way... */
		gotsp = 1;
		while ((len = read(fd, buf, MAXBSIZE)) > 0) {
			size_t wlen;

			r = do_mb(wbuf, (char *)buf, (size_t)len, &st, &wlen,
			    file);
			if (dochar) {
				charct += wlen;
			} else if (dobyte)
				charct += len;
			for (WC = wbuf; wlen--; ++WC) {
				if (iswspace(*WC)) {
					gotsp = 1;
					if (*WC == L'\n') {
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
	}

	if (len == -1) {
		warn ("%s", file);
		rval = 1;
	}
	if (dochar && r == (size_t)-2) {
		warnx ("%s: incomplete multibyte character", file);
		rval = 1;
	}

	print_counts(linect, wordct, charct, file ? file : 0);

	/*
	 * don't bother checkint doline, doword, or dobyte --- speeds
	 * up the common case
	 */
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
	wc_count_t lines;
	wc_count_t words;
	wc_count_t chars;
	char *name;
{

	if (doline)
		printf(WCFMT, (WCCAST)lines);
	if (doword)
		printf(WCFMT, (WCCAST)words);
	if (dobyte || dochar)
		printf(WCFMT, (WCCAST)chars);

	if (name)
		printf(" %s\n", name);
	else
		printf("\n");
}

static void
usage()
{

	(void)fprintf(stderr, "usage: wc [-clw] [file ...]\n");
	exit(1);
}
