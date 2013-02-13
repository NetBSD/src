/*	$NetBSD: vis.c,v 1.18 2013/02/13 22:24:48 christos Exp $	*/

/*-
 * Copyright (c) 1989, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)vis.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: vis.c,v 1.18 2013/02/13 22:24:48 christos Exp $");
#endif /* not lint */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <wchar.h>
#include <limits.h>
#include <unistd.h>
#include <err.h>
#include <vis.h>

#include "extern.h"

static int eflags, fold, foldwidth = 80, none, markeol;
#ifdef DEBUG
int debug;
#endif
static char *extra;

static void process(FILE *);

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ch;
	int rval;

	while ((ch = getopt(argc, argv, "bcde:F:fhlmnostw")) != -1)
		switch((char)ch) {
		case 'b':
			eflags |= VIS_NOSLASH;
			break;
		case 'c':
			eflags |= VIS_CSTYLE;
			break;
#ifdef DEBUG
		case 'd':
			debug++;
			break;
#endif
		case 'e':
			extra = optarg;
			break;
		case 'F':
			if ((foldwidth = atoi(optarg)) < 5) {
				errx(1, "can't fold lines to less than 5 cols");
				/* NOTREACHED */
			}
			markeol++;
			break;
		case 'f':
			fold++;		/* fold output lines to 80 cols */
			break;		/* using hidden newline */
		case 'h':
			eflags |= VIS_HTTPSTYLE;
			break;
		case 'l':
			markeol++;	/* mark end of line with \$ */
			break;
		case 'm':
			eflags |= VIS_MIMESTYLE;
			if (foldwidth == 80)
				foldwidth = 76;
			break;
		case 'n':
			none++;
			break;
		case 'o':
			eflags |= VIS_OCTAL;
			break;
		case 's':
			eflags |= VIS_SAFE;
			break;
		case 't':
			eflags |= VIS_TAB;
			break;
		case 'w':
			eflags |= VIS_WHITE;
			break;
		case '?':
		default:
			(void)fprintf(stderr, 
			    "Usage: %s [-bcfhlmnostw] [-e extra]" 
			    " [-F foldwidth] [file ...]\n", getprogname());
			return 1;
		}

	if ((eflags & (VIS_HTTPSTYLE|VIS_MIMESTYLE)) ==
	    (VIS_HTTPSTYLE|VIS_MIMESTYLE))
		errx(1, "Can't specify -m and -h at the same time");

	argc -= optind;
	argv += optind;

	rval = 0;

	if (*argv)
		while (*argv) {
			if ((fp = fopen(*argv, "r")) != NULL) {
				process(fp);
				(void)fclose(fp);
			} else {
				warn("%s", *argv);
				rval = 1;
			}
			argv++;
		}
	else
		process(stdin);
	return rval;
}
	
static void
process(FILE *fp)
{
	static int col = 0;
	static char nul[] = "\0";
	char *cp = nul + 1;	/* so *(cp-1) starts out != '\n' */
	wint_t c, c1, rachar; 
	wchar_t ibuff[3]; /* room for c + rachar + NUL */
	char mbibuff[13]; /* ((sizeof(ibuff) - 1) * MB_LEN_MAX) + NUL */
	char buff[5]; /* max vis-encoding length for one char + NUL */
	
	c = getwc(fp);
	if (c == WEOF && errno == EILSEQ)
		c = (wint_t)getc(fp);
	while (c != WEOF) {
		rachar = getwc(fp);
		if (rachar == WEOF && errno == EILSEQ)
			rachar = (wint_t)getc(fp);
		if (none) {
			cp = buff;
			*cp++ = c;
			if (c == '\\')
				*cp++ = '\\';
			*cp = '\0';
		} else if (markeol && c == '\n') {
			cp = buff;
			if ((eflags & VIS_NOSLASH) == 0)
				*cp++ = '\\';
			*cp++ = '$';
			*cp++ = '\n';
			*cp = '\0';
		} else {
			c1 = rachar;
			if (c1 == WEOF)
				c1 = L'\0';
			swprintf(ibuff, 3, L"%lc%lc", c, c1);
			wcstombs(mbibuff, ibuff,
			    (wcslen(ibuff) * MB_LEN_MAX) + 1);
			(void) strsvisx(buff, mbibuff, 1, eflags, extra);
		}

		cp = buff;
		if (fold) {
#ifdef DEBUG
			if (debug)
				(void)printf("<%02d,", col);
#endif
			col = foldit(cp, col, foldwidth, eflags);
#ifdef DEBUG
			if (debug)
				(void)printf("%02d>", col);
#endif
		}
		do {
			(void)putchar(*cp);
		} while (*++cp);
		c = rachar;
	}
	/*
	 * terminate partial line with a hidden newline
	 */
	if (fold && *(cp - 1) != '\n')
		(void)printf(eflags & VIS_MIMESTYLE ? "=\n" : "\\\n");
}
