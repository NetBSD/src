/*	$NetBSD: ul.c,v 1.20 2019/02/03 03:19:30 mrg Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ul.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: ul.c,v 1.20 2019/02/03 03:19:30 mrg Exp $");
#endif /* not lint */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>
#include <unistd.h>
#include <util.h>

#define	IESC	'\033'
#define	SO	'\016'
#define	SI	'\017'
#define	HFWD	'9'
#define	HREV	'8'
#define	FREV	'7'
#define	MAXBUF	512

#define	NORMAL	000
#define	ALTSET	001	/* Reverse */
#define	SUPERSC	002	/* Dim */
#define	SUBSC	004	/* Dim | Ul */
#define	UNDERL	010	/* Ul */
#define	BOLD	020	/* Bold */

static int	must_overstrike;

struct	CHAR	{
	char	c_mode;
	char	c_char;
} ;

static size_t col, maxcol;
static int	mode;
static int	halfpos;
static int	upln;
static int	iflag;

static void filter(FILE *);
static void flushln(struct CHAR *, size_t);
static void fwd(struct CHAR *, size_t);
static void iattr(struct CHAR *);
static void initbuf(struct CHAR *, size_t);
static void outc(int);
static int outchar(int);
static void overstrike(struct CHAR *);
static void reverse(struct CHAR *, size_t);
static void setulmode(int);
static void alloc_buf(struct CHAR **, size_t *);
static void set_mode(void);


#define	PRINT(s)	if (s == NULL) /* void */; else tputs(s, 1, outchar)

int
main(int argc, char **argv)
{
	int c;
	const char *termtype;
	FILE *f;

	termtype = getenv("TERM");
	if (termtype == NULL || (argv[0][0] == 'c' && !isatty(1)))
		termtype = "lpr";
	while ((c=getopt(argc, argv, "it:T:")) != -1)
		switch(c) {

		case 't':
		case 'T': /* for nroff compatibility */
				termtype = optarg;
			break;
		case 'i':
			iflag = 1;
			break;

		default:
			fprintf(stderr,
				"usage: %s [ -i ] [ -tTerm ] file...\n",
				argv[0]);
			exit(1);
		}

	setupterm(termtype, 0, NULL);
	if ((over_strike && enter_bold_mode == NULL) ||
	    (transparent_underline && enter_underline_mode == NULL &&
		underline_char == NULL)) {
		set_mode();
	}
	if (optind == argc)
		filter(stdin);
	else {
		for (; optind < argc; optind++) {
			f = fopen(argv[optind], "r");
			if (f == NULL)
				err(EXIT_FAILURE, "Failed to open `%s'", argv[optind]);
			filter(f);
			fclose(f);
		}
	}
	exit(0);
}

static void
filter(FILE *f)
{
	int c;
	struct	CHAR *obuf = NULL;
	size_t obuf_size = 0;
	alloc_buf(&obuf, &obuf_size);

	while ((c = getc(f)) != EOF) switch(c) {

	case '\b':
		if (col > 0)
			col--;
		continue;

	case '\t':
		col = (col+8) & ~07;
		if (col > maxcol)
			maxcol = col;
		if (col >= obuf_size)
			alloc_buf(&obuf, &obuf_size);
		continue;

	case '\r':
		col = 0;
		continue;

	case SO:
		mode |= ALTSET;
		continue;

	case SI:
		mode &= ~ALTSET;
		continue;

	case IESC:
		switch (c = getc(f)) {

		case HREV:
			if (halfpos == 0) {
				mode |= SUPERSC;
				halfpos--;
			} else if (halfpos > 0) {
				mode &= ~SUBSC;
				halfpos--;
			} else {
				halfpos = 0;
				reverse(obuf, obuf_size);
			}
			continue;

		case HFWD:
			if (halfpos == 0) {
				mode |= SUBSC;
				halfpos++;
			} else if (halfpos < 0) {
				mode &= ~SUPERSC;
				halfpos++;
			} else {
				halfpos = 0;
				fwd(obuf, obuf_size);
			}
			continue;

		case FREV:
			reverse(obuf, obuf_size);
			continue;

		default:
			fprintf(stderr,
				"Unknown escape sequence in input: %o, %o\n",
				IESC, c);
			exit(1);
		}

	case '_':
		if (obuf[col].c_char)
			obuf[col].c_mode |= UNDERL | mode;
		else
			obuf[col].c_char = '_';
		/* FALLTHROUGH */
	case ' ':
		col++;
		if (col > maxcol)
			maxcol = col;
		if (col >= obuf_size)
			alloc_buf(&obuf, &obuf_size);
		continue;

	case '\n':
		flushln(obuf, obuf_size);
		continue;

	case '\f':
		flushln(obuf, obuf_size);
		putchar('\f');
		continue;

	default:
		if (c < ' ')	/* non printing */
			continue;
		if (obuf[col].c_char == '\0') {
			obuf[col].c_char = c;
			obuf[col].c_mode = mode;
		} else if (obuf[col].c_char == '_') {
			obuf[col].c_char = c;
			obuf[col].c_mode |= UNDERL|mode;
		} else if (obuf[col].c_char == c)
			obuf[col].c_mode |= BOLD|mode;
		else
			obuf[col].c_mode = mode;
		col++;
		if (col > maxcol)
			maxcol = col;
		if (col >= obuf_size)
			alloc_buf(&obuf, &obuf_size);
		continue;
	}
	if (maxcol)
		flushln(obuf, obuf_size);

	free(obuf);
}

static void
flushln(struct CHAR *obuf, size_t obuf_size)
{
	int lastmode;
	size_t i;
	int hadmodes = 0;

	lastmode = NORMAL;
	for (i=0; i<maxcol; i++) {
		if (obuf[i].c_mode != lastmode) {
			hadmodes++;
			setulmode(obuf[i].c_mode);
			lastmode = obuf[i].c_mode;
		}
		if (obuf[i].c_char == '\0') {
			if (upln) {
				PRINT(cursor_right);
			}
			else {
				outc(' ');
			}
		} else
			outc(obuf[i].c_char);
	}
	if (lastmode != NORMAL) {
		setulmode(0);
	}
	if (must_overstrike && hadmodes)
		overstrike(obuf);
	putchar('\n');
	if (iflag && hadmodes)
		iattr(obuf);
	(void)fflush(stdout);
	if (upln)
		upln--;
	initbuf(obuf, obuf_size);
}

/*
 * For terminals that can overstrike, overstrike underlines and bolds.
 * We don't do anything with halfline ups and downs, or Greek.
 */
static void
overstrike(struct CHAR *obuf)
{
	size_t i;
	char lbuf[256];
	char *cp = lbuf;
	int hadbold=0;

	/* Set up overstrike buffer */
	for (i=0; i<maxcol; i++)
		switch (obuf[i].c_mode) {
		case NORMAL:
		default:
			*cp++ = ' ';
			break;
		case UNDERL:
			*cp++ = '_';
			break;
		case BOLD:
			*cp++ = obuf[i].c_char;
			hadbold=1;
			break;
		}
	putchar('\r');
	for (*cp=' '; *cp==' '; cp--)
		*cp = 0;
	for (cp=lbuf; *cp; cp++)
		putchar(*cp);
	if (hadbold) {
		putchar('\r');
		for (cp=lbuf; *cp; cp++)
			putchar(*cp=='_' ? ' ' : *cp);
		putchar('\r');
		for (cp=lbuf; *cp; cp++)
			putchar(*cp=='_' ? ' ' : *cp);
	}
}

static void
iattr(struct CHAR *obuf)
{
	size_t i;
	char lbuf[256];
	char *cp = lbuf;

	for (i=0; i<maxcol; i++)
		switch (obuf[i].c_mode) {
		case NORMAL:	*cp++ = ' '; break;
		case ALTSET:	*cp++ = 'g'; break;
		case SUPERSC:	*cp++ = '^'; break;
		case SUBSC:	*cp++ = 'v'; break;
		case UNDERL:	*cp++ = '_'; break;
		case BOLD:	*cp++ = '!'; break;
		default:	*cp++ = 'X'; break;
		}
	for (*cp=' '; *cp==' '; cp--)
		*cp = 0;
	for (cp=lbuf; *cp; cp++)
		putchar(*cp);
	putchar('\n');
}

static void
initbuf(struct CHAR *obuf, size_t obuf_size)
{

	memset(obuf, 0, obuf_size * sizeof(*obuf));	/* depends on NORMAL == 0 */
	col = 0;
	maxcol = 0;
	set_mode();
}

static void
set_mode(void)
{
	mode &= ALTSET;
}

static void
fwd(struct CHAR *obuf, size_t obuf_size)
{
	int oldcol, oldmax;

	oldcol = col;
	oldmax = maxcol;
	flushln(obuf, obuf_size);
	col = oldcol;
	maxcol = oldmax;
}

static void
reverse(struct CHAR *obuf, size_t obuf_size)
{
	upln++;
	fwd(obuf, obuf_size);
	PRINT(cursor_up);
	PRINT(cursor_up);
	upln++;
}

static int
outchar(int c)
{
	return (putchar(c & 0177));
}

static int curmode = 0;

static void
outc(int c)
{
	putchar(c);
	if (underline_char && !enter_underline_mode && (curmode & UNDERL)) {
		if (cursor_left)
			PRINT(cursor_left);
		else
			putchar('\b');
		PRINT(underline_char);
	}
}

static void
setulmode(int newmode)
{
	if (!iflag) {
		if (curmode != NORMAL && newmode != NORMAL)
			setulmode(NORMAL);
		switch (newmode) {
		case NORMAL:
			switch(curmode) {
			case NORMAL:
				break;
			case UNDERL:
				if (enter_underline_mode)
					PRINT(exit_underline_mode);
				else
					PRINT(exit_standout_mode);
				break;
			default:
				/* This includes standout */
				if (exit_attribute_mode)
					PRINT(exit_attribute_mode);
				else
					PRINT(exit_standout_mode);
				break;
			}
			break;
		case ALTSET:
			if (enter_reverse_mode)
				PRINT(enter_reverse_mode);
			else
				PRINT(enter_standout_mode);
			break;
		case SUPERSC:
			/*
			 * This only works on a few terminals.
			 * It should be fixed.
			 */
			PRINT(enter_underline_mode);
			PRINT(enter_dim_mode);
			break;
		case SUBSC:
			if (enter_dim_mode)
				PRINT(enter_dim_mode);
			else
				PRINT(enter_standout_mode);
			break;
		case UNDERL:
			if (enter_underline_mode)
				PRINT(enter_underline_mode);
			else
				PRINT(enter_standout_mode);
			break;
		case BOLD:
			if (enter_bold_mode)
				PRINT(enter_bold_mode);
			else
				PRINT(enter_reverse_mode);
			break;
		default:
			/*
			 * We should have some provision here for multiple modes
			 * on at once.  This will have to come later.
			 */
			PRINT(enter_standout_mode);
			break;
		}
	}
	curmode = newmode;
}

/*
 * Reallocates the buffer pointed to by *buf and sets
 * the newly allocated set of bytes to 0.
 */
static void
alloc_buf(struct CHAR **buf, size_t *size)
{
        size_t osize = *size;
        *size += MAXBUF;
        ereallocarr(buf, *size, sizeof(**buf));
        memset(*buf + osize, 0, (*size - osize) * sizeof(**buf));
}
