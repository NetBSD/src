/* ed.c: This file contains the main control and user-interface routines
   for the ed line editor. */
/*-
 * This code is derived from software by Brian W. Kernighan, Bell
 * Laboratories, Murray Hill, New Jersey and P. J. Plauger, Whitesmiths, Ltd.
 *
 * Kernighan/Plauger, "Software Tools in Pascal," (c) 1981 by
 * Addison-Wesley Publishing Company, Inc.  Reprinted with permission of
 * the publisher.
 */
/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Andrew Moore, Talke Studio.
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
char copyright1[] =
"@(#) Kernighan/Plauger, \"Software Tools in Pascal,\" (c) 1981 by\n\
 Addison-Wesley Publishing Company, Inc.  Reprinted with permission of\n\
 the publisher.\n";
char copyright2[] =
"@(#) Copyright (c) 1993 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ed.c	5.5 (Berkeley) 3/28/93";
#endif /* not lint */

/*
 * ED -- ED line editor
 * options:
 *	-p key	use key as the command prompt
 *	-s	run a script
 *	-	same as -s (though deprecated)
 *	-x	use crypt on file i/o
 *
 * Author: Andrew Moore
 *	   Talke Studio
 *	   485 Redwood Avenue, #5
 *	   Redwood City, CA 94061
 * Email:  alm@netcom.com
 *
 * Credits: The buf.c algorithm is attributed to Rodney Ruddock of
 *	    the University of Guelph, Guelph, Ontario.
 *
 *	    The cbc.c encryption code is adapted from
 *	    the bdes program by Matt Bishop of Dartmouth College,
 *	    Hanover, NH.
 *
 *	    The Addison-Wesley Publishing Company generously
 *	    granted permission to distribute this program over
 *	    Internet.
 *
 *	    Brian Beattie, Kees Bot and "others" did a C port of
 * 	    Kernighan and Pike's "Software Tools" editor.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <pwd.h>
#include <sys/ioctl.h>

#include "ed.h"

jmp_buf	env;
line_t	line0;			/* initial node of line queue */
int modified;			/* file-changed status */
char ibuf[MAXLINE];		/* input buffer */
char *ibufp;			/* input pointer */
long curln;			/* current line number */
long lastln;			/* last line number */
char fnp[MAXFNAME] = "";	/* output file name */
char *prompt;			/* command-line prompt */
char *defprompt = "*";		/* default prompt */
int lineno;			/* script line number */
struct winsize ws;		/* window size structure */
int rows = 22;			/* scroll length: ws_row - 2 */
int cols = 72;			/* wrap column: ws_col - 8 */


/* global flags */
int garrulous = 0;		/* if set, print all error messages */
int verbose = 1;		/* if set, print diagnostics */
int des = 0;			/* if set, use crypt for i/o */
int rtq = 0;			/* if set, ready to quit */
int interactive = 0;		/* if set, interactive command mode */
int mutex = 0;			/* if set, signals set "sigflags" */
int sigflags = 0;		/* if set, signals received while mutex set */

extern char errmsg[];
extern int optind;
extern char *optarg;

/* ed: line editor */
main(argc, argv)
	int argc;
	char **argv;
{
	int c, n;
	long status = 0;

	while ((c = getopt(argc, argv, "p:sx")) != EOF)
		switch(c) {
		case 'p':				/* set prompt */
			prompt = optarg;
			break;
		case 's':				/* run script */
			verbose = 0;
		   	break;
		case 'x':				/* use crypt */
#ifdef DES
			des = getkey();
#else
			fprintf(stderr, "crypt unavailable\n?\n");
#endif
			break;

		default:
			fprintf(stderr, "usage: ed [-p prompt] [-sx] [-] [file]\n");
			exit(1);
		}
	argv += optind;
	argc -= optind;
	if (argc && **argv == '-') {
		verbose = 0;
		argc--;
		argv++;
	}
	requeue(&line0, &line0);
	if (sbopen() < 0 || argc && doread(0, strcpy(fnp, *argv)) < 0) {
		fprintf(stderr, "%s\n?\n", errmsg);
		if (!isatty(0)) quit(2);
	}
	dowinch(SIGWINCH);
	/* assert: reliable signals! */
	if (isatty(0))
		signal(SIGWINCH, dowinch);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, onhup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
		if (!(status = setjmp(env)))
			signal(SIGINT, onintr);
	}
	freecmdv();
	interactive = 0;
	for (;;) {
		rtq = (!verbose || status == EMOD);	/* ready to quit? */
		if (status >= 0) *errmsg = '\0';
		if (garrulous && *errmsg) fprintf(stderr, "%s\n", errmsg);
		if (prompt) printf("%s", prompt);
		fflush(stdout);

		if ((n = getline(ibuf, sizeof ibuf)) == 0)
			if (modified && !rtq) {
				fputs("?\n", stderr);	/* give warning */
				sprintf(errmsg, "file modified");
				clearerr(stdin);
				status = EMOD;
				continue;
			} else	quit(0);

		if (ibuf[n - 1] != '\n') {		/* discard line */
			sprintf(errmsg, "unexpected EOF");
			clearerr(stdin);
			status = ERR;
			continue;
		}
		ibufp = ibuf;
		if ((n = getlist()) >= 0 && (status = ckglob()) != 0) {
		 	if (status > 0 && (status = doglob()) >= 0) {
				curln = status;
				continue;
			}
		} else if ((status = n) >= 0 && (status = docmd(0)) >= 0) {
		 	if (!status || status
		 	 && (status = doprnt(curln, curln, status)) >= 0)
				continue;
		}
		switch (status) {
		case EOF:
			quit(0);
		case EMOD:
			if (rtq) quit(0);
			fputs("?\n", stderr);		/* give warning */
			sprintf(errmsg, "file modified");
			break;
		default:
			fputs("?\n", stderr);
			if (!isatty(0)) {
				fprintf(stderr, garrulous ? "script, line %d: %s\n" : "", lineno, errmsg);
				quit(2);
			}
			break;
		}
	}
	/*NOTREACHED*/
}


long	line1, line2, nlines;

/* getlist: get line numbers from the command buffer until an illegal
   address is seen.  return range status */
getlist()
{
	long num;

	nlines = line2 = 0;
	while ((num = getone()) >= 0) {
		line1 = line2;
		line2 = num;
		nlines++;
		if (*ibufp != ',' && *ibufp != ';')
			break;
		else if (*ibufp++ == ';')
			curln = num;
	}
	nlines = min(nlines, 2);
	if (nlines == 0)
		line2 = curln;
	if (nlines <= 1)
		line1 = line2;
	return  (num == ERR) ? ERR : nlines;
}


/*  getone: return the next line number in the command buffer */
long
getone()
{
	int c;
	long i, num;

	if ((num = getnum(1)) < 0)
		return num;
	for (;;) {
		c = isspace(*ibufp);
		skipblanks();
		c = c && isdigit(*ibufp);
		if (!c && *ibufp != '+' && *ibufp != '-' && *ibufp != '^')
			break;
		c = c ? '+' : *ibufp++;
		if ((i = getnum(0)) < 0) {
			sprintf(errmsg, "invalid address");
			return  i;
		}
		if (c == '+')
			num += i;
		else	num -= i;
	}
	if (num > lastln || num < 0) {
		sprintf(errmsg, "invalid address");
		return ERR;
	}
	return num;
}

long	mark['z'-'a'+1];

/* getnum:  return a relative line number from the command buffer */
long
getnum(first)
	int first;
{
	pattern_t *srchpat;
	char c;

	skipblanks();
	if (isdigit(*ibufp))
		return strtol(ibufp, &ibufp, 10);
	switch(c = *ibufp) {
	case '.':
		ibufp++;
		return first ? curln : ERR;
	case '$':
		ibufp++;
		return first ? lastln : ERR;
	case '/':
	case '?':
		if ((srchpat = optpat()) == NULL)
			return ERR;
		else if (*ibufp == c)
			ibufp++;
		return first ? find(srchpat, (c == '/') ? 1 : 0) : ERR;
	case '^':
	case '-':
	case '+':
		return first ? curln : 1;
	case '\'':
		ibufp++;
		return (islower(*ibufp) && first) ? mark[*ibufp++ - 'a'] : ERR;
	case '%':
	case ',':
	case ';':
		if (first) {
			ibufp++;
			line2 = (c == ';') ? curln : 1;
			nlines++;
			return lastln;
		}
		return 1;
	default:
		return  first ? EOF : 1;
	}
}


/* gflags */
#define GLB 001		/* global command */
#define GPR 002		/* print after command */
#define GLS 004		/* list after command */
#define GNP 010		/* enumerate after command */
#define GSG 020		/* global substitute */


/* VRFYCMD: verify the command suffix in the command buffer */
#define VRFYCMD() { \
	int done = 0; \
	do { \
		switch(*ibufp) { \
		case 'p': \
			gflag |= GPR, ibufp++; \
			break; \
		case 'l': \
			gflag |= GLS, ibufp++; \
			break; \
		case 'n': \
			gflag |= GNP, ibufp++; \
			break; \
		default: \
			done++; \
		} \
	} while (!done); \
	if (*ibufp++ != '\n') { \
		sprintf(errmsg, "invalid command suffix"); \
		return ERR; \
	} \
}


int Gflag = 0;			/* interactive global command suffix */

/* ckglob:  mark lines matching a pattern in the command buffer; return
   global status  */
ckglob()
{
	pattern_t *glbpat;
	char c, delim;
	char *s;
	int nomatch;
	long n;
	line_t *ptr;
	int gflag = 0;

	if ((c = *ibufp) == 'V' || c == 'G')
		interactive++;
	else if (c != 'g' && c != 'v')
		return 0;
	if (deflt(1, lastln) < 0 || (delim = *++ibufp) == ' ' || delim == '\n'
	 || (glbpat = optpat()) == NULL)
		return ERR;
	if (*ibufp == delim)
		ibufp++;
	if (interactive) {
		VRFYCMD();				/* set gflag */
		Gflag = gflag;
	}
	for (n = 1, ptr = getptr(n); n <= lastln; n++, ptr = ptr->next) {
		if ((s = gettxt(ptr)) == (char *) ERR)
			return ERR;
		ptr->len &= ~ACTV;			/* zero ACTV  bit */
		if (line1 <= n && n <= line2
		 && (!(nomatch = regexec(glbpat, s, 0, NULL, 0))
		 && (c == 'g'  || c == 'G')
		 || nomatch && (c == 'v' || c == 'V')))
			ptr->len |= ACTV;
	}
	return 1;
}


/* doglob: apply command list in the command buffer to the active
   lines in a range; return command status */
long
doglob()
{
	line_t *ptr = NULL;
	long lc;
	int status;
	int n;
	char *cmd = NULL;

	if (!interactive && (cmd = getcmdv()) == NULL)
		return ERR;
	ureset();
	for (;;) {
		for (lc = 1, ptr = getptr(lc); lc <= lastln; lc++, ptr = ptr->next)
			if (ptr->len & ACTV)		/* active line */
				break;
		if (lc > lastln)
			break;
		ptr->len ^= ACTV;			/* zero ACTV bit */
		curln = lc;
		if (interactive) {
			/* print curln and get a command in global syntax */
			if (doprnt(curln, curln, 0) < 0) {
				interactive = 0;
				freecmdv();
				return ERR;
			}
			while ((n = getline(ibufp = ibuf, sizeof ibuf))
			    && ibuf[n - 1] != '\n')
				clearerr(stdin);
			if (!n) {
				sprintf(errmsg, "unexpected end of file");
				interactive = 0;
				freecmdv();
				return ERR;
			}
			if (!strcmp(ibuf, "\n"))
				continue;
			if (!strcmp(ibuf, "&\n")) {
				if (cmd == NULL) {
					sprintf(errmsg, "null command");
					interactive = 0;
					freecmdv();
					return ERR;
				}
			} else {
				freecmdv();
				if ((cmd = getcmdv()) == NULL) {
					interactive = 0;
					return ERR;
				}
			}
		}
		ibufp = cmd;
		for (; *ibufp;)
			if ((status = getlist()) < 0
			 || (status = docmd(1)) < 0
			 || (status > 0
			 && (status = doprnt(curln, curln, status)) < 0)) {
				freecmdv();
				interactive = 0;
				return status;
			}
	}
	if (interactive && Gflag && doprnt(curln, curln, Gflag) < 0)
		return ERR;
	freecmdv();
	interactive = 0;
	return curln;
}


/* GETLINE3: get a legal address from the command buffer */
#define GETLINE3(num) \
{ \
	long ol1, ol2; \
\
	ol1 = line1, ol2 = line2; \
	if (getlist() < 0) \
		return ERR; \
	if (line2 < 0 || lastln < line2) { \
		sprintf(errmsg, "invalid address"); \
		return ERR; \
	} \
	num = line2; \
	line1 = ol1, line2 = ol2; \
}

/* sgflags */
#define SGG 001		/* complement previous global substitute suffix */
#define SGP 002		/* complement previous print suffix */
#define SGR 004		/* use last regex instead of last subpat */
#define SGF 010		/* newline found */

int usw = 0;		/* if set, undo last undo */
int patlock = 0;	/* if set, pattern not released by optpat() */

/* docmd: execute the next command in command buffer; return print
   request, if any */
docmd(glob)
	int glob;
{
	static pattern_t *subpat = NULL;
	static int sgflag = 0;
	static char rhs[MAXLINE];
	static char cmdbuf[MAXLINE];

	pattern_t *tpat;
	int c = 0;
	char *fptr;
	int gflag = 0;
	long num;
	int changed;
	int subflags = 0;
	char *sp, *tp;

	skipblanks();
	switch(*ibufp++) {
	case '\n':
		if (deflt(curln + !glob, curln + !glob) < 0
		 || doprnt(line2, line2, 0) < 0)
			return ERR;
		break;
	case '!':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		if (*ibufp == '!') {
			subflags++;
			ibufp++;
		} else {
			cmdbuf[0] = '\0';
			for (;;) {
				/* replace unescaped %s with file name */
				if (sp = strchr(ibufp, '%'))
					for (tp = ibufp; sp && oddesc(tp, sp);)
						sp = strchr((tp = sp) + 1, '%');
				if (sp) {
					subflags++;
					if (strlen(cmdbuf) + strlen(fnp) > sizeof cmdbuf) {
						sprintf(errmsg, "command too long");
						return ERR;
					}
					strncat(cmdbuf, ibufp, sp - ibufp);
					strcat(cmdbuf, fnp);
					ibufp = sp + 1;
				} else {
					sp = strchr(ibufp, '\n');
					if (strlen(cmdbuf) + sp - ibufp + 1 > sizeof cmdbuf) {
						sprintf(errmsg, "command too long");
						return ERR;
					}
					strncat(cmdbuf, ibufp, sp - ibufp + 1);
					ibufp = sp;
					break;
				}
			}
		}
		VRFYCMD();
		if (subflags) printf("%s", cmdbuf);
		system(cmdbuf);
		if (verbose) printf("!\n");
		break;
	case '=':
		VRFYCMD();
		printf("%d\n", nlines ? line2 : lastln);
		break;
	case 'a':
		VRFYCMD();
		if (!glob) ureset();
		if (append(line2, glob) < 0)
			return ERR;
		break;
	case 'c':
		if (deflt(curln, curln) < 0)
			return ERR;
		VRFYCMD();
		if (!glob) ureset();
		if (del(line1, line2) < 0 || append(curln, glob) < 0)
			return ERR;
		break;
	case 'd':
		if (deflt(curln, curln) < 0)
			return ERR;
		VRFYCMD();
		if (!glob) ureset();
		if (del(line1, line2) < 0)
			return ERR;
		if (nextln(curln, lastln) != 0)
			curln = nextln(curln, lastln);
		modified = TRUE;
		break;
	case 'e':
		if (modified && !rtq)
			return EMOD;
		/* fall through */
	case 'E':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		} else if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		}
		if ((fptr = getfn()) == NULL)
			return ERR;
		VRFYCMD();
		del(1, lastln);
		ureset();
		sbclose();
		if (sbopen() < 0)
			return ERR;
		if (*fptr && *fptr != '!') strcpy(fnp, fptr);
		if (doread(0, *fptr ? fptr : fnp) < 0) {
			if (!garrulous) fprintf(stderr, "%s\n", errmsg);
			return ERR;
		}
		ureset();
		bzero(mark, sizeof mark);
		modified = FALSE;
		break;
	case 'f':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		} else if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		}
		if ((fptr = getfn()) == NULL)
			return ERR;
		VRFYCMD();
		if (*fptr) strcpy(fnp, fptr);
		printf("%s\n", fnp);
		break;
	case 'H':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		VRFYCMD();
		if ((garrulous = 1 - garrulous) && *errmsg)
			fprintf(stderr, "%s\n", errmsg);
		break;
	case 'h':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		VRFYCMD();
		if (*errmsg) fprintf(stderr, "%s\n", errmsg);
		break;
	case 'i':
		VRFYCMD();
		if (!glob) ureset();
		if (append(prevln(line2, lastln), glob) < 0)
			return ERR;
		break;
	case 'j':
		if (deflt(curln, curln + 1) < 0)
			return ERR;
		VRFYCMD();
		if (!glob) ureset();
		if (join(line1, line2) < 0)
			return ERR;
		break;
	case 'k':
		if (!islower(c = *ibufp++)) {
			sprintf(errmsg, "invalid mark character");
			return ERR;
		}
		VRFYCMD();
		mark[c-'a'] = line2;
		break;
	case 'l':
		if (deflt(curln, curln) < 0)
			return ERR;
		gflag |= GLS;
		VRFYCMD();
		if (doprnt(line1, line2, gflag) < 0)
			return ERR;
		gflag = 0;
		break;
	case 'm':
		/*  Clean but inefficient move.
		    The inefficiency is outweighed since the requeue
		    alternative requires special handling by undo(). */
		if (deflt(curln, curln) < 0)
			return ERR;
		GETLINE3(num);
		if (line1 <= num && num < line2) {
			sprintf(errmsg, "invalid destination");
			return ERR;
		}
		VRFYCMD();
		if (!glob) ureset();
		if (transfer(num) < 0)
			return ERR;
		c = (num > line1) ? 0 : line2 - line1 + 1;
		if (del(line1 + c, line2 + c) < 0)
			return ERR;
		curln =  num + c;
		modified = TRUE;
		break;
	case 'n':
		if (deflt(curln, curln) < 0)
			return ERR;
		gflag |= GNP;
		VRFYCMD();
		if (doprnt(line1, line2, gflag) < 0)
			return ERR;
		gflag = 0;
		break;
	case 'P':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		VRFYCMD();
		prompt = prompt ? NULL : optarg ? optarg : defprompt;
		break;
	case 'p':
		if (deflt(curln, curln) < 0)
			return ERR;
		gflag |= GPR;
		VRFYCMD();
		if (doprnt(line1, line2, gflag) < 0)
			return ERR;
		gflag = 0;
		break;
	case 'q':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		VRFYCMD();
		if (modified)
			return EMOD;
		return (nlines == 0 && !glob) ? EOF : ERR;
	case 'Q':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		VRFYCMD();
		return EOF;
	case 'r':
		if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		}
		if (!nlines) line2 = lastln;
		if ((fptr = getfn()) == NULL)
			return ERR;
		VRFYCMD();
		if (!glob) ureset();
		if (*fnp == '\0' && *fptr != '!') strcpy(fnp, fptr);
		if ((num = doread(line2, *fptr ? fptr : fnp)) < 0) {
			if (!garrulous) fprintf(stderr, "%s\n", errmsg);
			return ERR;
		}
		else if (num != lastln)
			modified = TRUE;
		break;
	case 's':
		do {
			switch(*ibufp) {
			case '\n':
				subflags |=SGF;
				break;
			case 'g':
				subflags |= SGG;
				ibufp++;
				break;
			case 'p':
				subflags |= SGP;
				ibufp++;
				break;
			case 'r':
				subflags |= SGR;
				ibufp++;
				break;
			default:
				if (subflags) return ERR;
			}
		} while (subflags && *ibufp != '\n');
		if (!(subflags & SGF))
			sgflag &= 0xff;
		tpat = subpat;
		spl1();
		if ((!subflags || (subflags & SGR))
		 && (tpat = optpat()) == NULL)
			return ERR;
		if (tpat != subpat) {
			if (subpat) {
				 regfree(subpat);
				 free(subpat);
			 }
			subpat = tpat;
			patlock = 1;		/* reserve pattern */
		}
		spl0();
		if (!subflags && (sgflag = getrhs(rhs, glob)) < 0)
			return ERR;
		if (glob)
			sgflag |= GLB;
		else
			sgflag &= ~GLB;
		if (subflags & SGG)
			sgflag ^= GSG;
		if (subflags & SGP)
			sgflag ^= GPR, sgflag &= ~GLS;
		do {
			switch(*ibufp) {
			case 'p':
				sgflag |= GPR, ibufp++;
				break;
			case 'l':
				sgflag |= GLS, ibufp++;
				break;
			case 'n':
				sgflag |= GNP, ibufp++;
				break;
			default:
				c++;
			}
		} while (!c);
		if (deflt(curln, curln) < 0)
			return ERR;
		VRFYCMD();
		if (!glob) ureset();
		if ((changed = subst(subpat, rhs, sgflag)) < 0)
			return ERR;
		else if (changed)
			modified = TRUE;
		break;
	case 't':
		if (deflt(curln, curln) < 0)
			return ERR;
		GETLINE3(num);
		VRFYCMD();
		if (!glob) ureset();
		if (transfer(num) < 0)
			return ERR;
		modified = TRUE;
		break;
	case 'u':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		VRFYCMD();
		if (undo() < 0)
			return ERR;
		break;
	case 'W':
	case 'w':
		if (*ibufp == 'q') {
			gflag = EOF;
			ibufp++;
		} else if (!isspace(*ibufp)) {
			sprintf(errmsg, "unexpected command suffix");
			return ERR;
		}
		if ((fptr = getfn()) == NULL)
			return ERR;
		if (nlines == 0 && !lastln)
			line1 = line2 = 0;
		else if (deflt(1, lastln) < 0)
			return ERR;
		VRFYCMD();
		if (*fnp == '\0' && *fptr != '!') strcpy(fnp, fptr);
		if (dowrite(line1, line2, *fptr ? fptr : fnp, c == 'W' ? "a" : "w") < 0) {
			if (!garrulous) fprintf(stderr, "%s\n", errmsg);
			return ERR;
		}
		modified = FALSE;
		break;
	case 'x':
		if (nlines > 0) {
			sprintf(errmsg, "unexpected address");
			return ERR;
		}
		VRFYCMD();
#ifdef DES
		des = getkey();
#else
		sprintf(errmsg, "crypt unavailable");
		return ERR;
#endif
		break;
	case 'z':
		if (deflt(curln + 1, curln + 1) < 0)
			return ERR;
		if (isdigit(*ibufp))
			rows = strtol(ibufp, &ibufp, 10);
		VRFYCMD();
		if (doprnt(line1, min(lastln, line1 + rows - 1), gflag) < 0)
			return ERR;
		gflag = 0;
		break;
	default:
		sprintf(errmsg, "unknown command");
		return ERR;
	}
	return gflag;
}


/* deflt: return status of line number range check */
deflt(def1, def2)
	long def1, def2;
{
	if (nlines == 0) {
		line1 = def1;
		line2 = def2;
	}
	if (line1 > line2 || 1 > line1 || line2 > lastln) {
		sprintf(errmsg, "invalid address");
		return ERR;
	}
	return 0;
}


/* find: return the number of the next line matching a pattern in a
   given direction.  wrap around begin/end of line queue if necessary */
long
find(pat, dir)
	pattern_t *pat;
	int dir;
{
	char *s;
	long n = curln;

	do {
		n = dir ? nextln(n, lastln) : prevln(n, lastln);
		if ((s = gettxt(getptr(n))) == (char *) ERR)
			return ERR;
		else if (s && !regexec(pat, s, 0, NULL, 0))
			return n;
	} while (n != curln);
	sprintf(errmsg, "no match");
	return  ERR;
}


/* getfn: return pointer to file name in the command buffer */
char *
getfn()
{
	static char file[MAXFNAME];
	char *cp = file;

	if (*ibufp != '\n') {
		skipblanks();
		if (*ibufp == '\n' || *ibufp == '\\' && *(ibufp + 1) == '\n') {
			sprintf(errmsg, "invalid file name");
			return NULL;
		}
	} else if (*fnp == '\0') {
		sprintf(errmsg, "no current file name");
		return  NULL;
	}
	while ((*cp = *ibufp) != '\n' && cp - file < sizeof file)
		cp++, ibufp++;
	if (cp - file == sizeof file) {
		sprintf(errmsg, "name too long");
		return NULL;
	}
	*cp = '\0';
	return  file;
}

/* getrhs: extract substitution template from the command buffer */
getrhs(sub, glob)
	char *sub;
	int glob;
{
	char delim;

	if ((delim = *ibufp) == '\n') {
		*sub = '\0';
		return GPR;
	} else if (makesub(sub, MAXLINE, glob) == NULL)
		return  ERR;
	else if (*ibufp == '\n')
		return GPR;
	else if (*ibufp == delim)
		ibufp++;
	if ('1' <= *ibufp && *ibufp <= '9')
		return (int) strtol(ibufp, &ibufp, 10) << 8;
	else if (*ibufp == 'g') {
		ibufp++;
		return GSG;
	}
	return  0;
}


/* makesub: return pointer to substitution template in the command buffer */
char *
makesub(sub, subsz, glob)
	char *sub;
	int subsz;
	int glob;
{
	int n = 0;
	int size = 0;
	char *cp = sub;
	char delim = *ibufp++;
	char c;

	if (*ibufp == '%' && *(ibufp + 1) == delim) {
		ibufp++;
		return sub;
	}
	for (; *ibufp != delim && size < subsz; size++)
		if ((c = *cp++ = *ibufp++) == '\n' && *ibufp == '\0') {
			cp--, ibufp--;
			break;
		} else if (c != ESCHAR)
			;
		else if (++size == subsz)
			break;
		else if ((*cp++ = *ibufp++) != '\n')
			;
		else if (!glob)
			while ((n = getline(ibufp = ibuf, sizeof ibuf)) == 0
			    || ibuf[n - 1] != '\n')
				clearerr(stdin);
		else
		/*NOTREACHED*/
			;
	if (size >= subsz) {
		sprintf(errmsg, "substitution too long");
		return  NULL;
	}
	*cp = '\0';
	return  sub;
}


/* append: insert text from stdin to after line n; stop when either a
   single period is read or EOF; return status */
append(n, glob)
	long n;
	int  glob;
{
	int l;
	char lin[MAXLINE];
	char *lp = lin;
	undo_t *up = NULL;

	for (curln = n;;) {
		if (!glob) {
			if ((l = getline(lin, sizeof lin)) == 0
			 || lin[l - 1] != '\n') {
				clearerr(stdin);
				return  l ? EOF : 0;
			}
		} else if (*(lp = ibufp) == '\0')
			return 0;
		else
			while (*ibufp++ != '\n')
				;
		if (lp[0] == '.' && lp[1] == '\n') {
			return 0;
		}
		spl1();
		if (puttxt(lp) == (char *) ERR) {
			spl0();
			return ERR;
		}
		if (up)	up->t = getptr(curln);
		else	up = getuptr(UADD, curln, curln);
		spl0();
		modified = 1;
	}
}

#ifdef sun
/* subst: change all text matching a pattern in a range of lines according to
   a substitution template; return status  */
subst(pat, sub, gflag)
	pattern_t *pat;
	char *sub;
	int gflag;
{
	undo_t *up = NULL;
	char *new;
	line_t *bp, *ep, *np;
	long ocl;
	long nsubs = 0;

	ep = getptr(curln = line2);
	for (bp = getptr(line1); bp != ep->next; bp = bp->next)
		if ((new = gettxt(bp)) == (char *) ERR)
			return ERR;
		else if ((new = regsub(pat, new, sub, gflag)) == (char *) ERR)
			return ERR;
		else if (!new) {
			/* add copy of bp after current line - this avoids
			   overloading the undo structure, since only two
			   undo nodes are needed for the whole substitution;
			   the cost is high, but the less than if undo is
			   overloaded. */
			if ((np = lpdup(bp)) == NULL)
				return ERR;
			spl1();
			lpqueue(np);
			if (up)
				up->t = getptr(curln);
			else	up = getuptr(UADD, curln, curln);
			spl0();
		} else {
			spl1();
			do {
				if ((new = puttxt(new)) == (char *) ERR) {
					spl0();
					return ERR;
				}
				if (up)
					up->t = getptr(curln);
				else	up = getuptr(UADD, curln, curln);
			} while (new != NULL);
			spl0();
			nsubs++;
			if ((gflag & (GPR | GLS))
			 && doprnt(curln, curln, gflag) < 0)
				return ERR;
		}
	ocl = curln;
	del(line1, line2);
	curln = ocl - (line2 - line1 + 1);
	if  (nsubs == 0 && !(gflag & GLB)) {
		sprintf(errmsg, "no match");
		return ERR;
	}
	return 1;
}
#else	/* sun */


/* subst: change all text matching a pattern in a range of lines according to
   a substitution template; return status  */
subst(pat, sub, gflag)
	pattern_t *pat;
	char *sub;
	int gflag;
{
	undo_t *up;
	char *new;
	long lc;
	int nsubs = 0;

	curln = prevln(line1, lastln);
	for (lc = 0; lc <= line2 - line1; lc++) {
		new = gettxt(getptr(curln = nextln(curln, lastln)));
		if (new == (char *) ERR)
			return ERR;
		else if ((new = regsub(pat, new, sub, gflag)) == (char *) ERR)
			return ERR;
		else if (new) {
			up = NULL;
			del(curln, curln);
			spl1();
			do {
				if ((new = puttxt(new)) == (char *) ERR) {
					spl0();
					return ERR;
				}
				if (up)	up->t = getptr(curln);
				else	up = getuptr(UADD, curln, curln);
			} while (new != NULL);
			spl0();
			nsubs++;
			if ((gflag & (GPR | GLS))
			 && doprnt(curln, curln, gflag) < 0)
				return ERR;
		}
	}
	if  (nsubs == 0 && !(gflag & GLB)) {
		sprintf(errmsg, "no match");
		return ERR;
	}
	return 1;
}
#endif	/* sun */


/* regsub: substitute possibly several expressions in a line of text
   matched by a pattern according to a substitution template; return
   pointer to the modified text */
char *
regsub(pat, txt, sub, gflag)
	pattern_t *pat;
	char *txt;
	char *sub;
	int gflag;
{
	static char buf[MAXLINE];

	char *eob = buf + sizeof buf - 1;	/* -1 for '\n' */
	char *new = buf;
	int kth = gflag >> 8;		/* substitute kth match only */
	int chngd = 0;
	int matchno = 0;
	int n;
	regmatch_t rm[SE_MAX];

	if (!regexec(pat, txt, SE_MAX, rm, 0)) {
		do {
			if (!kth || kth == ++matchno) {
				chngd++;
				if ((n = rm[0].rm_so) >=  eob - new) {
					sprintf(errmsg, "line too long");
					return (char *) ERR;
				}
				strncpy(new, txt, n);
				if (!(new = subcat(txt, rm, sub, new += n, eob))) {
					sprintf(errmsg, "line too long");
					return (char *) ERR;
				}
			} else {
				if ((n = rm[0].rm_eo) >=  eob - new) {
					sprintf(errmsg, "line too long");
					return (char *) ERR;
				}
				strncpy(new, txt, n);
				new += n;
			}
			txt += rm[0].rm_eo;
		} while (*txt && (!chngd || (gflag & GSG) && rm[0].rm_eo)
		      && !regexec(pat, txt, SE_MAX, rm, REG_NOTBOL));
		if ((n = strlen(txt)) >= eob - new) {
			sprintf(errmsg, "line too long");
			return  (char *) ERR;
		} else if (n > 0 && !rm[0].rm_eo && (gflag & GSG)) {
			sprintf(errmsg, "infinite loop");
			return  (char *) ERR;
		}
		strcpy(new, txt);
		strcpy(new + n, "\n");
	}
	return chngd ? buf : NULL;
}


/* join: replace a range of lines with the joined text of those lines */
join(from, to)
	long from;
	long to;
{
	char buf[MAXLINE] = "";
	char *s;
	int len = 0;
	int size = 0;
	line_t *bp, *ep;

	if (from == to)
		return 0;
	ep = getptr(nextln(to, lastln));
	for (bp = getptr(from); bp != ep; bp = bp->next, size += len) {
		if ((s = gettxt(bp)) == (char *) ERR)
			return ERR;
		if (size + (len = strlen(s)) >= sizeof buf - 1) {
			sprintf(errmsg, "line too long");
			return ERR;
		}
		strcpy(buf + size, s);
	}
	strcpy(buf + size, "\n");
	del(from, to);
	curln = from - 1;
	spl1();
	if (puttxt(buf) == (char *) ERR) {
		spl0();
		return ERR;
	}
	getuptr(UADD, curln, curln);
	spl0();
	modified = TRUE;
	return 0;
}


/* transfer: copy a range of lines; return status */
transfer(num)
	long num;
{
	line_t *lp;
	long nl, nt, lc;
	long mid = (num < line2) ? num : line2;
	undo_t *up = NULL;

	curln = num;
	for (nt = 0, nl = line1; nl <= mid; nl++, nt++) {
		spl1();
		if ((lp = lpdup(getptr(nl))) == NULL) {
			spl0();
			return ERR;
		}
		lpqueue(lp);
		if (up)	up->t = lp;
		else	up = getuptr(UADD, curln, curln);
		spl0();
	}
	for (nl += nt, lc = line2 + nt; nl <= lc; nl += 2, lc++) {
		spl1();
		if ((lp = lpdup(getptr(nl))) == NULL) {
			spl0();
			return ERR;
		}
		lpqueue(lp);
		if (up)	up->t = lp;
		else	up = getuptr(UADD, curln, curln);
		spl0();
	}
	return 0;
}


/* del: delete a range of lines */
del(from, to)
	long from, to;
{
	line_t *before, *after;

	spl1();
	getuptr(UDEL, from, to);
	after = getptr(nextln(to, lastln));
	before = getptr(prevln(from, lastln));	/* this getptr last! */
	requeue(before, after);
	lastln -= to - from + 1;
	curln = prevln(from, lastln);
	spl0();
	return 0;
}


/* subcat: modify text according to a substitution template;
   return pointer to modified text */
char *
subcat(boln, rm, sub, new, newend)
	char *boln;
	regmatch_t *rm;
	char *sub, *new, *newend;
{
	char *from = boln + rm[0].rm_so;
	char *to = boln + rm[0].rm_eo;

	char *cp = new, *cp2, *cp3;

	for (; *sub && cp < newend ; sub++)
		if (*sub == DITTO)
			for (cp2 = from; cp2 < to;) {
				*cp++ = *cp2++;
				if (cp >= newend)
					break;
			}
		else if (*sub == ESCHAR && '1' <= *++sub && *sub <= '9'
		      && rm[*sub - '0'].rm_so >= 0
		      && rm[*sub - '0'].rm_eo >= 0) {
			cp2 = boln + rm[*sub - '0'].rm_so;
			cp3 = boln + rm[*sub - '0'].rm_eo;
			while (cp2 < cp3) {
				*cp++ = *cp2++;
				if (cp >= newend)
					break;
			}
		} else *cp++ = *sub;
	if (cp >= newend)
		return NULL;
	*cp = '\0';
	return cp;
}

/* doprnt: print a range of lines to stdout */
doprnt(from, to, gflag)
	long from;
	long to;
	int gflag;
{
	line_t *bp;
	line_t *ep;
	char *s;

	ep = getptr(nextln(to, lastln));
	for (bp = getptr(from); bp != ep; bp = bp->next) {
		if ((s = gettxt(bp)) == (char *) ERR)
			return ERR;
		prntln(s, curln = from++, gflag);
	}
	return 0;
}


/* prntln: print text to stdout */
void
prntln(s, n, gflag)
	char *s;
	long n;
	int gflag;
{
	int col = 0;

	if (gflag & GNP) {
		printf("%ld\t", n);
		col = 8;
	}
	for (; *s; s++) {
		if ((gflag & GLS) && ++col > cols) {
			fputs("\\\n", stdout);
			col = 1;
		}
		if (gflag & GLS) {
			switch (*s) {
			case '\b':
				fputs("\\b", stdout);
				break;
			case '\f':
				fputs("\\f", stdout);
				break;
			case '\n':
				fputs("\\n", stdout);
				break;
			case '\r':
				fputs("\\r", stdout);
				break;
			case '\t':
				fputs("\\t", stdout);
				break;
			case '\v':
				fputs("\\v", stdout);
				break;
			default:
				if (*s < 32 || 126 < *s) {
					putchar('\\');
					putchar((*s >> 6) + '0');
					putchar((*s >> 3) + '0');
					putchar((*s & 07) + '0');
					col += 2;
				} else if (*s == '\\')
					fputs("\\\\", stdout);
				else {
					putchar(*s);
					col--;
				}
			}
			col++;
		} else
			putchar(*s);
	}
	if (gflag & GLS)
		putchar('$');
	putchar('\n');
}


int nullchar;		/* null chars read */
int newline_added;		/* long record split across lines */

/* doread: read a text file into the editor buffer; return line count */
long
doread(n, fn)
	long n;
	char *fn;
{
	FILE *fp;
	char buf[MAXLINE];
	unsigned long size = 0;
	undo_t *up = NULL;
	int len;
	line_t *lp = getptr(n);

	nullchar = newline_added = 0;
	if ((fp = (*fn == '!') ? popen(fn + 1, "r") : desopen(esctos(fn), "r")) == NULL) {
		sprintf(errmsg, "cannot open file");
		return ERR;
	}
	for (curln = n; (len = sgetline(buf, MAXLINE, fp)) > 0; size += len) {
		spl1();
		if (puttxt(buf) == (char *) ERR) {
			spl0();
			return ERR;
		}
		lp = lp->next;
		if (up)	up->t = lp;
		else	up = getuptr(UADD, curln, curln);
		spl0();
	}
	(*fn == '!') ?  pclose(fp) : fclose(fp);
	if (nullchar)
		fputs("null(s) discarded\n", stderr);
	if (newline_added)
		fputs("newline(s) added\n", stderr);
	if (des) size += 8 - size % 8;
	fprintf(stderr, verbose ? "%lu\n" : "", size);
	return  curln - n;
}


/* dowrite: write the text of a range of lines to a file; return line count */
long
dowrite(n, m, fn, mode)
	long n;
	long m;
	char *fn;
	char *mode;
{
	FILE *fp;
	unsigned long size = 0;
	char *s = NULL;
	line_t *lp;

	if ((fp = ((*fn == '!') ? popen(fn + 1, "w") : desopen(esctos(fn), mode))) == NULL) {
		sprintf(errmsg, "cannot open file");
		return ERR;
	}
	if (n && !des)
		for (lp = getptr(n); n <= m; n++, lp = lp->next) {
			if ((s = gettxt(lp)) == (char *) ERR)
				return ERR;
			else if ((fputs(s, fp) < 0 || putc('\n', fp) < 0)) {
				sprintf(errmsg, "cannot write file");
				return ERR;
			}
			size += (lp->len & ~ACTV) + 1;		/* +1 '\n' */
		}
	else if (n)
		for (lp = getptr(n); n <= m; n++, lp = lp->next) {
			if ((s = gettxt(lp)) == (char *) ERR)
				return ERR;
			while (*s)
				if (desputc(*s++, fp) < 0) {
					sprintf(errmsg, "cannot write file");
					return ERR;
				}
			if (desputc('\n', fp) < 0) {
				sprintf(errmsg, "cannot write file");
				return ERR;
			}
			size += (lp->len & ~ACTV) + 1;		/* +1 '\n' */
		}
	if (des) {
		desputc(EOF, fp);			/* flush buffer */
		size += 8 - size % 8;			/* adjust DES size */
	}
	(*fn == '!') ?  pclose(fp) : fclose(fp);
	fprintf(stderr, verbose ? "%lu\n" : "", size);
	return  n ? m - n + 1 : 0;
}


#define USIZE 100				/* undo stack size */
undo_t *ustack = NULL;				/* undo stack */
long usize = 0;					/* stack size variable */
long u_p = 0;					/* undo stack pointer */
long ucurln;


/* getuptr: return pointer to intialized undo node */
undo_t *
getuptr(type, from, to)
	int type;
	long from;
	long to;
{
	undo_t *t;

#if defined(sun) || defined(NO_REALLOC_NULL)
	if (ustack == NULL
	 && (ustack = (undo_t *) malloc((usize = USIZE) * sizeof(undo_t))) == NULL)
		return NULL;
#endif
	t = ustack;
	if (u_p < usize
	 || (t = (undo_t *) realloc(ustack, (usize += USIZE) * sizeof(undo_t))) != NULL) {
		ustack = t;
		ustack[u_p].type = type;
		ustack[u_p].t = getptr(to);
		ustack[u_p].h = getptr(from);
		return ustack + u_p++;
	}
	/* out of memory - release undo stack */
	ureset();
	free(ustack);
	ustack = NULL;
	usize = 0;
	return NULL;
}

/* undo: undo last change to the editor buffer */
undo()
{
	long n = usw ? 0 : u_p - 1;
	int i = usw ? 1 : -1;
	long j = u_p;
	long ocurln = curln;

	if (u_p == 0) {
		sprintf(errmsg, "file unmodified");
		return ERR;
	}
	spl1();
	for (; j--; n += i)
		switch(ustack[n].type ^ usw) {
		case UADD:
			getptr(0);		/* this getptr last! */
			requeue(ustack[n].h->prev, ustack[n].t->next);
			lastln -= getrange(ustack[n].h, ustack[n].t);
			break;
		case UDEL:
			getptr(0);		/* this getptr last! */
			requeue(ustack[n].h->prev, ustack[n].h);
			requeue(ustack[n].t, ustack[n].t->next);
			lastln += getrange(ustack[n].h, ustack[n].t);
			break;
		default:
			/*NOTREACHED*/
			;
		}
	curln = ucurln;
	ucurln = ocurln;
	usw = 1 - usw;
	spl0();
	return 0;
}


/* ureset: clear the undo stack */
void
ureset()
{
	line_t *lp, *ep, *tl;

	while (u_p--)
		if ((ustack[u_p].type ^ usw) == UDEL) {
			ep = ustack[u_p].t->next;
			for (lp = ustack[u_p].h; lp != ep; lp = tl) {
				tl = lp->next;
				free(lp);
			}
		}
	u_p = usw = 0;
	ucurln = curln;
}



/* sgetline: read a line of text up a maximum size from a file; return
   line length */
sgetline(buf, size, fp)
	char *buf;
	int size;
	FILE *fp;
{
	int c = EOF;
	char *cp = buf;

	/* pull des out of loop */

	if (!des)
		while (cp - buf < size - 1 && (c = getc(fp)) != EOF && c != '\n')
			c && (*cp++ = c) || (nullchar = 1);
	else
		while (cp - buf < size - 1 && (c = desgetc(fp)) != EOF && c != '\n')
			c && (*cp++ = c) || (nullchar = 1);
	if (c == '\n')
		*cp++ = c;
	else if (c == EOF) {
		if (cp - buf) {
			*cp++ = '\n';
		 	newline_added = 1;
		}
	} else {
		ungetc(c, fp);
		*(cp - 1) = '\n';
		newline_added = 1;
	}
	*cp = '\0';
	return cp - buf;
}


int sg_ct = 0;			/* count for desgetc */
int sp_ct = 0;			/* count for desputc */

/* desopen: open a file and initialize DES; return file pointer */
FILE *
desopen(fn, mode)
	char *fn;
	char *mode;
{
	FILE *fp;

#ifdef DES
	sg_ct = 0;
	sp_ct = 0;
	cbcinit();
#endif
	if ((fp = fopen(fn, mode)) != NULL)
		setvbuf(fp, NULL, _IOFBF, BUFSIZ);
	return fp;
}


/* desgetc: return next char in a file */
desgetc(fp)
	FILE *fp;
{
#ifdef DES
	static char buf[8];
	static int n = 0;

	if (n >= sg_ct) {
		n = 0;
		sg_ct = cbcdec(buf, fp);
	}
	return (sg_ct > 0) ? buf[n++] : EOF;
#endif
}

/* desputc: write a char to a file */
desputc(c, fp)
	char c;
	FILE *fp;
{
#ifdef DES
	static char buf[8];
	static int n = 0;

	if (n == sizeof buf) {
		sp_ct = cbcenc(buf, n, fp);
		n = 0;
	}
	if (c == EOF) {
		sp_ct = cbcenc(buf, n, fp);
		n = 0;
	}
	return (sp_ct >= 0 && c != EOF) ? (buf[n++] = c) : EOF;
#endif
}


/* getline: read a line of text up a maximum size from stdin; return
   line length */
getline(buf, n)
	char *buf;
	int n;
{
	static char oc = '\0';		/* overflow character */

	register int i = 0;
	register int oi = 0;
	char c;

	/* Read one character at a time to avoid i/o contention with shell
	   escapes invoked by nonterminal input, e.g.,
	   ed - <<EOF
	   !cat
	   hello, world
	   EOF */
	if (n < 2)
		return 0;
	if (oc != '\0') {
		buf[i++] = oc;
		oc = '\0';
	}
	for (;;)
		switch (read(0, &c, 1)) {
		default:
			oi = 0;
			if (i < n - 1) {
				if ((buf[i++] = c) != '\n')
					continue;
				lineno++;		/* script line no. */
				buf[i] = '\0';
				return i;
			}
			oc = c;
			buf[i] = '\0';
			return i;
		case 0:
			if (i != oi) {
				oi = i;
				continue;
			}
			/* fall through */
		case -1:
			if (i) buf[i] = '\0';
			return i;
		}
}


char *gbuf = NULL;

#define strip_trailing_esc \
	*(gbuf + l - 2) = '\n', *(gbuf + l - 1) = '\0', l--


/* getcmdv: get a command-list from the command buffer */
char *
getcmdv()
{
	int l, n, m = (sizeof ibuf);
	char *t;

	if ((gbuf = (char *) malloc(m)) == NULL) {
		sprintf(errmsg, "out of memory");
		return NULL;
	}
	strcpy(gbuf, ibufp);
	if ((l = strlen(gbuf)) < 2 || !oddesc(gbuf, gbuf + l - 1))
		return gbuf;
	strip_trailing_esc;
	for (;;) {
		t = gbuf;
		if ((n = getline(ibuf, m)) == 0  || ibuf[n - 1] != '\n'
		 || (l += n) >= m
		 && (t = realloc(gbuf, m += sizeof ibuf)) == NULL) {
			sprintf(errmsg, "out of memory");
			freecmdv();
			return NULL;
		}
		gbuf = t;
		strcat(gbuf, ibuf);
		if (n < 2 || !oddesc(gbuf, gbuf + l - 1))
			break;
		strip_trailing_esc;
	}
	return gbuf;
}


/* lpdup: duplicate a line node */
line_t *
lpdup(lp)
	line_t *lp;
{
	line_t *np;

	if ((np = (line_t *) malloc(sizeof(line_t))) == NULL) {
		sprintf(errmsg, "out of memory");
		return NULL;
	}
	np->seek = lp->seek;
	np->len = (lp->len & ~ACTV);	/* zero ACTV bit */
	return np;
}


/* oddesc:  return the parity of escapes preceding a character in a
   string */
oddesc(s, t)
	char *s;
	char *t;
{
    return (s == t || *(t - 1) != '\\') ? 0 : !oddesc(s, t - 1);
}


/* freecmdv: free memory used by a command list */
void
freecmdv()
{
	free(gbuf);
	gbuf = NULL;
}


/* getrange: return the number of nodes in a list */
long
getrange(h, t)
	line_t *h;
	line_t *t;
{
	long n = 1;

	for (; h != t; h = h->next)
		n++;
	return n;
}


void
onhup(signo)
	int signo;
{
	if (mutex)
		sigflags |= (1 << signo);
	else	dohup(signo);
}


void
onintr(signo)
	int signo;
{
	if (mutex)
		sigflags |= (1 << signo);
	else	dointr(signo);
}


char hup[MAXFNAME] = "ed.hup";	/* hup file name */

void
dohup(signo)
	int signo;
{
	char *s;
	int n;

	sigflags &= ~(1 << signo);
	if (lastln && dowrite(1, lastln, hup, "w") < 0
	 && (s = getenv("HOME")) != NULL
	 && (n = strlen(s)) + 8 < sizeof hup) {	/* "ed.hup" + '/' */
		strcpy(hup, s);
		if (hup[n - 1] != '/')
			hup[n] = '/', hup[n+1] = '\0';
		strcat(hup, "ed.hup");
		dowrite(1, lastln, hup, "w");
	}
	quit(2);
}


void
dointr(signo)
	int signo;
{
	sigflags &= ~(1 << signo);
	fputs("\n?\n", stderr);
	sprintf(errmsg, "interrupt");
	longjmp(env, -1);
}


void
dowinch(signo)
	int signo;
{
	sigflags &= ~(1 << signo);
	if (ioctl(0, TIOCGWINSZ, (char *) &ws) >= 0) {
		if (ws.ws_row > 2) rows = ws.ws_row - 2;
		if (ws.ws_col > 8) cols = ws.ws_col - 8;
	}
}


/* esctos: return pointer to copy of string stripped of  escape characters
   - ignore trailing escape */
char *
esctos(s)
	char *s;
{
	static char file[MAXFNAME];
	char *t = file;

	while (*t++ = (*s == '\\') ? *++s : *s)
		s++;
	return file;
}


