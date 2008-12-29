/*	$NetBSD: for.c,v 1.41 2008/12/29 10:12:30 dsl Exp $	*/

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
static char rcsid[] = "$NetBSD: for.c,v 1.41 2008/12/29 10:12:30 dsl Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)for.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: for.c,v 1.41 2008/12/29 10:12:30 dsl Exp $");
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
#include    "strlist.h"

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
    strlist_t     vars;			/* Iteration variables	*/
    strlist_t     items;		/* Substitution items */
} For;

static For        accumFor;             /* Loop being accumulated */



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
 * For_Eval --
 *	Evaluate the for loop in the passed line. The line
 *	looks like this:
 *	    .for <variable> in <varlist>
 *
 * Input:
 *	line		Line to parse
 *
 * Results:
 *      0: Not a .for statement, parse the line
 *	1: We found a for loop
 *     -1: A .for statement with a bad syntax error, discard.
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
    int i;
    int escapes;
    int depth;
    char ch;

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
	strlist_add_str(&accumFor.vars, make_str(ptr, len));
    }

    if (strlist_num(&accumFor.vars) == 0) {
	Parse_Error(PARSE_FATAL, "no iteration variables in for");
	return -1;
    }

    while (*ptr && isspace((unsigned char) *ptr))
	ptr++;

    /*
     * Make a list with the remaining words
     * The values are substituted as ${:U<value>... so we must \ escape
     * characters that break that syntax - particularly ':', maybe $ and \.
     */
    sub = Var_Subst(NULL, ptr, VAR_GLOBAL, FALSE);

    for (ptr = sub;; ptr += len) {
	while (*ptr && isspace((unsigned char)*ptr))
	    ptr++;
	if (*ptr == 0)
	    break;
	escapes = 0;
	for (len = 0; ptr[len] && !isspace((unsigned char)ptr[len]); len++)
	    if (ptr[len] == ':')
		escapes++;
	if (escapes == 0)
	    strlist_add_str(&accumFor.items, make_str(ptr, len));
	else {
	    char *item = bmake_malloc(len + escapes + 1);
	    strlist_add_str(&accumFor.items, item);
	    for (depth= 0, i = 0; i < len; i++) {
		ch = ptr[i];
		/* Loose determination of nested variable definitions. */
		if (ch == '(' || ch == '{')
		    depth++;
		else if (ch == ')' || ch == '}')
		    depth--;
		else if (ch == ':' && depth == 0)
		    *item++ = '\\';
		*item++ = ch;
	    }
	    *item = 0;
	}
    }

    free(sub);

    if (strlist_num(&accumFor.items) % strlist_num(&accumFor.vars)) {
	Parse_Error(PARSE_FATAL,
		"Wrong number of words in .for substitution list %d %d",
		strlist_num(&accumFor.items), strlist_num(&accumFor.vars));
	/*
	 * Return 'success' so that the body of the .for loop is accumulated.
	 * The loop will have zero iterations expanded due a later test.
	 */
    }

    accumFor.buf = Buf_Init(0);
    forLevel = 1;
    return 1;
}

/*
 * Add another line to a .for loop.
 * Returns 0 when the matching .enfor is reached.
 */

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
    int i, len;
    unsigned int num_items;
    char *for_body;
    char *var, *item;
    char *cp;
    char *cmd_cp;
    char *body_end;
    char ch;
    Buffer cmds;
    int short_var;

    arg = accumFor;
    memset(&accumFor, 0, sizeof accumFor);

    num_items = strlist_num(&arg.items);
    if (num_items % strlist_num(&arg.vars))
	/* Error message already printed */
	goto out;

    short_var = 0;
    STRLIST_FOREACH(var, &arg.vars, i) {
	if (var[1] == 0) {
	    short_var = 1;
	    break;
	}
    }

    /*
     * Scan the for loop body and replace references to the loop variables
     * with variable references that expand to the required text.
     * Using variable expansions ensures that the .for loop can't generate
     * syntax, and that the later parsing will still see a variable.
     * We assume that the null variable will never be defined.
     *
     * The detection of substitions of the loop control variable is naive.
     * Many of the modifiers use \ to escape $ (not $) so it is possible
     * to contrive a makefile where an unwanted substitution happens.
     *
     * Each loop expansion is fed back into the parser as if it were an
     * include file.  This means we have to generate the last iteration first.
     */
    while (num_items != 0) {
	num_items -= strlist_num(&arg.vars);
	for_body = (char *)Buf_GetAll(arg.buf, &len);
	body_end = for_body + len;
	cmds = Buf_Init(len + 256);
	cmd_cp = for_body;
	for (cp = for_body; (cp = strchr(cp, '$')) != NULL;) {
	    ch = *++cp;
	    if (ch == '(' || ch == '{') {
		char ech = ch == '(' ? ')' : '}';
		cp++;
		/* Check variable name against the .for loop variables */
		STRLIST_FOREACH(var, &arg.vars, i) {
		    len = strlen(var);
		    if (memcmp(cp, var, len) != 0)
			continue;
		    if (cp[len] != ':' && cp[len] != ech && cp[len] != '\\')
			continue;
		    /* Found a variable match. Replace with ${:U<value> */
		    Buf_AddBytes(cmds, cp - cmd_cp, cmd_cp);
		    Buf_AddBytes(cmds, 2, ":U");
		    cp += len;
		    cmd_cp = cp;
		    item = strlist_str(&arg.items, num_items + i);
		    Buf_AddBytes(cmds, strlen(item), item);
		    break;
		}
		continue;
	    }
	    if (ch == 0)
		break;
	    /* Probably a single character name, ignore $$ and stupid ones. */
	    if (!short_var || strchr("}):$", ch) != NULL) {
		cp++;
		continue;
	    }
	    STRLIST_FOREACH(var, &arg.vars, i) {
		if (var[0] != ch || var[1] != 0)
		    continue;
		/* Found a variable match. Replace with ${:U<value>} */
		Buf_AddBytes(cmds, cp - cmd_cp, cmd_cp);
		Buf_AddBytes(cmds, 3, "{:U");
		cmd_cp = ++cp;
		item = strlist_str(&arg.items, num_items + i);
		Buf_AddBytes(cmds, strlen(item), item);
		Buf_AddBytes(cmds, 1, "}");
		break;
	    }
	}
	Buf_AddBytes(cmds, body_end - cmd_cp, cmd_cp);

	cp = Buf_GetAll(cmds, NULL);
	if (DEBUG(FOR))
	    (void)fprintf(debug_file, "For: loop body:\n%s", cp);
	Parse_SetInput(NULL, lineno, -1, cp);
	Buf_Destroy(cmds, FALSE);
    }

  out:
    strlist_clean(&arg.vars);
    strlist_clean(&arg.items);

    Buf_Destroy(arg.buf, TRUE);
}
