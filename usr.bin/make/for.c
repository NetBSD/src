/*	$NetBSD: for.c,v 1.33 2008/11/30 22:37:55 dsl Exp $	*/

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
static char rcsid[] = "$NetBSD: for.c,v 1.33 2008/11/30 22:37:55 dsl Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)for.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: for.c,v 1.33 2008/11/30 22:37:55 dsl Exp $");
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
    int           nitem;		/* # of substitution items */
    Lst  	  lst;			/* List of items	*/
} For;

static For        accumFor;             /* Loop being accumulated */

static void ForAddVar(const char *, size_t);




static char *
make_str(const char *ptr, int len)
{
	char *new_ptr;

	new_ptr = bmake_malloc(len + 1);
	memcpy(new_ptr, ptr, len);
	new_ptr[len] = 0;
	return new_ptr;
}

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
	int nvars;

	nvars = accumFor.nvars;
	accumFor.nvars = nvars + 1;
	accumFor.vars = bmake_realloc(accumFor.vars, nvars * sizeof(char *));

	accumFor.vars[nvars] = make_str(data, len);
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
    char *ptr = line, *sub;
    int len;

    /* Forget anything we previously knew about - it cannot be useful */
    memset(&accumFor, 0, sizeof accumFor);

    forLevel = 0;
    for (ptr++; *ptr && isspace((unsigned char) *ptr); ptr++)
	continue;
    /*
     * If we are not in a for loop quickly determine if the statement is
     * a for.
     */
    if (ptr[0] != 'f' || ptr[1] != 'o' || ptr[2] != 'r' ||
	    !isspace((unsigned char) ptr[3])) {
	if (ptr[0] == 'e' && strncmp(ptr+1, "ndfor", 5) == 0) {
	    Parse_Error(PARSE_FATAL, "for-less endfor");
	    return -1;
	}
	return 0;
    }
    ptr += 3;

    /*
     * we found a for loop, and now we are going to parse it.
     */

    /* Grab the variables. Terminate on "in". */
    for (;; ptr += len) {
	while (*ptr && isspace((unsigned char) *ptr))
	    ptr++;
	if (*ptr == '\0') {
	    Parse_Error(PARSE_FATAL, "missing `in' in for");
	    return -1;
	}
	for (len = 1; ptr[len] && !isspace((unsigned char)ptr[len]); len++)
	    continue;
	if (len == 2 && ptr[0] == 'i' && ptr[1] == 'n') {
	    ptr += 2;
	    break;
	}
	ForAddVar(ptr, len);
    }

    if (accumFor.nvars == 0) {
	Parse_Error(PARSE_FATAL, "no iteration variables in for");
	return -1;
    }

    while (*ptr && isspace((unsigned char) *ptr))
	ptr++;

    /*
     * Make a list with the remaining words
     */
    accumFor.lst = Lst_Init(FALSE);
    sub = Var_Subst(NULL, ptr, VAR_GLOBAL, FALSE);

    for (ptr = sub;; ptr += len, accumFor.nitem++) {
	while (*ptr && isspace((unsigned char)*ptr))
	    ptr++;
	if (*ptr == 0)
	    break;
	for (len = 1; ptr[len] && !isspace((unsigned char)ptr[len]); len++)
	    continue;
	Lst_AtFront(accumFor.lst, make_str(ptr, len));
    }

    free(sub);

    if (accumFor.nitem % accumFor.nvars) {
	Parse_Error(PARSE_FATAL,
		"Wrong number of words in .for substitution list %d %d",
		accumFor.nitem, accumFor.nvars);
	/*
	 * Return 'success' so that the body of the .for loop is accumulated.
	 * The loop will have zero iterations expanded due a later test.
	 */
    }

    accumFor.buf = Buf_Init(0);
    forLevel = 1;
    return 1;
}

int
For_Accum(char *line)
{
    char *ptr = line;

    if (*ptr == '.') {

	for (ptr++; *ptr && isspace((unsigned char) *ptr); ptr++)
	    continue;

	if (strncmp(ptr, "endfor", 6) == 0 &&
		(isspace((unsigned char) ptr[6]) || !ptr[6])) {
	    if (DEBUG(FOR))
		(void)fprintf(debug_file, "For: end for %d\n", forLevel);
	    if (--forLevel <= 0)
		return 0;
	} else if (strncmp(ptr, "for", 3) == 0 &&
		 isspace((unsigned char) ptr[3])) {
	    forLevel++;
	    if (DEBUG(FOR))
		(void)fprintf(debug_file, "For: new loop %d\n", forLevel);
	}
    }

    Buf_AddBytes(accumFor.buf, strlen(line), (Byte *)line);
    Buf_AddByte(accumFor.buf, (Byte)'\n');
    return 1;
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

    arg = accumFor;
    accumFor.buf = NULL;
    accumFor.vars = NULL;
    accumFor.nvars = 0;
    accumFor.lst = NULL;

    if (arg.nitem % arg.nvars)
	/* Error message already printed */
	return;

    if (Lst_Open(arg.lst) != SUCCESS)
	return;

    values = bmake_malloc(arg.nvars * sizeof(char *));
    
    while (!done) {
	/* 
	 * due to the dumb way this is set up, this loop must run
	 * backwards.
	 */
	for (i = arg.nvars - 1; i >= 0; i--) {
	    ln = Lst_Next(arg.lst);
	    if (ln == NILLNODE) {
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
