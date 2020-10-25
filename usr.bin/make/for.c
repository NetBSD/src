/*	$NetBSD: for.c,v 1.98 2020/10/25 13:51:56 rillig Exp $	*/

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

/*-
 * Handling of .for/.endfor loops in a makefile.
 *
 * For loops are of the form:
 *
 * .for <varname...> in <value...>
 * ...
 * .endfor
 *
 * When a .for line is parsed, all following lines are accumulated into a
 * buffer, up to but excluding the corresponding .endfor line.  To find the
 * corresponding .endfor, the number of nested .for and .endfor directives
 * are counted.
 *
 * During parsing, any nested .for loops are just passed through; they get
 * handled recursively in For_Eval when the enclosing .for loop is evaluated
 * in For_Run.
 *
 * When the .for loop has been parsed completely, the variable expressions
 * for the iteration variables are replaced with expressions of the form
 * ${:Uvalue}, and then this modified body is "included" as a special file.
 *
 * Interface:
 *	For_Eval	Evaluate the loop in the passed line.
 *
 *	For_Run		Run accumulated loop
 */

#include    "make.h"

/*	"@(#)for.c	8.1 (Berkeley) 6/6/93"	*/
MAKE_RCSID("$NetBSD: for.c,v 1.98 2020/10/25 13:51:56 rillig Exp $");

typedef enum ForEscapes {
    FOR_SUB_ESCAPE_CHAR = 0x0001,
    FOR_SUB_ESCAPE_BRACE = 0x0002,
    FOR_SUB_ESCAPE_PAREN = 0x0004
} ForEscapes;

static int forLevel = 0;	/* Nesting level */

/* One of the variables to the left of the "in" in a .for loop. */
typedef struct ForVar {
    char *name;
    size_t len;
} ForVar;

/* One of the items to the right of the "in" in a .for loop. */
typedef struct ForItem {
    char *value;		/* unexpanded */
    ForEscapes escapes;
} ForItem;

/*
 * State of a for loop.
 */
typedef struct For {
    Buffer buf;			/* Body of loop */
    Vector /* of ForVar */ vars;
    Vector /* of ForItem */ items;	/* Substitution items */
    char *parse_buf;
    /* Is any of the names 1 character long? If so, when the variable values
     * are substituted, the parser must handle $V expressions as well, not
     * only ${V} and $(V). */
    Boolean short_var;
    unsigned int sub_next;
} For;

static For *accumFor;		/* Loop being accumulated */

static void
ForAddVar(For *f, const char *name, size_t len)
{
    ForVar *var = Vector_Push(&f->vars);
    var->name = bmake_strldup(name, len);
    var->len = len;
}

static void
ForVarDone(ForVar *var)
{
    free(var->name);
}

static void
ForAddItem(For *f, const char *value, ForEscapes escapes)
{
    ForItem *item = Vector_Push(&f->items);
    item->value = bmake_strdup(value);
    item->escapes = escapes;
}

static void
ForItemDone(ForItem *item)
{
    free(item->value);
}

static void
For_Free(For *arg)
{
    Buf_Destroy(&arg->buf, TRUE);

    while (arg->vars.len > 0)
        ForVarDone(Vector_Pop(&arg->vars));
    Vector_Done(&arg->vars);

    while (arg->items.len > 0)
	ForItemDone(Vector_Pop(&arg->items));
    Vector_Done(&arg->items);

    free(arg->parse_buf);

    free(arg);
}

/* Evaluate the for loop in the passed line. The line looks like this:
 *	.for <varname...> in <value...>
 *
 * Input:
 *	line		Line to parse
 *
 * Results:
 *      0: Not a .for statement, parse the line
 *	1: We found a for loop
 *     -1: A .for statement with a bad syntax error, discard.
 */
int
For_Eval(const char *line)
{
    For *new_for;
    const char *ptr;
    Words words;

    /* Skip the '.' and any following whitespace */
    ptr = line + 1;
    cpp_skip_whitespace(&ptr);

    /*
     * If we are not in a for loop quickly determine if the statement is
     * a for.
     */
    if (ptr[0] != 'f' || ptr[1] != 'o' || ptr[2] != 'r' ||
	!ch_isspace(ptr[3])) {
	if (ptr[0] == 'e' && strncmp(ptr + 1, "ndfor", 5) == 0) {
	    Parse_Error(PARSE_FATAL, "for-less endfor");
	    return -1;
	}
	return 0;
    }
    ptr += 3;

    /*
     * we found a for loop, and now we are going to parse it.
     */

    new_for = bmake_malloc(sizeof *new_for);
    Buf_Init(&new_for->buf, 0);
    Vector_Init(&new_for->vars, sizeof(ForVar));
    Vector_Init(&new_for->items, sizeof(ForItem));
    new_for->parse_buf = NULL;
    new_for->short_var = FALSE;
    new_for->sub_next = 0;

    /* Grab the variables. Terminate on "in". */
    for (;;) {
	size_t len;

	cpp_skip_whitespace(&ptr);
	if (*ptr == '\0') {
	    Parse_Error(PARSE_FATAL, "missing `in' in for");
	    For_Free(new_for);
	    return -1;
	}

	for (len = 1; ptr[len] && !ch_isspace(ptr[len]); len++)
	    continue;
	if (len == 2 && ptr[0] == 'i' && ptr[1] == 'n') {
	    ptr += 2;
	    break;
	}
	if (len == 1)
	    new_for->short_var = TRUE;

	ForAddVar(new_for, ptr, len);
	ptr += len;
    }

    if (new_for->vars.len == 0) {
	Parse_Error(PARSE_FATAL, "no iteration variables in for");
	For_Free(new_for);
	return -1;
    }

    cpp_skip_whitespace(&ptr);

    /*
     * Make a list with the remaining words.
     * The values are later substituted as ${:U<value>...} so we must
     * backslash-escape characters that break that syntax.
     * Variables are fully expanded - so it is safe for escape $.
     * We can't do the escapes here - because we don't know whether
     * we will be substituting into ${...} or $(...).
     */
    {
	char *items;
	(void)Var_Subst(ptr, VAR_GLOBAL, VARE_WANTRES, &items);
	/* TODO: handle errors */
	words = Str_Words(items, FALSE);
	free(items);
    }

    {
	size_t n;

	for (n = 0; n < words.len; n++) {
	    ForEscapes escapes;
	    char ch;

	    ptr = words.words[n];
	    if (ptr[0] == '\0')
		continue;
	    escapes = 0;
	    while ((ch = *ptr++)) {
		switch (ch) {
		case ':':
		case '$':
		case '\\':
		    escapes |= FOR_SUB_ESCAPE_CHAR;
		    break;
		case ')':
		    escapes |= FOR_SUB_ESCAPE_PAREN;
		    break;
		case '}':
		    escapes |= FOR_SUB_ESCAPE_BRACE;
		    break;
		}
	    }
	    ForAddItem(new_for, words.words[n], escapes);
	}
    }

    Words_Free(words);

    {
	size_t nitems, nvars;

	if ((nitems = new_for->items.len) > 0 &&
	    nitems % (nvars = new_for->vars.len)) {
	    Parse_Error(PARSE_FATAL,
			"Wrong number of words (%zu) in .for substitution list"
			" with %zu vars", nitems, nvars);
	    /*
	     * Return 'success' so that the body of the .for loop is
	     * accumulated.
	     * Remove all items so that the loop doesn't iterate.
	     */
	    while (new_for->items.len > 0)
		ForItemDone(Vector_Pop(&new_for->items));
	}
    }

    accumFor = new_for;
    forLevel = 1;
    return 1;
}

/*
 * Add another line to a .for loop.
 * Returns FALSE when the matching .endfor is reached.
 */
Boolean
For_Accum(const char *line)
{
    const char *ptr = line;

    if (*ptr == '.') {
	ptr++;
	cpp_skip_whitespace(&ptr);

	if (strncmp(ptr, "endfor", 6) == 0 && (ch_isspace(ptr[6]) || !ptr[6])) {
	    DEBUG1(FOR, "For: end for %d\n", forLevel);
	    if (--forLevel <= 0)
		return FALSE;
	} else if (strncmp(ptr, "for", 3) == 0 && ch_isspace(ptr[3])) {
	    forLevel++;
	    DEBUG1(FOR, "For: new loop %d\n", forLevel);
	}
    }

    Buf_AddStr(&accumFor->buf, line);
    Buf_AddByte(&accumFor->buf, '\n');
    return TRUE;
}


static size_t
for_var_len(const char *var)
{
    char ch, var_start, var_end;
    int depth;
    size_t len;

    var_start = *var;
    if (var_start == 0)
	/* just escape the $ */
	return 0;

    if (var_start == '(')
	var_end = ')';
    else if (var_start == '{')
	var_end = '}';
    else
	/* Single char variable */
	return 1;

    depth = 1;
    for (len = 1; (ch = var[len++]) != 0;) {
	if (ch == var_start)
	    depth++;
	else if (ch == var_end && --depth == 0)
	    return len;
    }

    /* Variable end not found, escape the $ */
    return 0;
}

static void
for_substitute(Buffer *cmds, ForItem *forItem, char ech)
{
    const char *item = forItem->value;
    ForEscapes escapes = forItem->escapes;
    char ch;

    /* If there were no escapes, or the only escape is the other variable
     * terminator, then just substitute the full string */
    if (!(escapes & (ech == ')' ? ~(unsigned)FOR_SUB_ESCAPE_BRACE
				: ~(unsigned)FOR_SUB_ESCAPE_PAREN))) {
	Buf_AddStr(cmds, item);
	return;
    }

    /* Escape ':', '$', '\\' and 'ech' - these will be removed later by
     * :U processing, see ApplyModifier_Defined. */
    while ((ch = *item++) != '\0') {
	if (ch == '$') {
	    size_t len = for_var_len(item);
	    if (len != 0) {
		Buf_AddBytes(cmds, item - 1, len + 1);
		item += len;
		continue;
	    }
	    Buf_AddByte(cmds, '\\');
	} else if (ch == ':' || ch == '\\' || ch == ech)
	    Buf_AddByte(cmds, '\\');
	Buf_AddByte(cmds, ch);
    }
}

static void
SubstVarLong(For *arg, const char **inout_cp, const char **inout_cmd_cp,
	     Buffer *cmds, char ech)
{
    size_t i;
    const char *cp = *inout_cp;
    const char *cmd_cp = *inout_cmd_cp;

    for (i = 0; i < arg->vars.len; i++) {
	ForVar *forVar = Vector_Get(&arg->vars, i);
	char *var = forVar->name;
	size_t vlen = forVar->len;

	/* XXX: undefined behavior for cp if vlen is longer than cp? */
	if (memcmp(cp, var, vlen) != 0)
	    continue;
	/* XXX: why test for backslash here? */
	if (cp[vlen] != ':' && cp[vlen] != ech && cp[vlen] != '\\')
	    continue;

	/* Found a variable match. Replace with :U<value> */
	Buf_AddBytesBetween(cmds, cmd_cp, cp);
	Buf_AddStr(cmds, ":U");
	cp += vlen;
	cmd_cp = cp;
	for_substitute(cmds, Vector_Get(&arg->items, arg->sub_next + i), ech);
	break;
    }

    *inout_cp = cp;
    *inout_cmd_cp = cmd_cp;
}

static void
SubstVarShort(For *arg, char const ch,
	      const char **inout_cp, const char **input_cmd_cp, Buffer *cmds)
{
    const char *cp = *inout_cp;
    const char *cmd_cp = *input_cmd_cp;
    size_t i;

    /* Probably a single character name, ignore $$ and stupid ones. {*/
    if (!arg->short_var || strchr("}):$", ch) != NULL) {
	cp++;
	*inout_cp = cp;
	return;
    }

    for (i = 0; i < arg->vars.len; i++) {
	ForVar *forVar = Vector_Get(&arg->vars, i);
	char *var = forVar->name;
	if (var[0] != ch || var[1] != 0)
	    continue;

	/* Found a variable match. Replace with ${:U<value>} */
	Buf_AddBytesBetween(cmds, cmd_cp, cp);
	Buf_AddStr(cmds, "{:U");
	cmd_cp = ++cp;
	for_substitute(cmds, Vector_Get(&arg->items, arg->sub_next + i), '}');
	Buf_AddByte(cmds, '}');
	break;
    }

    *inout_cp = cp;
    *input_cmd_cp = cmd_cp;
}

/*
 * Scan the for loop body and replace references to the loop variables
 * with variable references that expand to the required text.
 *
 * Using variable expansions ensures that the .for loop can't generate
 * syntax, and that the later parsing will still see a variable.
 * We assume that the null variable will never be defined.
 *
 * The detection of substitutions of the loop control variable is naive.
 * Many of the modifiers use \ to escape $ (not $) so it is possible
 * to contrive a makefile where an unwanted substitution happens.
 */
static char *
ForIterate(void *v_arg, size_t *ret_len)
{
    For *arg = v_arg;
    const char *cp;
    const char *cmd_cp;
    const char *body_end;
    Buffer cmds;
    char *cmds_str;
    size_t cmd_len;

    if (arg->sub_next + arg->vars.len > arg->items.len) {
	/* No more iterations */
	For_Free(arg);
	return NULL;
    }

    free(arg->parse_buf);
    arg->parse_buf = NULL;

    cmd_cp = Buf_GetAll(&arg->buf, &cmd_len);
    body_end = cmd_cp + cmd_len;
    Buf_Init(&cmds, cmd_len + 256);
    for (cp = cmd_cp; (cp = strchr(cp, '$')) != NULL;) {
	char ch, ech;
	ch = *++cp;
	if ((ch == '(' && (ech = ')', 1)) || (ch == '{' && (ech = '}', 1))) {
	    cp++;
	    /* Check variable name against the .for loop variables */
	    SubstVarLong(arg, &cp, &cmd_cp, &cmds, ech);
	    continue;
	}
	if (ch == '\0')
	    break;

	SubstVarShort(arg, ch, &cp, &cmd_cp, &cmds);
    }
    Buf_AddBytesBetween(&cmds, cmd_cp, body_end);

    *ret_len = Buf_Len(&cmds);
    cmds_str = Buf_Destroy(&cmds, FALSE);
    DEBUG1(FOR, "For: loop body:\n%s", cmds_str);

    arg->sub_next += arg->vars.len;

    arg->parse_buf = cmds_str;
    return cmds_str;
}

/* Run the for loop, imitating the actions of an include file. */
void
For_Run(int lineno)
{
    For *arg;

    arg = accumFor;
    accumFor = NULL;

    if (arg->items.len == 0) {
	/* Nothing to expand - possibly due to an earlier syntax error. */
	For_Free(arg);
	return;
    }

    Parse_SetInput(NULL, lineno, -1, ForIterate, arg);
}
