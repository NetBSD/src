/*	$NetBSD: for.c,v 1.24.2.1 2007/06/05 20:53:29 bouyer Exp $	*/

/*
 * Copyright (c) 1992, The Regents of the University of California.
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

#ifndef MAKE_NATIVE
static char rcsid[] = "$NetBSD: for.c,v 1.24.2.1 2007/06/05 20:53:29 bouyer Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)for.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: for.c,v 1.24.2.1 2007/06/05 20:53:29 bouyer Exp $");
#endif
#endif /* not lint */
#endif

/*-
 * for.c --
 *	Functions to handle loops in a makefile.
 *
 * Interface:
 *	For_Eval 	Evaluate the loop in the passed line.
 *	For_Run		Run accumulated loop
 *
 */

#include    <assert.h>
#include    <ctype.h>

#include    "make.h"
#include    "hash.h"
#include    "dir.h"
#include    "buf.h"

/*
 * For statements are of the form:
 *
 * .for <variable> in <varlist>
 * ...
 * .endfor
 *
 * The trick is to look for the matching end inside for for loop
 * To do that, we count the current nesting level of the for loops.
 * and the .endfor statements, accumulating all the statements between
 * the initial .for loop and the matching .endfor;
 * then we evaluate the for loop for each variable in the varlist.
 *
 * Note that any nested fors are just passed through; they get handled
 * recursively in For_Eval when we're expanding the enclosing for in
 * For_Run.
 */

static int  	  forLevel = 0;  	/* Nesting level	*/

/*
 * State of a for loop.
 */
typedef struct _For {
    Buffer	  buf;			/* Body of loop		*/
    char	**vars;			/* Iteration variables	*/
    int           nvars;		/* # of iteration vars	*/
    Lst  	  lst;			/* List of items	*/
} For;

static For        accumFor;             /* Loop being accumulated */

static void ForAddVar(const char *, size_t);




/*-
 *-----------------------------------------------------------------------
 * ForAddVar --
 *	Add an iteration variable to the currently accumulating for.
 *
 * Results: none
 * Side effects: no additional side effects.
 *-----------------------------------------------------------------------
 */
static void
ForAddVar(const char *data, size_t len)
{
	Buffer buf;
	int varlen;

	buf = Buf_Init(0);
	Buf_AddBytes(buf, len, (Byte *)UNCONST(data));

	accumFor.nvars++;
	accumFor.vars = erealloc(accumFor.vars, accumFor.nvars*sizeof(char *));

	accumFor.vars[accumFor.nvars-1] = (char *)Buf_GetAll(buf, &varlen);

	Buf_Destroy(buf, FALSE);
}

/*-
 *-----------------------------------------------------------------------
 * For_Eval --
 *	Evaluate the for loop in the passed line. The line
 *	looks like this:
 *	    .for <variable> in <varlist>
 *
 * Input:
 *	line		Line to parse
 *
 * Results:
 *	TRUE: We found a for loop, or we are inside a for loop
 *	FALSE: We did not find a for loop, or we found the end of the for
 *	       for loop.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
For_Eval(char *line)
{
    char	    *ptr = line, *sub, *in, *wrd;
    int	    	    level;  	/* Level at which to report errors. */

    level = PARSE_FATAL;


    if (forLevel == 0) {
	Buffer	    buf;
	int	    varlen;
	static const char instr[] = "in";

	for (ptr++; *ptr && isspace((unsigned char) *ptr); ptr++)
	    continue;
	/*
	 * If we are not in a for loop quickly determine if the statement is
	 * a for.
	 */
	if (ptr[0] != 'f' || ptr[1] != 'o' || ptr[2] != 'r' ||
		!isspace((unsigned char) ptr[3]))
	    return FALSE;
	ptr += 3;

	/*
	 * we found a for loop, and now we are going to parse it.
	 */
	while (*ptr && isspace((unsigned char) *ptr))
	    ptr++;

	/*
	 * Find the "in".
	 */
	for (in = ptr; *in; in++) {
	    if (isspace((unsigned char) in[0]) && in[1]== 'i' &&
		in[2] == 'n' &&
		(in[3] == '\0' || isspace((unsigned char) in[3])))
		break;
	}
	if (*in == '\0') {
	    Parse_Error(level, "missing `in' in for");
	    return 0;
	}

	/*
	 * Grab the variables.
	 */
	accumFor.vars = NULL;

	while (ptr < in) {
	    wrd = ptr;
	    while (*ptr && !isspace((unsigned char) *ptr))
	        ptr++;
	    ForAddVar(wrd, ptr - wrd);
	    while (*ptr && isspace((unsigned char) *ptr))
		ptr++;
	}

	if (accumFor.nvars == 0) {
	    Parse_Error(level, "no iteration variables in for");
	    return 0;
	}

	/* At this point we should be pointing right at the "in" */
	/*
	 * compensate for hp/ux's brain damaged assert macro that
	 * does not handle double quotes nicely.
	 */
	assert(!memcmp(ptr, instr, 2));
	ptr += 2;

	while (*ptr && isspace((unsigned char) *ptr))
	    ptr++;

	/*
	 * Make a list with the remaining words
	 */
	accumFor.lst = Lst_Init(FALSE);
	buf = Buf_Init(0);
	sub = Var_Subst(NULL, ptr, VAR_GLOBAL, FALSE);

#define ADDWORD() \
	Buf_AddBytes(buf, ptr - wrd, (Byte *)wrd), \
	Buf_AddByte(buf, (Byte)'\0'), \
	Lst_AtFront(accumFor.lst, Buf_GetAll(buf, &varlen)), \
	Buf_Destroy(buf, FALSE)

	for (ptr = sub; *ptr && isspace((unsigned char) *ptr); ptr++)
	    continue;

	for (wrd = ptr; *ptr; ptr++)
	    if (isspace((unsigned char) *ptr)) {
		ADDWORD();
		buf = Buf_Init(0);
		while (*ptr && isspace((unsigned char) *ptr))
		    ptr++;
		wrd = ptr--;
	    }
	if (DEBUG(FOR)) {
	    int i;
	    for (i = 0; i < accumFor.nvars; i++) {
		(void)fprintf(debug_file, "For: variable %s\n", accumFor.vars[i]);
	    }
	    (void)fprintf(debug_file, "For: list %s\n", sub);
	}
	if (ptr - wrd > 0)
	    ADDWORD();
	else
	    Buf_Destroy(buf, TRUE);
	free(sub);

	accumFor.buf = Buf_Init(0);
	forLevel++;
	return 1;
    }

    if (*ptr == '.') {

	for (ptr++; *ptr && isspace((unsigned char) *ptr); ptr++)
	    continue;

	if (strncmp(ptr, "endfor", 6) == 0 &&
		(isspace((unsigned char) ptr[6]) || !ptr[6])) {
	    if (DEBUG(FOR))
		(void)fprintf(debug_file, "For: end for %d\n", forLevel);
	    if (--forLevel < 0) {
		Parse_Error(level, "for-less endfor");
		return 0;
	    }
	} else if (strncmp(ptr, "for", 3) == 0 &&
		 isspace((unsigned char) ptr[3])) {
	    forLevel++;
	    if (DEBUG(FOR))
		(void)fprintf(debug_file, "For: new loop %d\n", forLevel);
	}
    }

    if (forLevel != 0) {
	Buf_AddBytes(accumFor.buf, strlen(line), (Byte *)line);
	Buf_AddByte(accumFor.buf, (Byte)'\n');
	return 1;
    }

    return 0;
}


/*-
 *-----------------------------------------------------------------------
 * For_Run --
 *	Run the for loop, imitating the actions of an include file
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
void
For_Run(int lineno)
{
    For arg;
    LstNode ln;
    char **values;
    int i, done = 0, len;
    char *guy, *orig_guy, *old_guy;

    if (accumFor.buf == NULL || accumFor.vars == NULL || accumFor.lst == NULL)
	return;
    arg = accumFor;
    accumFor.buf = NULL;
    accumFor.vars = NULL;
    accumFor.nvars = 0;
    accumFor.lst = NULL;

    if (Lst_Open(arg.lst) != SUCCESS)
	return;

    values = emalloc(arg.nvars * sizeof(char *));
    
    while (!done) {
	/* 
	 * due to the dumb way this is set up, this loop must run
	 * backwards.
	 */
	for (i = arg.nvars - 1; i >= 0; i--) {
	    ln = Lst_Next(arg.lst);
	    if (ln == NILLNODE) {
		if (i != arg.nvars-1) {
		    Parse_Error(PARSE_FATAL, 
			"Not enough words in for substitution list");
		}
		done = 1;
		break;
	    } else {
		values[i] = (char *)Lst_Datum(ln);
	    }
	}
	if (done)
	    break;

	for (i = 0; i < arg.nvars; i++) {
	    Var_Set(arg.vars[i], values[i], VAR_GLOBAL, 0);
	    if (DEBUG(FOR))
		(void)fprintf(debug_file, "--- %s = %s\n", arg.vars[i], 
		    values[i]);
	}

	/*
	 * Hack, hack, kludge.
	 * This is really ugly, but to do it any better way would require
	 * making major changes to var.c, which I don't want to get into
	 * yet. There is no mechanism for expanding some variables, only
	 * for expanding a single variable. That should be corrected, but
	 * not right away. (XXX)
	 */
	
	guy = (char *)Buf_GetAll(arg.buf, &len);
	orig_guy = guy;
	for (i = 0; i < arg.nvars; i++) {
	    old_guy = guy;
	    guy = Var_Subst(arg.vars[i], guy, VAR_GLOBAL, FALSE);
	    if (old_guy != orig_guy)
		free(old_guy);
	}
	Parse_SetInput(NULL, lineno, -1, guy);

	for (i = 0; i < arg.nvars; i++)
	    Var_Delete(arg.vars[i], VAR_GLOBAL);
    }

    free(values);

    Lst_Close(arg.lst);

    for (i=0; i<arg.nvars; i++) {
	free(arg.vars[i]);
    }
    free(arg.vars);

    Lst_Destroy(arg.lst, (FreeProc *)free);
    Buf_Destroy(arg.buf, TRUE);
}
