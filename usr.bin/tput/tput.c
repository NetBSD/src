/*-
 * Copyright (c) 1980, 1988 The Regents of the University of California.
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
"@(#) Copyright (c) 1980, 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)tput.c	5.7 (Berkeley) 6/7/90";*/
static char rcsid[] = "$Id: tput.c,v 1.4 1994/03/19 07:42:18 cgd Exp $";
#endif /* not lint */

#include <sys/termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>

static void   prlongname __P((char *));
static void   setospeed __P((void));
static void   outc	 __P((int));
static void   usage __P((void));
static char **process __P((char *, char *, char **));

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern char *optarg;
	extern int optind;
	int ch, exitval, n;
	char *cptr, *p, *term, buf[1024], tbuf[1024];

	term = NULL;
	while ((ch = getopt(argc, argv, "T:")) != EOF)
		switch(ch) {
		case 'T':
			term = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!term && !(term = getenv("TERM"))) {
		(void)fprintf(stderr, "tput: no terminal type specified.\n");
		exit(2);
	}
	if (tgetent(tbuf, term) != 1) {
		(void)fprintf(stderr, "tput: tgetent failure.\n");
		exit(2);
	}
	setospeed();
	for (exitval = 0; (p = *argv) != NULL; ++argv) {
		switch(*p) {
		case 'c':
			if (!strcmp(p, "clear"))
				p = "cl";
			break;
		case 'i':
			if (!strcmp(p, "init"))
				p = "is";
			break;
		case 'l':
			if (!strcmp(p, "longname")) {
				prlongname(tbuf);
				continue;
			}
			break;
		case 'r':
			if (!strcmp(p, "reset"))
				p = "rs";
			break;
		}
		cptr = buf;
		if (tgetstr(p, &cptr))
			argv = process(p, buf, argv);
		else if ((n = tgetnum(p)) != -1)
			(void)printf("%d\n", n);
		else
			exitval = !tgetflag(p);

		if (argv == NULL)
			break;
	}
	exit(argv ? exitval : 2);
}

static void
prlongname(buf)
	char *buf;
{
	register char *p;
	int savech;
	char *savep;

	for (p = buf; *p && *p != ':'; ++p)
		continue;
	savech = *(savep = p);
	for (*p = '\0'; p >= buf && *p != '|'; --p)
		continue;
	(void)printf("%s\n", p + 1);
	*savep = savech;
}

static void
setospeed()
{
#undef ospeed
	extern short ospeed;
	struct termios t;

	if (tcgetattr(STDOUT_FILENO, &t) != -1)
		ospeed = 0;
	else
		ospeed = cfgetospeed(&t);
}

static void
outc(c)
	int c;
{
	putchar(c);
}

static void
usage()
{
	(void)fprintf(stderr, "usage: tput [-T term] attribute ...\n");
	exit(1);
}

static char **
process(cap, str, argv)
	char *cap;
	char *str;
	char **argv;
{
	static char errfew[] =
		 "tput: Not enough arguments (%d) for capability `%s'\n";
	static char errmany[] =
		 "tput: Too many arguments (%d) for capability `%s'\n";
	static char erresc[] =
		 "tput: Unknown %% escape `%c' for capability `%s'\n";
	/*
	 * Count home many values we need for this capability.
	 */
	char *cp;
	int arg_need, arg_rows, arg_cols;
	for (cp = str, arg_need = 0; *cp; cp++)
		if (*cp == '%')
			    switch (*++cp) {
			    case 'd':
			    case '2':
			    case '3':
			    case '.':
			    case '+':
				    arg_need++;
				    break;

			    case '%':
			    case '>':
			    case 'i':
			    case 'r':
			    case 'n':
			    case 'B':
			    case 'D':
				    break;

			    default:
				/*
				 * hpux has lot's of them, but we complain
				 */
			        (void)fprintf(stderr, erresc, *cp, cap);
				return NULL;
			    }

	/*
	 * And print them
	 */
	switch (arg_need) {
	case 0:
		(void) tputs(str, 1, outc);
		break;

	case 1:
		arg_cols = 0;

		argv++;
		if (!*argv || *argv[0] == '\0') {
			(void)fprintf(stderr, errfew, 1, cap);
			return NULL;
		}
		arg_rows = atoi(*argv);

		(void)tputs(tgoto(str, arg_cols, arg_rows), 1, outc);
		break;

	case 2:
		argv++;
		if (!*argv || *argv[0] == '\0') {
			(void)fprintf(stderr, errfew, 2, cap);
			return NULL;
		}
		arg_cols = atoi(*argv);

		argv++;
		if (!*argv || *argv[0] == '\0') {
			(void)fprintf(stderr, errfew, 2, cap);
			return NULL;
		}
		arg_rows = atoi(*argv);

		(void) tputs(tgoto(str, arg_cols, arg_rows), arg_rows, outc);
		break;

	default:
		(void)fprintf(stderr, errmany, arg_need, cap);
		return NULL;
	}
	return argv;
}
