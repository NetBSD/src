/*	$NetBSD: uudecode.c,v 1.17 2003/08/07 11:16:58 agc Exp $	*/

/*-
 * Copyright (c) 1983, 1993
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
#if defined(__COPYRIGHT) && !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif

#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)uudecode.c	8.2 (Berkeley) 4/2/94";
#endif
__RCSID("$NetBSD: uudecode.c,v 1.17 2003/08/07 11:16:58 agc Exp $");
#endif /* not lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * uudecode [file ...]
 *
 * create the specified file, decoding as you go.
 * used with uuencode.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int decode __P((void));
static void usage __P((void));
int main __P((int, char **));

int pflag;
char *filename;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, rval;

	setlocale(LC_ALL, "");

	pflag = 0;
	while ((ch = getopt(argc, argv, "p")) != -1)
		switch (ch) {
		case 'p':
			pflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (*argv) {
		rval = 0;
		do {
			if (!freopen(filename = *argv, "r", stdin)) {
				warn("%s", *argv);
				rval = 1;
				continue;
			}
			rval |= decode();
		} while (*++argv);
	} else {
		filename = "stdin";
		rval = decode();
	}
	exit(rval);
}

static int
decode()
{
	struct passwd *pw;
	int n;
	char ch, *p;
	int n1;
	long mode;
	char *fn;
	char buf[MAXPATHLEN];

	/* search for header line */
	do {
		if (!fgets(buf, sizeof(buf), stdin)) {
			warnx("%s: no \"begin\" line", filename);
			return(1);
		}
	} while (strncmp(buf, "begin ", 6));
        /* must be followed by an octal mode and a space */
	mode = strtol(buf + 6, &fn, 8);
	if (fn == (buf+6) || !isspace(*fn) || mode==LONG_MIN || mode==LONG_MAX)
	{
	        warnx("%s: invalid mode on \"begin\" line", filename);
		return(1);
	}
	/* skip whitespace for file name */
	while (*fn && isspace(*fn)) fn++;
	if (*fn == 0) {
                warnx("%s: no filename on \"begin\" line", filename);
		return(1);
	}
	/* zap newline */
	for (p = fn; *p && *p != '\n'; p++) 
	        ;
	if (*p) *p = 0;
	
	/* handle ~user/file format */
	if (*fn == '~') {
		if (!(p = strchr(fn, '/'))) {
			warnx("%s: illegal ~user.", filename);
			return(1);
		}
		*p++ = '\0';
		if (!(pw = getpwnam(fn + 1))) {
			warnx("%s: no user %s.", filename, buf);
			return(1);
		}
		n = strlen(pw->pw_dir);
		n1 = strlen(p);
		if (n + n1 + 2 > MAXPATHLEN) {
			warnx("%s: path too long.", filename);
			return(1);
		}
		/* make space at beginning of buf by moving end of pathname */
		memmove(buf + n + 1, p, n1 + 1);
		memmove(buf, pw->pw_dir, n);
		buf[n] = '/';
		fn = buf;
	}

	/* create output file, set mode */
	if (!pflag && (!freopen(fn, "w", stdout) ||
	    fchmod(fileno(stdout), mode & 0666))) { 
		warn("%s: %s", fn, filename);
		return(1);
	}

	/* for each input line */
	for (;;) {
		if (!fgets(p = buf, sizeof(buf), stdin)) {
			warnx("%s: short file.", filename);
			return(1);
		}
#define	DEC(c)	(((c) - ' ') & 077)		/* single character decode */
		/*
		 * `n' is used to avoid writing out all the characters
		 * at the end of the file.
		 */
		if ((n = DEC(*p)) <= 0)
			break;
		for (++p; n > 0; p += 4, n -= 3)
			if (n >= 3) {
				ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
				putchar(ch);
				ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
				putchar(ch);
				ch = DEC(p[2]) << 6 | DEC(p[3]);
				putchar(ch);
			}
			else {
				if (n >= 1) {
					ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
					putchar(ch);
				}
				if (n >= 2) {
					ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
					putchar(ch);
				}
				if (n >= 3) {
					ch = DEC(p[2]) << 6 | DEC(p[3]);
					putchar(ch);
				}
			}
	}
	if (!fgets(buf, sizeof(buf), stdin) || strcmp(buf, "end\n")) {
		warnx("%s: no \"end\" line.", filename);
		return(1);
	}
	return(0);
}

static void
usage()
{
	(void)fprintf(stderr, "usage: uudecode [-p] [file ...]\n");
	exit(1);
}
