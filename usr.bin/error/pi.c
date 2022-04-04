/*	$NetBSD: pi.c,v 1.20 2022/04/04 19:33:46 andvar Exp $	*/

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
static char sccsid[] = "@(#)pi.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: pi.c,v 1.20 2022/04/04 19:33:46 andvar Exp $");
#endif /* not lint */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "error.h"

#if 0 /* not const-correct */
static char *unk_hdr[] = {"In", "program", "???"};
#else
DECL_STRINGS_3(static, unk_hdr, "In", "program", "???");
#endif

static char *c_linenumber;
static char **c_header = &unk_hdr[0];

static boolean alldigits(const char *);
static boolean isdateformat(int, char **);
static boolean instringset(const char *, const char **);
static boolean piptr(const char *);


/*
 * Attempt to handle error messages produced by pi (and by pc)
 *
 *	problem #1:	There is no file name available when a file does not
 *			use a #include; this will have to be given to error
 *			in the command line.
 *	problem #2:	pi doesn't always tell you what line number
 *			a error refers to; for example during the tree
 *			walk phase of code generation and error detection,
 *			an error can refer to "variable foo in procedure bletch"
 *			without giving a line number
 *	problem #3:	line numbers, when available, are attached to
 *			the source line, along with the source line itself
 *			These line numbers must be extracted, and
 *			the source line thrown away.
 *	problem #4:	Some error messages produce more than one line number
 *			on the same message.
 *			There are only two (I think):
 *				%s undefined on line%s
 *				%s improperly used on line%s
 *			here, the %s makes line plural or singular.
 *
 *	Here are the error strings used in pi version 1.2 that can refer
 *	to a file name or line number:
 *
 *		Multiply defined label in case, lines %d and %d
 *		Goto %s from line %d is into a structured statement
 *		End matched %s on line %d
 *		Inserted keyword end matching %s on line %d
 *
 *	Here are the general pi patterns recognized:
 *	define piptr == -.*^-.*
 *	define msg = .*
 *	define digit = [0-9]
 *	definename = .*
 *	define date_format letter*3 letter*3 (digit | (digit digit))
 *			(digit | (digit digit)):digit*2 digit*4
 *
 *	{e,E} (piptr) (msg)	Encounter an error during textual scan
 *	E {digit}* - (msg)	Have an error message that refers to a new line
 *	E - msg			Have an error message that refers to current
 *					function, program or procedure
 *	(date_format) (name):	When switch compilation files
 *	... (msg)		When refer to the previous line
 *	'In' ('procedure'|'function'|'program') (name):
 *				pi is now complaining about 2nd pass errors.
 *
 *	Here is the output from a compilation
 *
 *
 *	     2  	var	i:integer;
 *	e --------------^--- Inserted ';'
 *	E 2 - All variables must be declared in one var part
 *	E 5 - Include filename must end in .i
 *	Mon Apr 21 15:56 1980  test.h:
 *	     2  begin
 *	e ------^--- Inserted ';'
 *	Mon Apr 21 16:06 1980  test.p:
 *	E 2 - Function type must be specified
 *	     6  procedure foo(var x:real);
 *	e ------^--- Inserted ';'
 *	In function bletch:
 *	  E - No assignment to the function variable
 *	  w - variable x is never used
 *	E 6 - foo is already defined in this block
 *	In procedure foo:
 *	  w - variable x is neither used nor set
 *	     9  	z : = 23;
 *	E --------------^--- Undefined variable
 *	    10  	y = [1];
 *	e ----------------^--- Inserted ':'
 *	    13  	z := 345.;
 *	e -----------------------^--- Digits required after decimal point
 *	E 10 - Constant set involved in non set context
 *	E 11 - Type clash: real is incompatible with integer
 *	   ... Type of expression clashed with type of variable in assignment
 *	E 12 - Parameter type not identical to type of var parameter x of foo
 *	In program mung:
 *	  w - variable y is never used
 *	  w - type foo is never used
 *	  w - function bletch is never used
 *	  E - z undefined on lines 9 13
 */
static const char *Months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	0
};
static const char *Days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0
};
static const char *Piroutines[] = {
	"program", "function", "procedure", 0
};


static boolean structured, multiple;

#if 0 /* not const-correct */
static char *pi_Endmatched[] = {"End", "matched"};
static char *pi_Inserted[] = {"Inserted", "keyword", "end", "matching"};

static char *pi_multiple[] = {"Multiply", "defined", "label", "in", "case,", "line"};
static char *pi_structured[] = {"is", "into", "a", "structured", "statement"};

static char *pi_und1[] = {"undefined", "on", "line"};
static char *pi_und2[] = {"undefined", "on", "lines"};
static char *pi_imp1[] = {"improperly", "used", "on", "line"};
static char *pi_imp2[] = {"improperly", "used", "on", "lines"};

#else
DECL_STRINGS_2(static, pi_Endmatched, "End", "matched");
DECL_STRINGS_4(static, pi_Inserted, "Inserted", "keyword", "end", "matching");

DECL_STRINGS_6(static, pi_multiple,
	       "Multiply", "defined", "label", "in", "case,", "line");
DECL_STRINGS_5(static, pi_structured,
	       "is", "into", "a", "structured", "statement");

DECL_STRINGS_3(static, pi_und1, "undefined", "on", "line");
DECL_STRINGS_3(static, pi_und2, "undefined", "on", "lines");
DECL_STRINGS_4(static, pi_imp1, "improperly", "used", "on", "line");
DECL_STRINGS_4(static, pi_imp2, "improperly", "used", "on", "lines");

#endif

static boolean
alldigits(const char *string)
{
	for (; *string && isdigit((unsigned char)*string); string++)
		continue;
	return (*string == '\0');
}

static boolean
instringset(const char *member, const char **set)
{
	for (; *set; set++) {
		if (strcmp(*set, member) == 0)
			return true;
	}
	return false;
}

static boolean
isdateformat(int wordc, char **wordv)
{
	return (
	        (wordc == 5)
	     && (instringset(wordv[0], Days))
	     && (instringset(wordv[1], Months))
	     && (alldigits(wordv[2]))
	     && (alldigits(wordv[4])));
}

static boolean
piptr(const char *string)
{
	if (*string != '-')
		return false;
	while (*string == '-')
		string++;
	if (*string != '^')
		return false;
	string++;
	while (*string == '-')
		string++;
	return (*string == '\0');
}

Errorclass
pi(void)
{
	char **nwordv;

	nwordv = NULL;
	if (cur_wordc < 2)
		return (C_UNKNOWN);
	if (strlen(cur_wordv[1]) == 1
	    && ( cur_wordv[1][0] == 'e' || cur_wordv[1][0] == 'E')
	    && piptr(cur_wordv[2])
	) {
		boolean longpiptr = 0;

		/*
		 *	We have recognized a first pass error of the form:
		 *	letter ------^---- message
		 *
		 *	turn into an error message of the form:
		 *
		 *	file line 'pascal errortype' letter \n |---- message
		 *	or of the form:
		 *	file line letter |---- message
		 *		when there are strlen("(*[pi]") or more
		 *		preceding '-' on the error pointer.
		 *
		 *	Where the | is intended to be a down arrow, so that
		 *	the pi error messages can be inserted above the
		 *	line in error, instead of below.  (All of the other
		 *	languages put their messages before the source line,
		 *	instead of after it as does pi.)
		 *
		 *	where the pointer to the error has been truncated
		 *	by 6 characters to account for the fact that
		 *	the pointer points into a tab preceded input line.
		 */
		language = INPI;
		(void)substitute(cur_wordv[2], '^', '|');
		longpiptr = position(cur_wordv[2],'|') > (6+8);
		nwordv = wordvsplice(longpiptr ? 2 : 4, cur_wordc, cur_wordv+1);
		nwordv[0] = strdup(currentfilename);
		nwordv[1] = strdup(c_linenumber);
		if (!longpiptr) {
			nwordv[2] = Strdup("pascal errortype"); /* XXX leaked */
			nwordv[3] = cur_wordv[1];
			nwordv[4] = strdup("%%%\n");
			if (strlen(nwordv[5]) > (8-2))	/* this is the pointer */
				nwordv[5] += (8-2);	/* bump over 6 characters */
		}
		cur_wordv = nwordv - 1;		/* convert to 1 based */
		cur_wordc += longpiptr ? 2 : 4;
		return (C_TRUE);
	}
	if (cur_wordc >= 4
	    && strlen(cur_wordv[1]) == 1
	    && (*cur_wordv[1] == 'E' || *cur_wordv[1] == 'w' || *cur_wordv[1] == 'e')
	    && alldigits(cur_wordv[2])
	    && strlen(cur_wordv[3]) == 1
	    && cur_wordv[3][0] == '-'
	) {
		/*
		 * Message of the form: letter linenumber - message
		 * Turn into form: filename linenumber letter - message
		 */
		language = INPI;
		nwordv = wordvsplice(1, cur_wordc, cur_wordv + 1);
		nwordv[0] = strdup(currentfilename);
		nwordv[1] = cur_wordv[2];
		nwordv[2] = cur_wordv[1];
		c_linenumber = cur_wordv[2];
		cur_wordc += 1;
		cur_wordv = nwordv - 1;
		return (C_TRUE);
	}
	if (cur_wordc >= 3
	    && strlen(cur_wordv[1]) == 1
	    && (*cur_wordv[1] == 'E' || *cur_wordv[1] == 'w' || *cur_wordv[1] == 'e')
	    && strlen(cur_wordv[2]) == 1
	    && cur_wordv[2][0] == '-'
	) {
		/*
		 * Message of the form: letter - message
		 *
		 * This happens only when we are traversing the tree
		 * during the second pass of pi, and discover semantic
		 * errors.
		 *
		 * We have already (presumably) saved the header message
		 * and can now construct a nulled error message for the
		 * current file.
		 *
		 * Turns into a message of the form:
		 *      filename (header) letter - message
		 *
		 * First, see if it is a message referring to more than
		 * one line number.  Only of the form:
 		 *      %s undefined on line%s
 		 *      %s improperly used on line%s
		 */
		boolean undefined = 0;
		int wordindex;

		language = INPI;
		if ((undefined = (wordvcmp(cur_wordv+2, 3, pi_und1) == 0))
		     || (undefined = (wordvcmp(cur_wordv+2, 3, pi_und2) == 0))
		     || wordvcmp(cur_wordv+2, 4, pi_imp1) == 0
		     || wordvcmp(cur_wordv+2, 4, pi_imp2) == 0
		) {
			for (wordindex = undefined ? 5 : 6;
			     wordindex <= cur_wordc;
			     wordindex++) {
				if (nwordv) {
					free(nwordv[0]);
					free(nwordv);
				}
				nwordv = wordvsplice(2, undefined ? 2 : 3, cur_wordv+1);
				nwordv[0] = strdup(currentfilename);
				nwordv[1] = cur_wordv[wordindex];
				if (wordindex != cur_wordc)
					erroradd(undefined ? 4 : 5, nwordv,
						C_TRUE, C_UNKNOWN);
			}
			cur_wordc = undefined ? 4 : 5;
			cur_wordv = nwordv - 1;
			return (C_TRUE);
		}

		nwordv = wordvsplice(1+3, cur_wordc, cur_wordv+1);
		nwordv[0] = strdup(currentfilename);
		nwordv[1] = strdup(c_header[0]);
		nwordv[2] = strdup(c_header[1]);
		nwordv[3] = strdup(c_header[2]);
		cur_wordv = nwordv - 1;
		cur_wordc += 1 + 3;
		return (C_THISFILE);
	}
	if (strcmp(cur_wordv[1], "...") == 0 && c_linenumber &&
	    currentfilename != default_currentfilename) {
		/*
		 * have a continuation error message
		 * of the form: ... message
		 * Turn into form : filename linenumber message
		 */
		language = INPI;
		nwordv = wordvsplice(1, cur_wordc, cur_wordv+1);
		nwordv[0] = strdup(currentfilename);
		nwordv[1] = strdup(c_linenumber);
		cur_wordv = nwordv - 1;
		cur_wordc += 1;
		return (C_TRUE);
	}
	if (cur_wordc == 6
	   && lastchar(cur_wordv[6]) == ':'
	   && isdateformat(5, cur_wordv + 1)
	) {
		/*
		 * Have message that tells us we have changed files
		 */
		language = INPI;
		currentfilename = strdup(cur_wordv[6]);
		clob_last(currentfilename, '\0');
		return (C_SYNC);
	}
	if (cur_wordc == 3
	   && strcmp(cur_wordv[1], "In") == 0
	   && lastchar(cur_wordv[3]) == ':'
	   && instringset(cur_wordv[2], Piroutines)
	) {
		language = INPI;
		c_header = wordvsplice(0, cur_wordc, cur_wordv+1);
		return (C_SYNC);
	}

	/*
	 * now, check for just the line number followed by the text
	 */
	if (alldigits(cur_wordv[1])) {
		language = INPI;
		c_linenumber = cur_wordv[1];
		return (C_IGNORE);
	}

	/*
	 * Attempt to match messages refering to a line number
	 *
	 * Multiply defined label in case, lines %d and %d
	 * Goto %s from line %d is into a structured statement
	 * End matched %s on line %d
	 * Inserted keyword end matching %s on line %d
	 */
	multiple = structured = 0;
	if (
	       (cur_wordc == 6 && wordvcmp(cur_wordv+1, 2, pi_Endmatched) == 0)
	    || (cur_wordc == 8 && wordvcmp(cur_wordv+1, 4, pi_Inserted) == 0)
	    || (multiple = (cur_wordc == 9 && wordvcmp(cur_wordv+1,6, pi_multiple) == 0))
	    || (structured = (cur_wordc == 10 && wordvcmp(cur_wordv+6,5, pi_structured) == 0))
	) {
		language = INPI;
		nwordv = wordvsplice(2, cur_wordc, cur_wordv+1);
		nwordv[0] = strdup(currentfilename);
		nwordv[1] = structured ? cur_wordv [5] : cur_wordv[cur_wordc];
		cur_wordc += 2;
		cur_wordv = nwordv - 1;
		if (!multiple)
			return (C_TRUE);
		erroradd(cur_wordc, nwordv, C_TRUE, C_UNKNOWN);
		nwordv = wordvsplice(0, cur_wordc, nwordv);
		nwordv[1] = cur_wordv[cur_wordc - 2];
		return (C_TRUE);
	}
	return (C_UNKNOWN);
}
