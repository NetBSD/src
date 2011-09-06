/*	$NetBSD: rs.c,v 1.15 2011/09/06 18:28:58 joerg Exp $	*/

/*-
 * Copyright (c) 1993
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
__COPYRIGHT("@(#) Copyright (c) 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)rs.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: rs.c,v 1.15 2011/09/06 18:28:58 joerg Exp $");
#endif
#endif /* not lint */

/*
 *	rs - reshape a data array
 *	Author:  John Kunze, Office of Comp. Affairs, UCB
 *		BEWARE: lots of unfinished edges
 */

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static long	flags;
#define	TRANSPOSE	000001
#define	MTRANSPOSE	000002
#define	ONEPERLINE	000004
#define	ONEISEPONLY	000010
#define	ONEOSEPONLY	000020
#define	NOTRIMENDCOL	000040
#define	SQUEEZE		000100
#define	SHAPEONLY	000200
#define	DETAILSHAPE	000400
#define	RIGHTADJUST	001000
#define	NULLPAD		002000
#define	RECYCLE		004000
#define	SKIPPRINT	010000
#define	ICOLBOUNDS	020000
#define	OCOLBOUNDS	040000
#define ONEPERCHAR	0100000
#define NOARGS		0200000

static short	*colwidths;
static short	*cord;
static short	*icbd;
static short	*ocbd;
static int	nelem;
static char	**elem;
static char	**endelem;
static char	*curline;
static int	allocsize = BUFSIZ;
static int	curlen;
static int	irows, icols;
static int	orows, ocols;
static int	maxlen;
static int	skip;
static int	propgutter;
static char	isep = ' ', osep = ' ';
static int	owidth = 80, gutter = 2;

static void	  usage(const char *, ...) __dead __printflike(1, 2);
static void	  getargs(int, char *[]);
static void	  getfile(void);
static int	  get_line(void);
static char	 *getlist(short **, char *);
static char	 *getnum(int *, char *, int);
static char	**getptrs(char **);
static void	  prepfile(void);
static void	  prints(char *, int);
static void	  putfile(void);

#define INCR(ep) do {			\
	if (++ep >= endelem)		\
		ep = getptrs(ep);	\
} while(0)

int
main(int argc, char *argv[])
{
	getargs(argc, argv);
	getfile();
	if (flags & SHAPEONLY) {
		printf("%d %d\n", irows, icols);
		exit(0);
	}
	prepfile();
	putfile();
	exit(0);
}

static void
getfile(void)
{
	char empty[1] = { '\0' };
	char *p;
	char *endp;
	char **ep = 0;
	int multisep = (flags & ONEISEPONLY ? 0 : 1);
	int nullpad = flags & NULLPAD;
	char **padto;

	while (skip--) {
		get_line();
		if (flags & SKIPPRINT)
			puts(curline);
	}
	get_line();
	if (flags & NOARGS && curlen < owidth)
		flags |= ONEPERLINE;
	if (flags & ONEPERLINE)
		icols = 1;
	else				/* count cols on first line */
		for (p = curline, endp = curline + curlen; p < endp; p++) {
			if (*p == isep && multisep)
				continue;
			icols++;
			while (*p && *p != isep)
				p++;
		}
	ep = getptrs(elem);
	p = curline;
	do {
		if (flags & ONEPERLINE) {
			*ep = curline;
			INCR(ep);		/* prepare for next entry */
			if (maxlen < curlen)
				maxlen = curlen;
			irows++;
			continue;
		}
		for (p = curline, endp = curline + curlen; p < endp; p++) {
			if (*p == isep && multisep)
				continue;	/* eat up column separators */
			if (*p == isep)		/* must be an empty column */
				*ep = empty;
			else			/* store column entry */
				*ep = p;
			while (p < endp && *p != isep)
				p++;		/* find end of entry */
			*p = '\0';		/* mark end of entry */
			if (maxlen < p - *ep)	/* update maxlen */
				maxlen = p - *ep;
			INCR(ep);		/* prepare for next entry */
		}
		irows++;			/* update row count */
		if (nullpad) {			/* pad missing entries */
			padto = elem + irows * icols;
			while (ep < padto) {
				*ep = empty;
				INCR(ep);
			}
		}
	} while (get_line() != EOF);
	*ep = 0;				/* mark end of pointers */
	nelem = ep - elem;
}

static void
putfile(void)
{
	char **ep;
	int i, j, n;

	ep = elem;
	if (flags & TRANSPOSE) {
		for (i = 0; i < orows; i++) {
			for (j = i; j < nelem; j += orows)
				prints(ep[j], (j - i) / orows);
			putchar('\n');
		}
	} else {
		for (n = 0, i = 0; i < orows && n < nelem; i++) {
			for (j = 0; j < ocols; j++) {
				if (n++ >= nelem)
					break;
				prints(*ep++, j);
			}
			putchar('\n');
		}
	}
}

static void
prints(char *s, int col)
{
	int n;
	char *p = s;

	while (*p)
		p++;
	n = (flags & ONEOSEPONLY ? 1 : colwidths[col] - (p - s));
	if (flags & RIGHTADJUST)
		while (n-- > 0)
			putchar(osep);
	for (p = s; *p; p++)
		putchar(*p);
	while (n-- > 0)
		putchar(osep);
}

static void
usage(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vwarnx(msg, ap);
	va_end(ap);
	fprintf(stderr,
"usage:  rs [ -[csCS][x][kKgGw][N]tTeEnyjhHm ] [ rows [ cols ] ]\n");
	exit(1);
}

static void
prepfile(void)
{
	char **ep;
	int  i;
	int  j;
	char **lp;
	int colw;
	int max = 0;
	int n;

	ep = NULL;
	if (!nelem)
		exit(0);
	gutter += maxlen * propgutter / 100.0;
	colw = maxlen + gutter;
	if (flags & MTRANSPOSE) {
		orows = icols;
		ocols = irows;
	}
	else if (orows == 0 && ocols == 0) {	/* decide rows and cols */
		ocols = owidth / colw;
		if (ocols == 0) {
			warnx("Display width %d is less than column width %d", owidth, colw);
			ocols = 1;
		}
		if (ocols > nelem)
			ocols = nelem;
		orows = nelem / ocols + (nelem % ocols ? 1 : 0);
	}
	else if (orows == 0)			/* decide on rows */
		orows = nelem / ocols + (nelem % ocols ? 1 : 0);
	else if (ocols == 0)			/* decide on cols */
		ocols = nelem / orows + (nelem % orows ? 1 : 0);
	lp = elem + orows * ocols;
	while (lp > endelem) {
		getptrs(elem + nelem);
		lp = elem + orows * ocols;
	}
	if (flags & RECYCLE) {
		for (ep = elem + nelem; ep < lp; ep++)
			*ep = *(ep - nelem);
		nelem = lp - elem;
	}
	if (!(colwidths = (short *) malloc(ocols * sizeof(short))))
		errx(1, "malloc:  No gutter space");
	if (flags & SQUEEZE) {
		if (flags & TRANSPOSE)
			for (ep = elem, i = 0; i < ocols; i++) {
				for (j = 0; j < orows; j++)
					if ((n = strlen(*ep++)) > max)
						max = n;
				colwidths[i] = max + gutter;
			}
		else
			for (ep = elem, i = 0; i < ocols; i++) {
				for (j = i; j < nelem; j += ocols)
					if ((n = strlen(ep[j])) > max)
						max = n;
				colwidths[i] = max + gutter;
			}
	}
	/*	for (i = 0; i < orows; i++) {
			for (j = i; j < nelem; j += orows)
				prints(ep[j], (j - i) / orows);
			putchar('\n');
		}
	else
		for (i = 0; i < orows; i++) {
			for (j = 0; j < ocols; j++)
				prints(*ep++, j);
			putchar('\n');
		}*/
	else
		for (i = 0; i < ocols; i++)
			colwidths[i] = colw;
	if (!(flags & NOTRIMENDCOL)) {
		if (flags & RIGHTADJUST)
			colwidths[0] -= gutter;
		else
			colwidths[ocols - 1] = 0;
	}
	n = orows * ocols;
	if (n > nelem && (flags & RECYCLE))
		nelem = n;
	/*for (i = 0; i < ocols; i++)
		fprintf(stderr, "%d ",colwidths[i]);
	fprintf(stderr, "is colwidths, nelem %d\n", nelem);*/
}

#define	BSIZE	2048
char	ibuf[BSIZE];		/* two screenfuls should do */

static int
get_line(void)	/* get line; maintain curline, curlen; manage storage */
{
	static	int putlength;
	static	char *endblock = ibuf + BSIZE;
	char *p;
	int c, i;

	if (!irows) {
		curline = ibuf;
		putlength = flags & DETAILSHAPE;
	}
	else if (skip <= 0) {			/* don't waste storage */
		curline += curlen + 1;
		if (putlength)		/* print length, recycle storage */
			printf(" %d line %d\n", curlen, irows);
	}
	if (!putlength && endblock - curline < BUFSIZ) {   /* need storage */
		/*ww = endblock-curline; tt += ww;*/
		/*printf("#wasted %d total %d\n",ww,tt);*/
		if (!(curline = (char *) malloc(BSIZE)))
			errx(1, "File too large");
		endblock = curline + BSIZE;
		/*printf("#endb %d curline %d\n",endblock,curline);*/
	}
	for (p = curline, i = 1; i < BUFSIZ; *p++ = c, i++)
		if ((c = getchar()) == EOF || c == '\n')
			break;
	*p = '\0';
	curlen = i - 1;
	return(c);
}

static char **
getptrs(char **sp)
{
	char **p;

	allocsize += allocsize;
	p = (char **)realloc(elem, allocsize * sizeof(char *));
	if (p == (char **)0)
		err(1, "no memory");

	sp += (p - elem);
	endelem = (elem = p) + allocsize;
	return(sp);
}

static void
getargs(int ac, char *av[])
{
	char *p;

	if (ac == 1) {
		flags |= NOARGS | TRANSPOSE;
	}
	while (--ac && **++av == '-')
		for (p = *av+1; *p; p++)
			switch (*p) {
			case 'T':
				flags |= MTRANSPOSE;
			case 't':
				flags |= TRANSPOSE;
				break;
			case 'c':		/* input col. separator */
				flags |= ONEISEPONLY;
			case 's':		/* one or more allowed */
				if (p[1])
					isep = *++p;
				else
					isep = '\t';	/* default is ^I */
				break;
			case 'C':
				flags |= ONEOSEPONLY;
			case 'S':
				if (p[1])
					osep = *++p;
				else
					osep = '\t';	/* default is ^I */
				break;
			case 'w':		/* window width, default 80 */
				p = getnum(&owidth, p, 0);
				if (owidth <= 0)
				usage("Width must be a positive integer");
				break;
			case 'K':			/* skip N lines */
				flags |= SKIPPRINT;
			case 'k':			/* skip, do not print */
				p = getnum(&skip, p, 0);
				if (!skip)
					skip = 1;
				break;
			case 'm':
				flags |= NOTRIMENDCOL;
				break;
			case 'g':		/* gutter space */
				p = getnum(&gutter, p, 0);
				break;
			case 'G':
				p = getnum(&propgutter, p, 0);
				break;
			case 'e':		/* each line is an entry */
				flags |= ONEPERLINE;
				break;
			case 'E':
				flags |= ONEPERCHAR;
				break;
			case 'j':			/* right adjust */
				flags |= RIGHTADJUST;
				break;
			case 'n':	/* null padding for missing values */
				flags |= NULLPAD;
				break;
			case 'y':
				flags |= RECYCLE;
				break;
			case 'H':			/* print shape only */
				flags |= DETAILSHAPE;
			case 'h':
				flags |= SHAPEONLY;
				break;
			case 'z':			/* squeeze col width */
				flags |= SQUEEZE;
				break;
			/*case 'p':
				ipagespace = atoi(++p);	(default is 1)
				break;*/
			case 'o':			/* col order */
				p = getlist(&cord, p);
				break;
			case 'b':
				flags |= ICOLBOUNDS;
				p = getlist(&icbd, p);
				break;
			case 'B':
				flags |= OCOLBOUNDS;
				p = getlist(&ocbd, p);
				break;
			default:
				usage("Bad flag:  %.1s", p);
			}
	/*if (!osep)
		osep = isep;*/
	switch (ac) {
	/*case 3:
		opages = atoi(av[2]);*/
	case 2:
		ocols = atoi(av[1]);
	case 1:
		orows = atoi(av[0]);
	case 0:
		break;
	default:
		usage("Too many arguments.");
	}
}

static char *
getlist(short **list, char *p)
{
	int count = 1;
	char *t;

	for (t = p + 1; *t; t++) {
		if (!isdigit((unsigned char)*t))
			usage("Option %.1s requires a list of unsigned numbers separated by commas", t);
		count++;
		while (*t && isdigit((unsigned char)*t))
			t++;
		if (*t != ',')
			break;
	}
	if (!(*list = (short *) malloc(count * sizeof(short))))
		errx(1, "No list space");
	count = 0;
	for (t = p + 1; *t; t++) {
		(*list)[count++] = atoi(t);
		printf("++ %d ", (*list)[count-1]);
		fflush(stdout);
		while (*t && isdigit((unsigned char)*t))
			t++;
		if (*t != ',')
			break;
	}
	(*list)[count] = 0;
	return(t - 1);
}

static char *
getnum(int *num, char *p, int strict)	/* num = number p points to; if (strict) complain */
					/* returns pointer to end of num */
{
	char *t = p;

	if (!isdigit((unsigned char)*++t)) {
		if (strict || *t == '-' || *t == '+')
			usage("Option %.1s requires an unsigned integer", p);
		*num = 0;
		return(p);
	}
	*num = atoi(t);
	while (*++t)
		if (!isdigit((unsigned char)*t))
			break;
	return(--t);
}
