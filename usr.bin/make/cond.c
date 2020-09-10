/*	$NetBSD: cond.c,v 1.120 2020/09/10 23:37:54 rillig Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
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

/*
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
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

#ifndef MAKE_NATIVE
static char rcsid[] = "$NetBSD: cond.c,v 1.120 2020/09/10 23:37:54 rillig Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)cond.c	8.2 (Berkeley) 1/2/94";
#else
__RCSID("$NetBSD: cond.c,v 1.120 2020/09/10 23:37:54 rillig Exp $");
#endif
#endif /* not lint */
#endif

/* Handling of conditionals in a makefile.
 *
 * Interface:
 *	Cond_Eval 	Evaluate the conditional in the passed line.
 *
 *	Cond_EvalExpression
 *			Evaluate the conditional in the passed line, which
 *			is either the argument of one of the .if directives
 *			or the condition in a :?true:false variable modifier.
 *
 *	Cond_save_depth
 *	Cond_restore_depth
 *			Save and restore the nesting of the conditions, at
 *			the start and end of including another makefile, to
 *			ensure that in each makefile the conditional
 *			directives are well-balanced.
 */

#include <errno.h>

#include "make.h"
#include "dir.h"

/*
 * The parsing of conditional expressions is based on this grammar:
 *	E -> F || E
 *	E -> F
 *	F -> T && F
 *	F -> T
 *	T -> defined(variable)
 *	T -> make(target)
 *	T -> exists(file)
 *	T -> empty(varspec)
 *	T -> target(name)
 *	T -> commands(name)
 *	T -> symbol
 *	T -> $(varspec) op value
 *	T -> $(varspec) == "string"
 *	T -> $(varspec) != "string"
 *	T -> "string"
 *	T -> ( E )
 *	T -> ! T
 *	op -> == | != | > | < | >= | <=
 *
 * 'symbol' is some other symbol to which the default function is applied.
 *
 * Tokens are scanned from the lexer. The scanner (CondToken)
 * will return TOK_AND for '&' and '&&', TOK_OR for '|' and '||',
 * TOK_NOT for '!', TOK_LPAREN for '(', TOK_RPAREN for ')' and will evaluate
 * the other terminal symbols, using either the default function or the
 * function given in the terminal, and return the result as either TOK_TRUE
 * or TOK_FALSE.
 *
 * TOK_FALSE is 0 and TOK_TRUE 1 so we can directly assign C comparisons.
 *
 * All Non-Terminal functions (CondE, CondF and CondT) return TOK_ERROR on
 * error.
 */
typedef enum {
    TOK_FALSE = 0, TOK_TRUE = 1, TOK_AND, TOK_OR, TOK_NOT,
    TOK_LPAREN, TOK_RPAREN, TOK_EOF, TOK_NONE, TOK_ERROR
} Token;

typedef struct {
    const struct If *if_info;	/* Info for current statement */
    const char *condExpr;	/* The expression to parse */
    Token curr;			/* Single push-back token used in parsing */
} CondLexer;

static Token CondE(CondLexer *lex, Boolean);
static CondEvalResult do_Cond_EvalExpression(CondLexer *lex, Boolean *);

static unsigned int cond_depth = 0;	/* current .if nesting level */
static unsigned int cond_min_depth = 0;	/* depth at makefile open */

/*
 * Indicate when we should be strict about lhs of comparisons.
 * In strict mode, the lhs must be a variable expression or a string literal
 * in quotes. In non-strict mode it may also be an unquoted string literal.
 *
 * TRUE when Cond_EvalExpression is called from Cond_Eval (.if etc)
 * FALSE when Cond_EvalExpression is called from var.c:ApplyModifiers
 * since lhs is already expanded and we cannot tell if
 * it was a variable reference or not.
 */
static Boolean lhsStrict;

static int
istoken(const char *str, const char *tok, size_t len)
{
    return strncmp(str, tok, len) == 0 && !isalpha((unsigned char)str[len]);
}

/* Push back the most recent token read. We only need one level of this. */
static void
CondLexer_PushBack(CondLexer *lex, Token t)
{
    assert(lex->curr == TOK_NONE);
    assert(t != TOK_NONE);

    lex->curr = t;
}

static void
CondLexer_SkipWhitespace(CondLexer *lex)
{
    while (isspace((unsigned char)lex->condExpr[0]))
	lex->condExpr++;
}

/* Parse the argument of a built-in function.
 *
 * Arguments:
 *	*linePtr initially points to the '(', upon successful return points
 *	beyond the ')'.
 *
 *	*out_arg receives the argument as string.
 *
 *	func says whether the argument belongs to an actual function, or
 *	whether the parsed argument is passed to the default function.
 *
 * Return the length of the argument. */
static int
ParseFuncArg(Boolean doEval, const char **linePtr, char **out_arg,
	     const char *func) {
    const char *cp;
    Buffer buf;
    int paren_depth;
    char ch;
    size_t argLen;

    cp = *linePtr;
    if (func != NULL)
	/* Skip opening '(' - verified by caller */
	cp++;

    if (*cp == '\0') {
	/*
	 * No arguments whatsoever. Because 'make' and 'defined' aren't really
	 * "reserved words", we don't print a message. I think this is better
	 * than hitting the user with a warning message every time s/he uses
	 * the word 'make' or 'defined' at the beginning of a symbol...
	 */
	*out_arg = NULL;
	return 0;
    }

    while (*cp == ' ' || *cp == '\t') {
	cp++;
    }

    /*
     * Create a buffer for the argument and start it out at 16 characters
     * long. Why 16? Why not?
     */
    Buf_Init(&buf, 16);

    paren_depth = 0;
    for (;;) {
	ch = *cp;
	if (ch == 0 || ch == ' ' || ch == '\t')
	    break;
	if ((ch == '&' || ch == '|') && paren_depth == 0)
	    break;
	if (*cp == '$') {
	    /*
	     * Parse the variable spec and install it as part of the argument
	     * if it's valid. We tell Var_Parse to complain on an undefined
	     * variable, so we don't need to do it. Nor do we return an error,
	     * though perhaps we should...
	     */
	    void *freeIt;
	    VarEvalFlags eflags = VARE_UNDEFERR | (doEval ? VARE_WANTRES : 0);
	    const char *cp2 = Var_ParsePP(&cp, VAR_CMD, eflags, &freeIt);
	    Buf_AddStr(&buf, cp2);
	    free(freeIt);
	    continue;
	}
	if (ch == '(')
	    paren_depth++;
	else if (ch == ')' && --paren_depth < 0)
	    break;
	Buf_AddByte(&buf, *cp);
	cp++;
    }

    *out_arg = Buf_GetAll(&buf, &argLen);
    Buf_Destroy(&buf, FALSE);

    while (*cp == ' ' || *cp == '\t') {
	cp++;
    }

    if (func != NULL && *cp++ != ')') {
	Parse_Error(PARSE_WARNING, "Missing closing parenthesis for %s()",
		    func);
	return 0;
    }

    *linePtr = cp;
    return argLen;
}

/* Test whether the given variable is defined. */
static Boolean
CondDoDefined(int argLen MAKE_ATTR_UNUSED, const char *arg)
{
    char *freeIt;
    Boolean result = Var_Value(arg, VAR_CMD, &freeIt) != NULL;
    bmake_free(freeIt);
    return result;
}

/* Wrapper around Str_Match, to be used by Lst_Find. */
static Boolean
CondFindStrMatch(const void *string, const void *pattern)
{
    return Str_Match(string, pattern);
}

/* See if the given target is being made. */
static Boolean
CondDoMake(int argLen MAKE_ATTR_UNUSED, const char *arg)
{
    return Lst_Find(create, CondFindStrMatch, arg) != NULL;
}

/* See if the given file exists. */
static Boolean
CondDoExists(int argLen MAKE_ATTR_UNUSED, const char *arg)
{
    Boolean result;
    char *path;

    path = Dir_FindFile(arg, dirSearchPath);
    if (DEBUG(COND)) {
	fprintf(debug_file, "exists(%s) result is \"%s\"\n",
		arg, path ? path : "");
    }
    if (path != NULL) {
	result = TRUE;
	free(path);
    } else {
	result = FALSE;
    }
    return result;
}

/* See if the given node exists and is an actual target. */
static Boolean
CondDoTarget(int argLen MAKE_ATTR_UNUSED, const char *arg)
{
    GNode *gn;

    gn = Targ_FindNode(arg, TARG_NOCREATE);
    return gn != NULL && !OP_NOP(gn->type);
}

/* See if the given node exists and is an actual target with commands
 * associated with it. */
static Boolean
CondDoCommands(int argLen MAKE_ATTR_UNUSED, const char *arg)
{
    GNode *gn;

    gn = Targ_FindNode(arg, TARG_NOCREATE);
    return gn != NULL && !OP_NOP(gn->type) && !Lst_IsEmpty(gn->commands);
}

/*-
 * Convert the given number into a double.
 * We try a base 10 or 16 integer conversion first, if that fails
 * then we try a floating point conversion instead.
 *
 * Results:
 *	Sets 'value' to double value of string.
 *	Returns TRUE if the conversion succeeded.
 */
static Boolean
CondCvtArg(const char *str, double *value)
{
    char *eptr, ech;
    unsigned long l_val;
    double d_val;

    errno = 0;
    if (!*str) {
	*value = (double)0;
	return TRUE;
    }
    l_val = strtoul(str, &eptr, str[1] == 'x' ? 16 : 10);
    ech = *eptr;
    if (ech == 0 && errno != ERANGE) {
	d_val = str[0] == '-' ? -(double)-l_val : (double)l_val;
    } else {
	if (ech != 0 && ech != '.' && ech != 'e' && ech != 'E')
	    return FALSE;
	d_val = strtod(str, &eptr);
	if (*eptr)
	    return FALSE;
    }

    *value = d_val;
    return TRUE;
}

/*-
 * Parse a string from a variable reference or an optionally quoted
 * string.  This is called for the lhs and rhs of string compares.
 *
 * Results:
 *	Returns the string, absent any quotes, or NULL on error.
 *	Sets quoted if the string was quoted.
 *	Sets freeIt if needed.
 */
/* coverity:[+alloc : arg-*3] */
static const char *
CondGetString(CondLexer *lex, Boolean doEval, Boolean *quoted, void **freeIt,
	      Boolean strictLHS)
{
    Buffer buf;
    const char *str;
    int len;
    Boolean qt;
    const char *start;
    VarEvalFlags eflags;

    Buf_Init(&buf, 0);
    str = NULL;
    *freeIt = NULL;
    *quoted = qt = *lex->condExpr == '"' ? 1 : 0;
    if (qt)
	lex->condExpr++;
    for (start = lex->condExpr; *lex->condExpr && str == NULL;) {
	switch (*lex->condExpr) {
	case '\\':
	    lex->condExpr++;
	    if (lex->condExpr[0] != '\0') {
		Buf_AddByte(&buf, *lex->condExpr);
		lex->condExpr++;
	    }
	    continue;
	case '"':
	    if (qt) {
		lex->condExpr++;	/* we don't want the quotes */
		goto got_str;
	    }
	    Buf_AddByte(&buf, *lex->condExpr); /* likely? */
	    lex->condExpr++;
	    continue;
	case ')':
	case '!':
	case '=':
	case '>':
	case '<':
	case ' ':
	case '\t':
	    if (!qt)
		goto got_str;
	    Buf_AddByte(&buf, *lex->condExpr);
	    lex->condExpr++;
	    continue;
	case '$':
	    /* if we are in quotes, then an undefined variable is ok */
	    eflags = ((!qt && doEval) ? VARE_UNDEFERR : 0) |
		     (doEval ? VARE_WANTRES : 0);
	    str = Var_Parse(lex->condExpr, VAR_CMD, eflags, &len, freeIt);
	    if (str == var_Error) {
		if (*freeIt) {
		    free(*freeIt);
		    *freeIt = NULL;
		}
		/*
		 * Even if !doEval, we still report syntax errors, which
		 * is what getting var_Error back with !doEval means.
		 */
		str = NULL;
		goto cleanup;
	    }
	    lex->condExpr += len;
	    /*
	     * If the '$' was first char (no quotes), and we are
	     * followed by space, the operator or end of expression,
	     * we are done.
	     */
	    if ((lex->condExpr == start + len) &&
		(*lex->condExpr == '\0' ||
		 isspace((unsigned char)*lex->condExpr) ||
		 strchr("!=><)", *lex->condExpr))) {
		goto cleanup;
	    }

	    Buf_AddStr(&buf, str);
	    if (*freeIt) {
		free(*freeIt);
		*freeIt = NULL;
	    }
	    str = NULL;		/* not finished yet */
	    continue;
	default:
	    if (strictLHS && !qt && *start != '$' &&
		!isdigit((unsigned char)*start)) {
		/* lhs must be quoted, a variable reference or number */
		if (*freeIt) {
		    free(*freeIt);
		    *freeIt = NULL;
		}
		str = NULL;
		goto cleanup;
	    }
	    Buf_AddByte(&buf, *lex->condExpr);
	    lex->condExpr++;
	    continue;
	}
    }
got_str:
    *freeIt = Buf_GetAll(&buf, NULL);
    str = *freeIt;
cleanup:
    Buf_Destroy(&buf, FALSE);
    return str;
}

/* The different forms of .if directives. */
static const struct If {
    const char *form;		/* Form of if */
    size_t formlen;		/* Length of form */
    Boolean doNot;		/* TRUE if default function should be negated */
    Boolean (*defProc)(int, const char *); /* Default function to apply */
} ifs[] = {
    { "def",   3, FALSE, CondDoDefined },
    { "ndef",  4, TRUE,  CondDoDefined },
    { "make",  4, FALSE, CondDoMake },
    { "nmake", 5, TRUE,  CondDoMake },
    { "",      0, FALSE, CondDoDefined },
    { NULL,    0, FALSE, NULL }
};

static Token
compare_expression(CondLexer *lex, Boolean doEval)
{
    Token t;
    const char *lhs;
    const char *rhs;
    const char *op;
    void *lhsFree;
    void *rhsFree;
    Boolean lhsQuoted;
    Boolean rhsQuoted;
    double left, right;

    t = TOK_ERROR;
    rhs = NULL;
    lhsFree = rhsFree = NULL;
    lhsQuoted = rhsQuoted = FALSE;

    /*
     * Parse the variable spec and skip over it, saving its
     * value in lhs.
     */
    lhs = CondGetString(lex, doEval, &lhsQuoted, &lhsFree, lhsStrict);
    if (!lhs)
	goto done;

    CondLexer_SkipWhitespace(lex);

    /*
     * Make sure the operator is a valid one. If it isn't a
     * known relational operator, pretend we got a
     * != 0 comparison.
     */
    op = lex->condExpr;
    switch (*lex->condExpr) {
    case '!':
    case '=':
    case '<':
    case '>':
	if (lex->condExpr[1] == '=') {
	    lex->condExpr += 2;
	} else {
	    lex->condExpr += 1;
	}
	break;
    default:
	if (!doEval) {
	    t = TOK_FALSE;
	    goto done;
	}
	/* For .ifxxx "..." check for non-empty string. */
	if (lhsQuoted) {
	    t = lhs[0] != 0;
	    goto done;
	}
	/* For .ifxxx <number> compare against zero */
	if (CondCvtArg(lhs, &left)) {
	    t = left != 0.0;
	    goto done;
	}
	/* For .if ${...} check for non-empty string (defProc is ifdef). */
	if (lex->if_info->form[0] == 0) {
	    t = lhs[0] != 0;
	    goto done;
	}
	/* Otherwise action default test ... */
	t = lex->if_info->defProc(strlen(lhs), lhs) != lex->if_info->doNot;
	goto done;
    }

    CondLexer_SkipWhitespace(lex);

    if (*lex->condExpr == '\0') {
	Parse_Error(PARSE_WARNING,
		    "Missing right-hand-side of operator");
	goto done;
    }

    rhs = CondGetString(lex, doEval, &rhsQuoted, &rhsFree, FALSE);
    if (!rhs)
	goto done;

    if (!doEval) {
	t = TOK_FALSE;
	goto done;
    }

    if (rhsQuoted || lhsQuoted) {
    do_string_compare:
	if (((*op != '!') && (*op != '=')) || (op[1] != '=')) {
	    Parse_Error(PARSE_WARNING,
			"String comparison operator should be either == or !=");
	    goto done;
	}

	if (DEBUG(COND)) {
	    fprintf(debug_file, "lhs = \"%s\", rhs = \"%s\", op = %.2s\n",
		    lhs, rhs, op);
	}
	/*
	 * Null-terminate rhs and perform the comparison.
	 * t is set to the result.
	 */
	if (*op == '=') {
	    t = strcmp(lhs, rhs) == 0;
	} else {
	    t = strcmp(lhs, rhs) != 0;
	}
    } else {
	/*
	 * rhs is either a float or an integer. Convert both the
	 * lhs and the rhs to a double and compare the two.
	 */

	if (!CondCvtArg(lhs, &left) || !CondCvtArg(rhs, &right))
	    goto do_string_compare;

	if (DEBUG(COND)) {
	    fprintf(debug_file, "left = %f, right = %f, op = %.2s\n", left,
		    right, op);
	}
	switch (op[0]) {
	case '!':
	    if (op[1] != '=') {
		Parse_Error(PARSE_WARNING,
			    "Unknown operator");
		goto done;
	    }
	    t = (left != right);
	    break;
	case '=':
	    if (op[1] != '=') {
		Parse_Error(PARSE_WARNING,
			    "Unknown operator");
		goto done;
	    }
	    t = (left == right);
	    break;
	case '<':
	    if (op[1] == '=') {
		t = (left <= right);
	    } else {
		t = (left < right);
	    }
	    break;
	case '>':
	    if (op[1] == '=') {
		t = (left >= right);
	    } else {
		t = (left > right);
	    }
	    break;
	}
    }

done:
    free(lhsFree);
    free(rhsFree);
    return t;
}

static int
ParseEmptyArg(Boolean doEval, const char **linePtr, char **argPtr,
	      const char *func MAKE_ATTR_UNUSED)
{
    void *val_freeIt;
    const char *val;
    int magic_res;

    /* We do all the work here and return the result as the length */
    *argPtr = NULL;

    (*linePtr)--;		/* Make (*linePtr)[1] point to the '('. */
    val = Var_ParsePP(linePtr, VAR_CMD, doEval ? VARE_WANTRES : 0, &val_freeIt);
    /* If successful, *linePtr points beyond the closing ')' now. */

    if (val == var_Error) {
	free(val_freeIt);
	return -1;
    }

    /* A variable is empty when it just contains spaces... 4/15/92, christos */
    while (isspace((unsigned char)val[0]))
	val++;

    /*
     * For consistency with the other functions we can't generate the
     * true/false here.
     */
    magic_res = *val != '\0' ? 2 : 1;
    free(val_freeIt);
    return magic_res;
}

static Boolean
CondDoEmpty(int arglen, const char *arg MAKE_ATTR_UNUSED)
{
    /* Magic values ahead, see ParseEmptyArg. */
    return arglen == 1;
}

static Token
compare_function(CondLexer *lex, Boolean doEval)
{
    static const struct fn_def {
	const char *fn_name;
	size_t fn_name_len;
	int (*fn_getarg)(Boolean, const char **, char **, const char *);
	Boolean (*fn_proc)(int, const char *);
    } fn_defs[] = {
	{ "defined",  7, ParseFuncArg,  CondDoDefined },
	{ "make",     4, ParseFuncArg,  CondDoMake },
	{ "exists",   6, ParseFuncArg,  CondDoExists },
	{ "empty",    5, ParseEmptyArg, CondDoEmpty },
	{ "target",   6, ParseFuncArg,  CondDoTarget },
	{ "commands", 8, ParseFuncArg,  CondDoCommands },
	{ NULL,       0, NULL, NULL },
    };
    const struct fn_def *fn_def;
    Token t;
    char *arg = NULL;
    int arglen;
    const char *cp = lex->condExpr;
    const char *cp1;

    for (fn_def = fn_defs; fn_def->fn_name != NULL; fn_def++) {
	if (!istoken(cp, fn_def->fn_name, fn_def->fn_name_len))
	    continue;
	cp += fn_def->fn_name_len;
	/* There can only be whitespace before the '(' */
	while (isspace((unsigned char)*cp))
	    cp++;
	if (*cp != '(')
	    break;

	arglen = fn_def->fn_getarg(doEval, &cp, &arg, fn_def->fn_name);
	if (arglen <= 0) {
	    lex->condExpr = cp;
	    return arglen < 0 ? TOK_ERROR : TOK_FALSE;
	}
	/* Evaluate the argument using the required function. */
	t = !doEval || fn_def->fn_proc(arglen, arg);
	free(arg);
	lex->condExpr = cp;
	return t;
    }

    /* Push anything numeric through the compare expression */
    cp = lex->condExpr;
    if (isdigit((unsigned char)cp[0]) || strchr("+-", cp[0]))
	return compare_expression(lex, doEval);

    /*
     * Most likely we have a naked token to apply the default function to.
     * However ".if a == b" gets here when the "a" is unquoted and doesn't
     * start with a '$'. This surprises people.
     * If what follows the function argument is a '=' or '!' then the syntax
     * would be invalid if we did "defined(a)" - so instead treat as an
     * expression.
     */
    arglen = ParseFuncArg(doEval, &cp, &arg, NULL);
    for (cp1 = cp; isspace((unsigned char)*cp1); cp1++)
	continue;
    if (*cp1 == '=' || *cp1 == '!')
	return compare_expression(lex, doEval);
    lex->condExpr = cp;

    /*
     * Evaluate the argument using the default function.
     * This path always treats .if as .ifdef. To get here, the character
     * after .if must have been taken literally, so the argument cannot
     * be empty - even if it contained a variable expansion.
     */
    t = !doEval || lex->if_info->defProc(arglen, arg) != lex->if_info->doNot;
    free(arg);
    return t;
}

/* Return the next token or comparison result from the lexer. */
static Token
CondToken(CondLexer *lex, Boolean doEval)
{
    Token t;

    t = lex->curr;
    if (t != TOK_NONE) {
	lex->curr = TOK_NONE;
	return t;
    }

    while (lex->condExpr[0] == ' ' || lex->condExpr[0] == '\t') {
	lex->condExpr++;
    }

    switch (lex->condExpr[0]) {

    case '(':
	lex->condExpr++;
	return TOK_LPAREN;

    case ')':
	lex->condExpr++;
	return TOK_RPAREN;

    case '|':
	lex->condExpr++;
	if (lex->condExpr[0] == '|') {
	    lex->condExpr++;
	}
	return TOK_OR;

    case '&':
	lex->condExpr++;
	if (lex->condExpr[0] == '&') {
	    lex->condExpr++;
	}
	return TOK_AND;

    case '!':
	lex->condExpr++;
	return TOK_NOT;

    case '#':
    case '\n':
    case '\0':
	return TOK_EOF;

    case '"':
    case '$':
	return compare_expression(lex, doEval);

    default:
	return compare_function(lex, doEval);
    }
}

/* Parse a single term in the expression. This consists of a terminal symbol
 * or TOK_NOT and a term (not including the binary operators):
 *
 *	T -> defined(variable) | make(target) | exists(file) | symbol
 *	T -> ! T | ( E )
 *
 * Results:
 *	TOK_TRUE, TOK_FALSE or TOK_ERROR.
 */
static Token
CondT(CondLexer *lex, Boolean doEval)
{
    Token t;

    t = CondToken(lex, doEval);

    if (t == TOK_EOF) {
	/*
	 * If we reached the end of the expression, the expression
	 * is malformed...
	 */
	t = TOK_ERROR;
    } else if (t == TOK_LPAREN) {
	/*
	 * T -> ( E )
	 */
	t = CondE(lex, doEval);
	if (t != TOK_ERROR) {
	    if (CondToken(lex, doEval) != TOK_RPAREN) {
		t = TOK_ERROR;
	    }
	}
    } else if (t == TOK_NOT) {
	t = CondT(lex, doEval);
	if (t == TOK_TRUE) {
	    t = TOK_FALSE;
	} else if (t == TOK_FALSE) {
	    t = TOK_TRUE;
	}
    }
    return t;
}

/* Parse a conjunctive factor (nice name, wot?)
 *
 *	F -> T && F | T
 *
 * Results:
 *	TOK_TRUE, TOK_FALSE or TOK_ERROR
 */
static Token
CondF(CondLexer *lex, Boolean doEval)
{
    Token l, o;

    l = CondT(lex, doEval);
    if (l != TOK_ERROR) {
	o = CondToken(lex, doEval);

	if (o == TOK_AND) {
	    /*
	     * F -> T && F
	     *
	     * If T is TOK_FALSE, the whole thing will be TOK_FALSE, but we
	     * have to parse the r.h.s. anyway (to throw it away).
	     * If T is TOK_TRUE, the result is the r.h.s., be it a TOK_ERROR
	     * or not.
	     */
	    if (l == TOK_TRUE) {
		l = CondF(lex, doEval);
	    } else {
		(void)CondF(lex, FALSE);
	    }
	} else {
	    /*
	     * F -> T
	     */
	    CondLexer_PushBack(lex, o);
	}
    }
    return l;
}

/* Main expression production.
 *
 *	E -> F || E | F
 *
 * Results:
 *	TOK_TRUE, TOK_FALSE or TOK_ERROR.
 */
static Token
CondE(CondLexer *lex, Boolean doEval)
{
    Token l, o;

    l = CondF(lex, doEval);
    if (l != TOK_ERROR) {
	o = CondToken(lex, doEval);

	if (o == TOK_OR) {
	    /*
	     * E -> F || E
	     *
	     * A similar thing occurs for ||, except that here we make sure
	     * the l.h.s. is TOK_FALSE before we bother to evaluate the r.h.s.
	     * Once again, if l is TOK_FALSE, the result is the r.h.s. and once
	     * again if l is TOK_TRUE, we parse the r.h.s. to throw it away.
	     */
	    if (l == TOK_FALSE) {
		l = CondE(lex, doEval);
	    } else {
		(void)CondE(lex, FALSE);
	    }
	} else {
	    /*
	     * E -> F
	     */
	    CondLexer_PushBack(lex, o);
	}
    }
    return l;
}

static CondEvalResult
do_Cond_EvalExpression(CondLexer *lex, Boolean *value)
{

    switch (CondE(lex, TRUE)) {
    case TOK_TRUE:
	if (CondToken(lex, TRUE) == TOK_EOF) {
	    *value = TRUE;
	    return COND_PARSE;
	}
	break;
    case TOK_FALSE:
	if (CondToken(lex, TRUE) == TOK_EOF) {
	    *value = FALSE;
	    return COND_PARSE;
	}
	break;
    default:
    case TOK_ERROR:
	break;
    }

    return COND_INVALID;
}

/* Evaluate the condition in the passed line, including any side effects from
 * the variable expressions in the condition. The condition consists of &&,
 * ||, !, function(arg), comparisons and parenthetical groupings thereof.
 *
 * Results:
 *	COND_PARSE	if the condition was valid grammatically
 *	COND_INVALID  	if not a valid conditional.
 *
 *	(*value) is set to the boolean value of the condition
 */
CondEvalResult
Cond_EvalExpression(const struct If *info, const char *line, Boolean *value,
		    int eprint, Boolean strictLHS)
{
    static const struct If *dflt_info;
    CondLexer lex;
    int rval;

    lhsStrict = strictLHS;

    while (*line == ' ' || *line == '\t')
	line++;

    if (info == NULL && (info = dflt_info) == NULL) {
	/* Scan for the entry for .if - it can't be first */
	for (info = ifs;; info++)
	    if (info->form[0] == 0)
		break;
	dflt_info = info;
    }
    assert(info != NULL);

    lex.if_info = info;
    lex.condExpr = line;
    lex.curr = TOK_NONE;

    rval = do_Cond_EvalExpression(&lex, value);

    if (rval == COND_INVALID && eprint)
	Parse_Error(PARSE_FATAL, "Malformed conditional (%s)", line);

    return rval;
}


/* Evaluate the conditional in the passed line. The line looks like this:
 *	.<cond-type> <expr>
 * In this line, <cond-type> is any of if, ifmake, ifnmake, ifdef, ifndef,
 * elif, elifmake, elifnmake, elifdef, elifndef.
 * In this line, <expr> consists of &&, ||, !, function(arg), comparisons
 * and parenthetical groupings thereof.
 *
 * Note that the states IF_ACTIVE and ELSE_ACTIVE are only different in order
 * to detect spurious .else lines (as are SKIP_TO_ELSE and SKIP_TO_ENDIF),
 * otherwise .else could be treated as '.elif 1'.
 *
 * Results:
 *	COND_PARSE	to continue parsing the lines after the conditional
 *			(when .if or .else returns TRUE)
 *	COND_SKIP	to skip the lines after the conditional
 *			(when .if or .elif returns FALSE, or when a previous
 *			branch has already been taken)
 *	COND_INVALID  	if the conditional was not valid, either because of
 *			a syntax error or because some variable was undefined
 *			or because the condition could not be evaluated
 */
CondEvalResult
Cond_Eval(const char *line)
{
    enum { MAXIF = 128 };	/* maximum depth of .if'ing */
    enum { MAXIF_BUMP = 32 };	/* how much to grow by */
    enum if_states {
	IF_ACTIVE,		/* .if or .elif part active */
	ELSE_ACTIVE,		/* .else part active */
	SEARCH_FOR_ELIF,	/* searching for .elif/else to execute */
	SKIP_TO_ELSE,		/* has been true, but not seen '.else' */
	SKIP_TO_ENDIF		/* nothing else to execute */
    };
    static enum if_states *cond_state = NULL;
    static unsigned int max_if_depth = MAXIF;

    const struct If *ifp;
    Boolean isElif;
    Boolean value;
    int level;			/* Level at which to report errors. */
    enum if_states state;

    level = PARSE_FATAL;
    if (!cond_state) {
	cond_state = bmake_malloc(max_if_depth * sizeof(*cond_state));
	cond_state[0] = IF_ACTIVE;
    }
    /* skip leading character (the '.') and any whitespace */
    for (line++; *line == ' ' || *line == '\t'; line++)
	continue;

    /* Find what type of if we're dealing with.  */
    if (line[0] == 'e') {
	if (line[1] != 'l') {
	    if (!istoken(line + 1, "ndif", 4))
		return COND_INVALID;
	    /* End of conditional section */
	    if (cond_depth == cond_min_depth) {
		Parse_Error(level, "if-less endif");
		return COND_PARSE;
	    }
	    /* Return state for previous conditional */
	    cond_depth--;
	    return cond_state[cond_depth] <= ELSE_ACTIVE
		   ? COND_PARSE : COND_SKIP;
	}

	/* Quite likely this is 'else' or 'elif' */
	line += 2;
	if (istoken(line, "se", 2)) {
	    /* It is else... */
	    if (cond_depth == cond_min_depth) {
		Parse_Error(level, "if-less else");
		return COND_PARSE;
	    }

	    state = cond_state[cond_depth];
	    switch (state) {
	    case SEARCH_FOR_ELIF:
		state = ELSE_ACTIVE;
		break;
	    case ELSE_ACTIVE:
	    case SKIP_TO_ENDIF:
		Parse_Error(PARSE_WARNING, "extra else");
		/* FALLTHROUGH */
	    default:
	    case IF_ACTIVE:
	    case SKIP_TO_ELSE:
		state = SKIP_TO_ENDIF;
		break;
	    }
	    cond_state[cond_depth] = state;
	    return state <= ELSE_ACTIVE ? COND_PARSE : COND_SKIP;
	}
	/* Assume for now it is an elif */
	isElif = TRUE;
    } else
	isElif = FALSE;

    if (line[0] != 'i' || line[1] != 'f')
	/* Not an ifxxx or elifxxx line */
	return COND_INVALID;

    /*
     * Figure out what sort of conditional it is -- what its default
     * function is, etc. -- by looking in the table of valid "ifs"
     */
    line += 2;
    for (ifp = ifs;; ifp++) {
	if (ifp->form == NULL)
	    return COND_INVALID;
	if (istoken(ifp->form, line, ifp->formlen)) {
	    line += ifp->formlen;
	    break;
	}
    }

    /* Now we know what sort of 'if' it is... */

    if (isElif) {
	if (cond_depth == cond_min_depth) {
	    Parse_Error(level, "if-less elif");
	    return COND_PARSE;
	}
	state = cond_state[cond_depth];
	if (state == SKIP_TO_ENDIF || state == ELSE_ACTIVE) {
	    Parse_Error(PARSE_WARNING, "extra elif");
	    cond_state[cond_depth] = SKIP_TO_ENDIF;
	    return COND_SKIP;
	}
	if (state != SEARCH_FOR_ELIF) {
	    /* Either just finished the 'true' block, or already SKIP_TO_ELSE */
	    cond_state[cond_depth] = SKIP_TO_ELSE;
	    return COND_SKIP;
	}
    } else {
	/* Normal .if */
	if (cond_depth + 1 >= max_if_depth) {
	    /*
	     * This is rare, but not impossible.
	     * In meta mode, dirdeps.mk (only runs at level 0)
	     * can need more than the default.
	     */
	    max_if_depth += MAXIF_BUMP;
	    cond_state = bmake_realloc(cond_state,
				       max_if_depth * sizeof(*cond_state));
	}
	state = cond_state[cond_depth];
	cond_depth++;
	if (state > ELSE_ACTIVE) {
	    /* If we aren't parsing the data, treat as always false */
	    cond_state[cond_depth] = SKIP_TO_ELSE;
	    return COND_SKIP;
	}
    }

    /* And evaluate the conditional expression */
    if (Cond_EvalExpression(ifp, line, &value, 1, TRUE) == COND_INVALID) {
	/* Syntax error in conditional, error message already output. */
	/* Skip everything to matching .endif */
	cond_state[cond_depth] = SKIP_TO_ELSE;
	return COND_SKIP;
    }

    if (!value) {
	cond_state[cond_depth] = SEARCH_FOR_ELIF;
	return COND_SKIP;
    }
    cond_state[cond_depth] = IF_ACTIVE;
    return COND_PARSE;
}

void
Cond_restore_depth(unsigned int saved_depth)
{
    int open_conds = cond_depth - cond_min_depth;

    if (open_conds != 0 || saved_depth > cond_depth) {
	Parse_Error(PARSE_FATAL, "%d open conditional%s", open_conds,
		    open_conds == 1 ? "" : "s");
	cond_depth = cond_min_depth;
    }

    cond_min_depth = saved_depth;
}

unsigned int
Cond_save_depth(void)
{
    int depth = cond_min_depth;

    cond_min_depth = cond_depth;
    return depth;
}
