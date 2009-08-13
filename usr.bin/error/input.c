/*	$NetBSD: input.c,v 1.15 2009/08/13 05:53:58 dholland Exp $	*/

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
#if 0
static char sccsid[] = "@(#)input.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: input.c,v 1.15 2009/08/13 05:53:58 dholland Exp $");
#endif /* not lint */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

int cur_wordc;		/* how long the current error message is */
char **cur_wordv;	/* the actual error message */

static Errorclass catchall(void);
static Errorclass cpp(void);
static Errorclass f77(void);
static Errorclass lint0(void);
static Errorclass lint1(void);
static Errorclass lint2(void);
static Errorclass lint3(void);
static Errorclass make(void);
static Errorclass mod2(void);
static Errorclass onelong(void);
static Errorclass pccccom(void);	/* Portable C Compiler C Compiler */
static Errorclass ri(void);
static Errorclass richieccom(void);	/* Richie Compiler for 11 */
static Errorclass troff(void);

/*
 * Eat all of the lines in the input file, attempting to categorize
 * them by their various flavors
 */
void
eaterrors(int *r_errorc, Eptr **r_errorv)
{
	Errorclass errorclass = C_SYNC;
	char *line;
	const char *inbuffer;
	size_t inbuflen;

    for (;;) {
	if ((inbuffer = fgetln(errorfile, &inbuflen)) == NULL)
		break;
	line = Calloc(inbuflen + 1, sizeof(char));
	memcpy(line, inbuffer, inbuflen);
	line[inbuflen] = '\0';
	wordvbuild(line, &cur_wordc, &cur_wordv);

	/*
	 * for convenience, convert cur_wordv to be 1 based, instead
	 * of 0 based.
	 */
	cur_wordv -= 1;
	if (cur_wordc > 0 &&
	   ((( errorclass = onelong() ) != C_UNKNOWN)
	   || (( errorclass = cpp() ) != C_UNKNOWN)
	   || (( errorclass = pccccom() ) != C_UNKNOWN)
	   || (( errorclass = richieccom() ) != C_UNKNOWN)
	   || (( errorclass = lint0() ) != C_UNKNOWN)
	   || (( errorclass = lint1() ) != C_UNKNOWN)
	   || (( errorclass = lint2() ) != C_UNKNOWN)
	   || (( errorclass = lint3() ) != C_UNKNOWN)
	   || (( errorclass = make() ) != C_UNKNOWN)
	   || (( errorclass = f77() ) != C_UNKNOWN)
	   || ((errorclass = pi() ) != C_UNKNOWN)
	   || (( errorclass = ri() )!= C_UNKNOWN)
	   || (( errorclass = mod2() )!= C_UNKNOWN)
	   || (( errorclass = troff() )!= C_UNKNOWN))
	) ;
	else
		errorclass = catchall();
	if (cur_wordc)
		erroradd(cur_wordc, cur_wordv+1, errorclass, C_UNKNOWN);
    }
#ifdef FULLDEBUG
    printf("%d errorentrys\n", nerrors);
#endif
    arrayify(r_errorc, r_errorv, er_head);
}

/*
 * create a new error entry, given a zero based array and count
 */
void
erroradd(int errorlength, char **errorv, Errorclass errorclass,
	 Errorclass errorsubclass)
{
	Eptr newerror;
	const char *cp;

	if (errorclass == C_TRUE) {
		/* check canonicalization of the second argument*/
		for (cp = errorv[1]; *cp && isdigit((unsigned char)*cp); cp++)
			continue;
		errorclass = (*cp == '\0') ? C_TRUE : C_NONSPEC;
#ifdef FULLDEBUG
		if (errorclass != C_TRUE)
			printf("The 2nd word, \"%s\" is not a number.\n",
				errorv[1]);
#endif
	}
	if (errorlength > 0) {
		newerror = Calloc(1, sizeof(Edesc));
		newerror->error_language = language; /* language is global */
		newerror->error_text = errorv;
		newerror->error_lgtext = errorlength;
		if (errorclass == C_TRUE)
			newerror->error_line = atoi(errorv[1]);
		newerror->error_e_class = errorclass;
		newerror->error_s_class = errorsubclass;
		switch (newerror->error_e_class = discardit(newerror)) {
			case C_SYNC:		nsyncerrors++; break;
			case C_DISCARD: 	ndiscard++; break;
			case C_NULLED:		nnulled++; break;
			case C_NONSPEC:		nnonspec++; break;
			case C_THISFILE: 	nthisfile++; break;
			case C_TRUE:		ntrue++; break;
			case C_UNKNOWN:		nunknown++; break;
			case C_IGNORE:		nignore++; break;
		}
		newerror->error_next = er_head;
		er_head = newerror;
		newerror->error_no = nerrors++;
	}	/* length > 0 */
}

static Errorclass
onelong(void)
{
	char **nwordv;

	if (cur_wordc == 1 && language != INLD) {
		/*
		 * We have either:
		 *	a) file name from cc
		 *	b) Assembler telling world that it is complaining
		 *	c) Noise from make ("Stop.")
		 *	c) Random noise
		 */
		cur_wordc = 0;
		if (strcmp(cur_wordv[1], "Stop.") == 0) {
			language = INMAKE;
			return (C_SYNC);
		}
		if (strcmp(cur_wordv[1], "Assembler:") == 0) {
			/* assembler always alerts us to what happened*/
			language = INAS;
			return (C_SYNC);
		} else
		if (strcmp(cur_wordv[1], "Undefined:") == 0) {
			/* loader complains about unknown symbols*/
			language = INLD;
			return (C_SYNC);
		}
		if (lastchar(cur_wordv[1]) == ':') {
			/* cc tells us what file we are in */
			currentfilename = cur_wordv[1];
			(void)substitute(currentfilename, ':', '\0');
			language = INCC;
			return (C_SYNC);
		}
	} else
	if (cur_wordc == 1 && language == INLD) {
		nwordv = Calloc(4, sizeof(char *));
		nwordv[0] = "ld:";
		nwordv[1] = cur_wordv[1];
		nwordv[2] = "is";
		nwordv[3] = "undefined.";
		cur_wordc = 4;
		cur_wordv = nwordv - 1;
		return (C_NONSPEC);
	} else
	if (cur_wordc == 1) {
		return (C_SYNC);
	}
	return (C_UNKNOWN);
}	/* end of one long */

static Errorclass
cpp(void)
{
	/*
	 * Now attempt a cpp error message match
	 * Examples:
	 *	./morse.h: 23: undefined control
	 *	morsesend.c: 229: MAGNIBBL: argument mismatch
	 *	morsesend.c: 237: MAGNIBBL: argument mismatch
	 *	test1.c: 6: undefined control
	 */
	if (cur_wordc < 3)
		return (C_UNKNOWN);
	if (language != INLD		/* loader errors have almost same fmt */
	    && lastchar(cur_wordv[1]) == ':'
	    && isdigit((unsigned char)firstchar(cur_wordv[2]))
	    && lastchar(cur_wordv[2]) == ':') {
		language = INCPP;
		clob_last(cur_wordv[1], '\0');
		clob_last(cur_wordv[2], '\0');
		return (C_TRUE);
	}
	return (C_UNKNOWN);
}	/*end of cpp*/

static Errorclass
pccccom(void)
{
	/*
	 * Now attempt a ccom error message match:
	 * Examples:
	 *	"morsesend.c", line 237: operands of & have incompatible types
	 *	"test.c", line 7: warning: old-fashioned initialization: use =
	 *	"subdir.d/foo2.h", line 1: illegal initialization
	 */
	if (cur_wordc < 4)
		return (C_UNKNOWN);
	if (firstchar(cur_wordv[1]) == '"'
	    && lastchar(cur_wordv[1]) == ','
	    && next_lastchar(cur_wordv[1]) == '"'
	    && strcmp(cur_wordv[2], "line") == 0
	    && isdigit((unsigned char)firstchar(cur_wordv[3]))
	    && lastchar(cur_wordv[3]) == ':') {
		clob_last(cur_wordv[1], '\0');	/* drop last , */
		clob_last(cur_wordv[1], '\0');	/* drop last " */
		cur_wordv[1]++;			/* drop first " */
		clob_last(cur_wordv[3], '\0');	/* drop : on line number */
		cur_wordv[2] = cur_wordv[1];	/* overwrite "line" */
		cur_wordv++;		/*compensate*/
		cur_wordc--;
		currentfilename = cur_wordv[1];
		language = INCC;
		return (C_TRUE);
	}
	return (C_UNKNOWN);
}	/* end of ccom */

/*
 * Do the error message from the Richie C Compiler for the PDP11,
 * which has this source:
 *
 *	if (filename[0])
 *		fprintf(stderr, "%s:", filename);
 *	fprintf(stderr, "%d: ", line);
 *
 */

static Errorclass
richieccom(void)
{
	char *cp;
	char **nwordv;
	char *file;

	if (cur_wordc < 2)
		return (C_UNKNOWN);

	if (lastchar(cur_wordv[1]) == ':') {
		cp = cur_wordv[1] + strlen(cur_wordv[1]) - 1;
		while (isdigit((unsigned char)*--cp))
			continue;
		if (*cp == ':') {
			clob_last(cur_wordv[1], '\0');	/* last : */
			*cp = '\0';			/* first : */
			file = cur_wordv[1];
			nwordv = wordvsplice(1, cur_wordc, cur_wordv+1);
			nwordv[0] = file;
			nwordv[1] = cp + 1;
			cur_wordc += 1;
			cur_wordv = nwordv - 1;
			language = INCC;
			currentfilename = cur_wordv[1];
			return (C_TRUE);
		}
	}
	return (C_UNKNOWN);
}

static Errorclass
lint0(void)
{
	char **nwordv;
	char *line, *file;

	/*
	 * Attempt a match for the new lint style normal compiler
	 * error messages, of the form
	 *
	 *	printf("%s(%d): %s\n", filename, linenumber, message);
	 */
	if (cur_wordc < 2)
		return (C_UNKNOWN);

	if (lastchar(cur_wordv[1]) == ':'
	    && next_lastchar(cur_wordv[1]) == ')') {
		clob_last(cur_wordv[1], '\0'); /* colon */
		if (persperdexplode(cur_wordv[1], &line, &file)) {
			nwordv = wordvsplice(1, cur_wordc, cur_wordv+1);
			nwordv[0] = file;	/* file name */
			nwordv[1] = line;	/* line number */
			cur_wordc += 1;
			cur_wordv = nwordv - 1;
			language = INLINT;
			return (C_TRUE);
		}
		cur_wordv[1][strlen(cur_wordv[1])] = ':';
	}
	return (C_UNKNOWN);
}

static Errorclass
lint1(void)
{
	char *line1 = NULL, *line2 = NULL;
	char *file1 = NULL, *file2 = NULL;
	char **nwordv1, **nwordv2;

	/*
	 * Now, attempt a match for the various errors that lint
	 * can complain about.
	 *
	 * Look first for type 1 lint errors
	 */
	if (cur_wordc > 1 && strcmp(cur_wordv[cur_wordc-1], "::") == 0) {
	 /*
  	  * %.7s, arg. %d used inconsistently %s(%d) :: %s(%d)
  	  * %.7s value used inconsistently %s(%d) :: %s(%d)
  	  * %.7s multiply declared %s(%d) :: %s(%d)
  	  * %.7s value declared inconsistently %s(%d) :: %s(%d)
  	  * %.7s function value type must be declared before use %s(%d) :: %s(%d)
	  */
		language = INLINT;
		if (cur_wordc > 2
		    && persperdexplode(cur_wordv[cur_wordc], &line2, &file2)
		    && persperdexplode(cur_wordv[cur_wordc-2], &line1, &file1)) {
			nwordv1 = wordvsplice(2, cur_wordc, cur_wordv+1);
			nwordv2 = wordvsplice(2, cur_wordc, cur_wordv+1);
			nwordv1[0] = file1;
			nwordv1[1] = line1;
			erroradd(cur_wordc+2, nwordv1, C_TRUE, C_DUPL); /* takes 0 based*/
			nwordv2[0] = file2;
			nwordv2[1] = line2;
			cur_wordc = cur_wordc + 2;
			cur_wordv = nwordv2 - 1;	/* 1 based */
			return (C_TRUE);
		}
	}
	if (file2)
		free(file2);
	if (file1)
		free(file1);
	if (line2)
		free(line2);
	if (line1)
		free(line1);
	return (C_UNKNOWN);
} /* end of lint 1*/

static Errorclass
lint2(void)
{
	char *file;
	char *line;
	char **nwordv;

	/*
	 * Look for type 2 lint errors
	 *
	 *	%.7s used( %s(%d) ), but not defined
	 *	%.7s defined( %s(%d) ), but never used
	 *	%.7s declared( %s(%d) ), but never used or defined
	 *
	 *	bufp defined( "./metric.h"(10) ), but never used
	 */
	if (cur_wordc < 5)
		return (C_UNKNOWN);

	if (lastchar(cur_wordv[2]) == '(' /* ')' */
	    && strcmp(cur_wordv[4], "),") == 0) {
		language = INLINT;
		if (persperdexplode(cur_wordv[3], &line, &file)) {
			nwordv = wordvsplice(2, cur_wordc, cur_wordv+1);
			nwordv[0] = file;
			nwordv[1] = line;
			cur_wordc = cur_wordc + 2;
			cur_wordv = nwordv - 1;	/* 1 based */
			return (C_TRUE);
		}
	}
	return (C_UNKNOWN);
} /* end of lint 2*/

static char *Lint31[4] = {"returns", "value", "which", "is"};
static char *Lint32[6] = {"value", "is", "used,", "but", "none", "returned"};

static Errorclass
lint3(void)
{
	if (cur_wordc < 3)
		return (C_UNKNOWN);
	if (wordvcmp(cur_wordv+2, 4, Lint31) == 0
	    || wordvcmp(cur_wordv+2, 6, Lint32) == 0) {
		language = INLINT;
		return (C_NONSPEC);
	}
	return (C_UNKNOWN);
}

/*
 * Special word vectors for use by F77 recognition
 */
static char *F77_fatal[3] = {"Compiler", "error", "line"};
static char *F77_error[3] = {"Error", "on", "line"};
static char *F77_warning[3] = {"Warning", "on", "line"};
static char *F77_no_ass[3] = {"Error.","No","assembly."};

static Errorclass
f77(void)
{
	char **nwordv;

	/*
	 * look for f77 errors:
	 * Error messages from /usr/src/cmd/f77/error.c, with
	 * these printf formats:
	 *
	 *	Compiler error line %d of %s: %s
	 *	Error on line %d of %s: %s
	 *	Warning on line %d of %s: %s
	 *	Error.  No assembly.
	 */
	if (cur_wordc == 3 && wordvcmp(cur_wordv+1, 3, F77_no_ass) == 0) {
		cur_wordc = 0;
		return (C_SYNC);
	}
	if (cur_wordc < 6)
		return (C_UNKNOWN);
	if (lastchar(cur_wordv[6]) == ':'
	    && (
		wordvcmp(cur_wordv+1, 3, F77_fatal) == 0
		|| wordvcmp(cur_wordv+1, 3, F77_error) == 0
		|| wordvcmp(cur_wordv+1, 3, F77_warning) == 0
	       )
	) {
		language = INF77;
		nwordv = wordvsplice(2, cur_wordc, cur_wordv+1);
		nwordv[0] = cur_wordv[6];
		clob_last(nwordv[0],'\0');
		nwordv[1] = cur_wordv[4];
		cur_wordc += 2;
		cur_wordv = nwordv - 1;	/* 1 based */
		return (C_TRUE);
	}
	return (C_UNKNOWN);
} /* end of f77 */

static char *Make_Croak[3] = {"***", "Error", "code"};
static char *Make_NotRemade[5] = {"not", "remade", "because", "of", "errors"};

static Errorclass
make(void)
{
	if (wordvcmp(cur_wordv+1, 3, Make_Croak) == 0) {
		language = INMAKE;
		return (C_SYNC);
	}
	if (wordvcmp(cur_wordv+2, 5, Make_NotRemade) == 0) {
		language = INMAKE;
		return (C_SYNC);
	}
	return (C_UNKNOWN);
}

static Errorclass
ri(void)
{
/*
 * Match an error message produced by ri; here is the
 * procedure yanked from the distributed version of ri
 * April 24, 1980.
 *
 *	serror(str, x1, x2, x3)
 *		char str[];
 *		char *x1, *x2, *x3;
 *	{
 *		extern int yylineno;
 *
 *		putc('"', stdout);
 *		fputs(srcfile, stdout);
 *		putc('"', stdout);
 *		fprintf(stdout, " %d: ", yylineno);
 *		fprintf(stdout, str, x1, x2, x3);
 *		fprintf(stdout, "\n");
 *		synerrs++;
 *	}
 */
	if (cur_wordc < 3)
		return (C_UNKNOWN);
	if (firstchar(cur_wordv[1]) == '"'
	    && lastchar(cur_wordv[1]) == '"'
	    && lastchar(cur_wordv[2]) == ':'
	    && isdigit((unsigned char)firstchar(cur_wordv[2]))) {
		clob_last(cur_wordv[1], '\0');	/* drop the last " */
		cur_wordv[1]++;			/* skip over the first " */
		clob_last(cur_wordv[2], '\0');
		language = INRI;
		return (C_TRUE);
	}
	return (C_UNKNOWN);
}

static Errorclass
catchall(void)
{
	/*
	 * Catches random things.
	 */
	language = INUNKNOWN;
	return (C_NONSPEC);
} /* end of catch all*/

static Errorclass
troff(void)
{
	/*
	 * troff source error message, from eqn, bib, tbl...
	 * Just like pcc ccom, except uses `'
	 */
	if (cur_wordc < 4)
		return (C_UNKNOWN);

	if (firstchar(cur_wordv[1]) == '`'
	    && lastchar(cur_wordv[1]) == ','
	    && next_lastchar(cur_wordv[1]) == '\''
	    && strcmp(cur_wordv[2], "line") == 0
	    && isdigit((unsigned char)firstchar(cur_wordv[3]))
	    && lastchar(cur_wordv[3]) == ':') {
		clob_last(cur_wordv[1], '\0');	/* drop last , */
		clob_last(cur_wordv[1], '\0');	/* drop last " */
		cur_wordv[1]++;			/* drop first " */
		clob_last(cur_wordv[3], '\0');	/* drop : on line number */
		cur_wordv[2] = cur_wordv[1];	/* overwrite "line" */
		cur_wordv++;			/*compensate*/
		currentfilename = cur_wordv[1];
		language = INTROFF;
		return (C_TRUE);
	}
	return (C_UNKNOWN);
}

static Errorclass
mod2(void)
{
	/*
	 * for decwrl modula2 compiler (powell)
	 */
	if (cur_wordc < 5)
		return (C_UNKNOWN);
	if ((strcmp(cur_wordv[1], "!!!") == 0		/* early version */
	     || strcmp(cur_wordv[1], "File") == 0)	/* later version */
	    && lastchar(cur_wordv[2]) == ','		/* file name */
	    && strcmp(cur_wordv[3], "line") == 0
	    && isdigit((unsigned char)firstchar(cur_wordv[4]))	/* line number */
	    && lastchar(cur_wordv[4]) == ':'	/* line number */
	) {
		clob_last(cur_wordv[2], '\0');	/* drop last , on file name */
		clob_last(cur_wordv[4], '\0');	/* drop last : on line number */
		cur_wordv[3] = cur_wordv[2];	/* file name on top of "line" */
		cur_wordv += 2;
		cur_wordc -= 2;
		currentfilename = cur_wordv[1];
		language = INMOD2;
		return (C_TRUE);
	}
	return (C_UNKNOWN);
}
