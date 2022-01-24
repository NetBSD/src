/*	$NetBSD: vfontedpr.c,v 1.19 2022/01/24 09:14:37 andvar Exp $	*/

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)vfontedpr.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: vfontedpr.c,v 1.19 2022/01/24 09:14:37 andvar Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "pathnames.h"
#include "extern.h"

#define STANDARD 0
#define ALTERNATE 1

/*
 * Vfontedpr.
 *
 * Dave Presotto 1/12/81 (adapted from an earlier version by Bill Joy)
 *
 */

#define STRLEN 10		/* length of strings introducing things */
#define PNAMELEN 40		/* length of a function/procedure name */
#define PSMAX 20		/* size of procedure name stacking */

static int       iskw(char *);
static bool      isproc(char *);
static void      putKcp(char *, char *, bool);
static void      putScp(char *);
static void      putcp(int);
static int       tabs(char *, char *);
static int       width(char *, char *);

/*
 *	The state variables
 */

static bool	filter = false;	/* act as a filter (like eqn) */
static bool	inchr;		/* in a string constant */
static bool	incomm;		/* in a comment of the primary type */
static bool	idx = false;	/* form an index */
static bool	instr;		/* in a string constant */
static bool	nokeyw = false;	/* no keywords being flagged */
static bool	pass = false;	/*
				 * when acting as a filter, pass indicates
				 * whether we are currently processing
				 * input.
				 */

static int	blklevel;	/* current nesting level */
static int	comtype;	/* type of comment */
static const char *defsfile[2] = { _PATH_VGRINDEFS, 0 };
				/* name of language definitions file */
static int	margin;
static int	plstack[PSMAX];	/* the procedure nesting level stack */
static char	pname[BUFSIZ+1];
static bool  prccont;	/* continue last procedure */
static int	psptr;		/* the stack index of the current procedure */
static char	pstack[PSMAX][PNAMELEN+1];	/* the procedure name stack */

/*
 *	The language specific globals
 */

char	*l_acmbeg;		/* string introducing a comment */
char	*l_acmend;		/* string ending a comment */
char	*l_blkbeg;		/* string beginning of a block */
char	*l_blkend;		/* string ending a block */
char    *l_chrbeg;		/* delimiter for character constant */
char    *l_chrend;		/* delimiter for character constant */
char	*l_combeg;		/* string introducing a comment */
char	*l_comend;		/* string ending a comment */
char	 l_escape;		/* character used to  escape characters */
char	*l_keywds[BUFSIZ/2];	/* keyword table address */
char	*l_prcbeg;		/* regular expr for procedure begin */
char    *l_strbeg;		/* delimiter for string constant */
char    *l_strend;		/* delimiter for string constant */
bool	 l_toplex;		/* procedures only defined at top lex level */
const char *language = "c";	/* the language indicator */

#define	ps(x)	printf("%s", x)
static char minus[] = "-";
static char minusn[] = "-n";

int
main(int argc, char *argv[])
{
    const char *fname = "";
    struct stat stbuf;
    char buf[BUFSIZ];
    char *defs;
    int needbp = 0;

    argc--, argv++;
    do {
	char *cp;
	int i;

	if (argc > 0) {
	    if (!strcmp(argv[0], "-h")) {
		if (argc == 1) {
		    printf("'ds =H\n");
		    argc = 0;
		    goto rest;
		}
		printf("'ds =H %s\n", argv[1]);
		argc--, argv++;
		argc--, argv++;
		if (argc > 0)
		    continue;
		goto rest;
	    }

	    /* act as a filter like eqn */
	    if (!strcmp(argv[0], "-f")) {
		filter=true;
		argv[0] = argv[argc-1];
		argv[argc-1] = minus;
		continue;
	    }

	    /* take input from the standard place */
	    if (!strcmp(argv[0], "-")) {
		argc = 0;
		goto rest;
	    }

	    /* build an index */
	    if (!strcmp(argv[0], "-x")) {
		idx=true;
		argv[0] = minusn;
	    }

	    /* indicate no keywords */
	    if (!strcmp(argv[0], "-n")) {
		nokeyw=true;
		argc--, argv++;
		continue;
	    }

	    /* specify the font size */
	    if (!strncmp(argv[0], "-s", 2)) {
		i = 0;
		cp = argv[0] + 2;
		while (*cp)
		    i = i * 10 + (*cp++ - '0');
		printf("'ps %d\n'vs %d\n", i, i+1);
		argc--, argv++;
		continue;
	    }

	    /* specify the language */
	    if (!strncmp(argv[0], "-l", 2)) {
		language = argv[0]+2;
		argc--, argv++;
		continue;
	    }

	    /* specify the language description file */
	    if (!strncmp(argv[0], "-d", 2)) {
		defsfile[0] = argv[1];
		argc--, argv++;
		argc--, argv++;
		continue;
	    }

	    /* open the file for input */
	    if (freopen(argv[0], "r", stdin) == NULL) {
		perror(argv[0]);
		exit(1);
	    }
	    if (idx)
		printf("'ta 4i 4.25i 5.5iR\n'in .5i\n");
	    fname = argv[0];
	    argc--, argv++;
	}
    rest:

	/*
	 *  get the  language definition from the defs file
	 */
	i = cgetent(&defs, defsfile, language);
	if (i == -1) {
	    fprintf (stderr, "no entry for language %s\n", language);
	    exit(0);
	} else  if (i == -2) { fprintf(stderr,
	    "cannot find vgrindefs file %s\n", defsfile[0]);
	    exit(0);
	} else if (i == -3) { fprintf(stderr,
	    "potential reference loop detected in vgrindefs file %s\n",
            defsfile[0]);
	    exit(0);
	}
	if (cgetustr(defs, "kw", &cp) == -1)
	    nokeyw = true;
	else  {
	    char **cpp;

	    cpp = l_keywds;
	    while (*cp) {
		while (*cp == ' ' || *cp =='\t')
		    *cp++ = '\0';
		if (*cp)
		    *cpp++ = cp;
		while (*cp != ' ' && *cp  != '\t' && *cp)
		    cp++;
	    }
	    *cpp = NULL;
	}
	cgetustr(defs, "pb", &cp);
	l_prcbeg = convexp(cp);
	cgetustr(defs, "cb", &cp);
	l_combeg = convexp(cp);
	cgetustr(defs, "ce", &cp);
	l_comend = convexp(cp);
	cgetustr(defs, "ab", &cp);
	l_acmbeg = convexp(cp);
	cgetustr(defs, "ae", &cp);
	l_acmend = convexp(cp);
	cgetustr(defs, "sb", &cp);
	l_strbeg = convexp(cp);
	cgetustr(defs, "se", &cp);
	l_strend = convexp(cp);
	cgetustr(defs, "bb", &cp);
	l_blkbeg = convexp(cp);
	cgetustr(defs, "be", &cp);
	l_blkend = convexp(cp);
	cgetustr(defs, "lb", &cp);
	l_chrbeg = convexp(cp);
	cgetustr(defs, "le", &cp);
	l_chrend = convexp(cp);
	l_escape = '\\';
	l_onecase = (cgetcap(defs, "oc", ':') != NULL);
	l_toplex = (cgetcap(defs, "tl", ':') != NULL);

	/* initialize the program */

	incomm = false;
	instr = false;
	inchr = false;
	x_escaped = false;
	blklevel = 0;
	for (psptr=0; psptr<PSMAX; psptr++) {
	    pstack[psptr][0] = '\0';
	    plstack[psptr] = 0;
	}
	psptr = -1;
	ps("'-F\n");
	if (!filter) {
	    printf(".ds =F %s\n", fname);
	    ps("'wh 0 vH\n");
	    ps("'wh -1i vF\n");
	}
	if (needbp) {
	    needbp = 0;
	    printf(".()\n");
	    printf(".bp\n");
	}
	if (!filter) {
	    fstat(fileno(stdin), &stbuf);
	    cp = ctime(&stbuf.st_mtime);
	    cp[16] = '\0';
	    cp[24] = '\0';
	    printf(".ds =M %s %s\n", cp+4, cp+20);
	}

	/*
	 *	MAIN LOOP!!!
	 */
	while (fgets(buf, sizeof buf, stdin) != NULL) {
	    if (buf[0] == '\f') {
		printf(".bp\n");
	    }
	    if (buf[0] == '.') {
		printf("%s", buf);
		if (!strncmp (buf+1, "vS", 2))
		    pass = true;
		if (!strncmp (buf+1, "vE", 2))
		    pass = false;
		continue;
	    }
	    prccont = false;
	    if (!filter || pass)
		putScp(buf);
	    else
		printf("%s", buf);
	    if (prccont && (psptr >= 0)) {
		ps("'FC ");
		ps(pstack[psptr]);
		ps("\n");
	    }
#ifdef DEBUG
	    printf ("com %o str %o chr %o ptr %d\n", incomm, instr, inchr, psptr);
#endif
	    margin = 0;
	}
	needbp = 1;
    } while (argc > 0);
    exit(0);
}

#define isidchr(c) (isalnum((unsigned char)(c)) || (c) == '_')

static void
putScp(char *os)
{
    char *s = os;			/* pointer to unmatched string */
    char dummy[BUFSIZ];			/* dummy to be used by expmatch */
    char *comptr;			/* end of a comment delimiter */
    char *acmptr;			/* end of a comment delimiter */
    char *strptr;			/* end of a string delimiter */
    char *chrptr;			/* end of a character const delimiter */
    char *blksptr;			/* end of a lexical block start */
    char *blkeptr;			/* end of a lexical block end */

    x_start = os;			/* remember the start for expmatch */
    x_escaped = false;
    if (nokeyw || incomm || instr)
	goto skip;
    if (isproc(s)) {
	ps("'FN ");
	ps(pname);
        ps("\n");
	if (psptr < PSMAX) {
	    ++psptr;
	    strlcpy(pstack[psptr], pname, sizeof(pstack[psptr]));
	    plstack[psptr] = blklevel;
	}
    }
skip:
    do {
	/* check for string, comment, blockstart, etc */
	if (!incomm && !instr && !inchr) {

	    blkeptr = expmatch(s, l_blkend, dummy);
	    blksptr = expmatch(s, l_blkbeg, dummy);
	    comptr = expmatch(s, l_combeg, dummy);
	    acmptr = expmatch(s, l_acmbeg, dummy);
	    strptr = expmatch(s, l_strbeg, dummy);
	    chrptr = expmatch(s, l_chrbeg, dummy);

	    /* start of a comment? */
	    if (comptr != NULL)
		if ((comptr < strptr || strptr == NULL)
		  && (comptr < acmptr || acmptr == NULL)
		  && (comptr < chrptr || chrptr == NULL)
		  && (comptr < blksptr || blksptr == NULL)
		  && (comptr < blkeptr || blkeptr == NULL)) {
		    putKcp(s, comptr-1, false);
		    s = comptr;
		    incomm = true;
		    comtype = STANDARD;
		    if (s != os)
			ps("\\c");
		    ps("\\c\n'+C\n");
		    continue;
		}

	    /* start of a comment? */
	    if (acmptr != NULL)
		if ((acmptr < strptr || strptr == NULL)
		  && (acmptr < chrptr || chrptr == NULL)
		  && (acmptr < blksptr || blksptr == NULL)
		  && (acmptr < blkeptr || blkeptr == NULL)) {
		    putKcp(s, acmptr-1, false);
		    s = acmptr;
		    incomm = true;
		    comtype = ALTERNATE;
		    if (s != os)
			ps("\\c");
		    ps("\\c\n'+C\n");
		    continue;
		}

	    /* start of a string? */
	    if (strptr != NULL)
		if ((strptr < chrptr || chrptr == NULL)
		  && (strptr < blksptr || blksptr == NULL)
		  && (strptr < blkeptr || blkeptr == NULL)) {
		    putKcp(s, strptr-1, false);
		    s = strptr;
		    instr = true;
		    continue;
		}

	    /* start of a character string? */
	    if (chrptr != NULL)
		if ((chrptr < blksptr || blksptr == NULL)
		  && (chrptr < blkeptr || blkeptr == NULL)) {
		    putKcp(s, chrptr-1, false);
		    s = chrptr;
		    inchr = true;
		    continue;
		}

	    /* end of a lexical block */
	    if (blkeptr != NULL) {
		if (blkeptr < blksptr || blksptr == NULL) {
		    putKcp(s, blkeptr - 1, false);
		    s = blkeptr;
		    blklevel--;
		    if (psptr >= 0 && plstack[psptr] >= blklevel) {

			/* end of current procedure */
			if (s != os)
			    ps("\\c");
			ps("\\c\n'-F\n");
			blklevel = plstack[psptr];

			/* see if we should print the last proc name */
			if (--psptr >= 0)
			    prccont = true;
			else
			    psptr = -1;
		    }
		    continue;
		}
	    }

	    /* start of a lexical block */
	    if (blksptr != NULL) {
		putKcp(s, blksptr - 1, false);
		s = blksptr;
		blklevel++;
		continue;
	    }

	/* check for end of comment */
	} else if (incomm) {
	    comptr = expmatch(s, l_comend, dummy);
	    acmptr = expmatch(s, l_acmend, dummy);
	    if (((comtype == STANDARD) && (comptr != NULL)) ||
	        ((comtype == ALTERNATE) && (acmptr != NULL))) {
		if (comtype == STANDARD) {
		    putKcp(s, comptr-1, true);
		    s = comptr;
		} else {
		    putKcp(s, acmptr-1, true);
		    s = acmptr;
		}
		incomm = false;
		ps("\\c\n'-C\n");
		continue;
	    } else {
		putKcp(s, s + strlen(s) -1, true);
		s = s + strlen(s);
		continue;
	    }

	/* check for end of string */
	} else if (instr) {
	    if ((strptr = expmatch(s, l_strend, dummy)) != NULL) {
		putKcp(s, strptr-1, true);
		s = strptr;
		instr = false;
		continue;
	    } else {
		putKcp(s, s+strlen(s)-1, true);
		s = s + strlen(s);
		continue;
	    }

	/* check for end of character string */
	} else if (inchr) {
	    if ((chrptr = expmatch(s, l_chrend, dummy)) != NULL) {
		putKcp(s, chrptr-1, true);
		s = chrptr;
		inchr = false;
		continue;
	    } else {
		putKcp(s, s+strlen(s)-1, true);
		s = s + strlen(s);
		continue;
	    }
	}

	/* print out the line */
	putKcp(s, s + strlen(s) -1, false);
	s = s + strlen(s);
    } while (*s);
}

static void
putKcp(
    char	*start,		/* start of string to write */
    char	*end,		/* end of string to write */
    bool	force)		/* true if we should force nokeyw */
{
    int i;
    int xfld = 0;

    while (start <= end) {
	if (idx) {
	    if (*start == ' ' || *start == '\t') {
		if (xfld == 0)
		    printf("");
		printf("\t");
		xfld = 1;
		while (*start == ' ' || *start == '\t')
		    start++;
		continue;
	    }
	}

	/* take care of nice tab stops */
	if (*start == '\t') {
	    while (*start == '\t')
		start++;
	    i = tabs(x_start, start) - margin / 8;
	    printf("\\h'|%dn'", i * 10 + 1 - margin % 8);
	    continue;
	}

	if (!nokeyw && !force)
	    if ((*start == '#' || isidchr(*start))
	    && (start == x_start || !isidchr(start[-1]))) {
		i = iskw(start);
		if (i > 0) {
		    ps("\\*(+K");
		    do
			putcp(*start++);
		    while (--i > 0);
		    ps("\\*(-K");
		    continue;
		}
	    }

	putcp(*start++);
    }
}


static int
tabs(char *s, char *os)
{

    return width(s, os) / 8;
}

static int
width(char *s, char *os)
{
	int i = 0;

	while (s < os) {
		if (*s == '\t') {
			i = (i + 8) &~ 7;
			s++;
			continue;
		}
		if (*s < ' ')
			i += 2;
		else
			i++;
		s++;
	}
	return i;
}

static void
putcp(int c)
{

	switch(c) {

	case 0:
		break;

	case '\f':
		break;

	case '{':
		ps("\\*(+K{\\*(-K");
		break;

	case '}':
		ps("\\*(+K}\\*(-K");
		break;

	case '\\':
		ps("\\e");
		break;

	case '_':
		ps("\\*_");
		break;

	case '-':
		ps("\\*-");
		break;

	case '`':
		ps("\\`");
		break;

	case '\'':
		ps("\\'");
		break;

	case '.':
		ps("\\&.");
		break;

	case '*':
		ps("\\fI*\\fP");
		break;

	case '/':
		ps("\\fI\\h'\\w' 'u-\\w'/'u'/\\fP");
		break;

	default:
		if (c < 040)
			putchar('^'), c |= '@';
		/* FALLTHROUGH */
	case '\t':
	case '\n':
		putchar(c);
	}
}

/*
 *	look for a process beginning on this line
 */
static bool
isproc(char *s)
{
    pname[0] = '\0';
    if (!l_toplex || blklevel == 0)
	if (expmatch(s, l_prcbeg, pname) != NULL) {
	    return true;
	}
    return false;
}


/*  iskw -	check to see if the next word is a keyword
 */

static int
iskw(char *s)
{
	char **ss = l_keywds;
	int i = 1;
	char *cp = s;

	while (++cp, isidchr((unsigned char)*cp))
		i++;
	while ((cp = *ss++) != NULL)
		if (!STRNCMP(s,cp,i) && !isidchr((unsigned char)cp[i]))
			return i;
	return 0;
}
