/*	$NetBSD: cond.c,v 1.38 2008/02/06 18:26:37 joerg Exp $	*/

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
static char rcsid[] = "$NetBSD: cond.c,v 1.38 2008/02/06 18:26:37 joerg Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)cond.c	8.2 (Berkeley) 1/2/94";
#else
__RCSID("$NetBSD: cond.c,v 1.38 2008/02/06 18:26:37 joerg Exp $");
#endif
#endif /* not lint */
#endif

/*-
 * cond.c --
 *	Functions to handle conditionals in a makefile.
 *
 * Interface:
 *	Cond_Eval 	Evaluate the conditional in the passed line.
 *
 */

#include    <ctype.h>

#include    "make.h"
#include    "hash.h"
#include    "dir.h"
#include    "buf.h"

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
 * 'symbol' is some other symbol to which the default function (condDefProc)
 * is applied.
 *
 * Tokens are scanned from the 'condExpr' string. The scanner (CondToken)
 * will return And for '&' and '&&', Or for '|' and '||', Not for '!',
 * LParen for '(', RParen for ')' and will evaluate the other terminal
 * symbols, using either the default function or the function given in the
 * terminal, and return the result as either True or False.
 *
 * All Non-Terminal functions (CondE, CondF and CondT) return Err on error.
 */
typedef enum {
    And, Or, Not, True, False, LParen, RParen, EndOfFile, None, Err
} Token;

/*-
 * Structures to handle elegantly the different forms of #if's. The
 * last two fields are stored in condInvert and condDefProc, respectively.
 */
static void CondPushBack(Token);
static int CondGetArg(char **, char **, const char *, Boolean);
static Boolean CondDoDefined(int, char *);
static int CondStrMatch(ClientData, ClientData);
static Boolean CondDoMake(int, char *);
static Boolean CondDoExists(int, char *);
static Boolean CondDoTarget(int, char *);
static Boolean CondDoCommands(int, char *);
static char * CondCvtArg(char *, double *);
static Token CondToken(Boolean);
static Token CondT(Boolean);
static Token CondF(Boolean);
static Token CondE(Boolean);

static const struct If {
    const char	*form;	      /* Form of if */
    int		formlen;      /* Length of form */
    Boolean	doNot;	      /* TRUE if default function should be negated */
    Boolean	(*defProc)(int, char *); /* Default function to apply */
} ifs[] = {
    { "def",	  3,	  FALSE,  CondDoDefined },
    { "ndef",	  4,	  TRUE,	  CondDoDefined },
    { "make",	  4,	  FALSE,  CondDoMake },
    { "nmake",	  5,	  TRUE,	  CondDoMake },
    { "",	  0,	  FALSE,  CondDoDefined },
    { NULL,	  0,	  FALSE,  NULL }
};

static Boolean	  condInvert;	    	/* Invert the default function */
static Boolean	  (*condDefProc)(int, char *);	/* Default function to apply */
static char 	  *condExpr;	    	/* The expression to parse */
static Token	  condPushBack=None;	/* Single push-back token used in
					 * parsing */

static unsigned int	cond_depth = 0;  	/* current .if nesting level */
static unsigned int	cond_min_depth = 0;  	/* depth at makefile open */

static int
istoken(const char *str, const char *tok, size_t len)
{
	return strncmp(str, tok, len) == 0 && !isalpha((unsigned char)str[len]);
}

/*-
 *-----------------------------------------------------------------------
 * CondPushBack --
 *	Push back the most recent token read. We only need one level of
 *	this, so the thing is just stored in 'condPushback'.
 *
 * Input:
 *	t		Token to push back into the "stream"
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	condPushback is overwritten.
 *
 *-----------------------------------------------------------------------
 */
static void
CondPushBack(Token t)
{
    condPushBack = t;
}

/*-
 *-----------------------------------------------------------------------
 * CondGetArg --
 *	Find the argument of a built-in function.
 *
 * Input:
 *	parens		TRUE if arg should be bounded by parens
 *
 * Results:
 *	The length of the argument and the address of the argument.
 *
 * Side Effects:
 *	The pointer is set to point to the closing parenthesis of the
 *	function call.
 *
 *-----------------------------------------------------------------------
 */
static int
CondGetArg(char **linePtr, char **argPtr, const char *func, Boolean parens)
{
    char	  *cp;
    int	    	  argLen;
    Buffer	  buf;

    cp = *linePtr;
    if (parens) {
	while (*cp != '(' && *cp != '\0') {
	    cp++;
	}
	if (*cp == '(') {
	    cp++;
	}
    }

    if (*cp == '\0') {
	/*
	 * No arguments whatsoever. Because 'make' and 'defined' aren't really
	 * "reserved words", we don't print a message. I think this is better
	 * than hitting the user with a warning message every time s/he uses
	 * the word 'make' or 'defined' at the beginning of a symbol...
	 */
	*argPtr = NULL;
	return (0);
    }

    while (*cp == ' ' || *cp == '\t') {
	cp++;
    }

    /*
     * Create a buffer for the argument and start it out at 16 characters
     * long. Why 16? Why not?
     */
    buf = Buf_Init(16);

    while ((strchr(" \t)&|", *cp) == NULL) && (*cp != '\0')) {
	if (*cp == '$') {
	    /*
	     * Parse the variable spec and install it as part of the argument
	     * if it's valid. We tell Var_Parse to complain on an undefined
	     * variable, so we don't do it too. Nor do we return an error,
	     * though perhaps we should...
	     */
	    char  	*cp2;
	    int		len;
	    void	*freeIt;

	    cp2 = Var_Parse(cp, VAR_CMD, TRUE, &len, &freeIt);
	    Buf_AddBytes(buf, strlen(cp2), (Byte *)cp2);
	    if (freeIt)
		free(freeIt);
	    cp += len;
	} else {
	    Buf_AddByte(buf, (Byte)*cp);
	    cp++;
	}
    }

    Buf_AddByte(buf, (Byte)'\0');
    *argPtr = (char *)Buf_GetAll(buf, &argLen);
    Buf_Destroy(buf, FALSE);

    while (*cp == ' ' || *cp == '\t') {
	cp++;
    }
    if (parens && *cp != ')') {
	Parse_Error(PARSE_WARNING, "Missing closing parenthesis for %s()",
		     func);
	return (0);
    } else if (parens) {
	/*
	 * Advance pointer past close parenthesis.
	 */
	cp++;
    }

    *linePtr = cp;
    return (argLen);
}

/*-
 *-----------------------------------------------------------------------
 * CondDoDefined --
 *	Handle the 'defined' function for conditionals.
 *
 * Results:
 *	TRUE if the given variable is defined.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static Boolean
CondDoDefined(int argLen, char *arg)
{
    char    savec = arg[argLen];
    char    *p1;
    Boolean result;

    arg[argLen] = '\0';
    if (Var_Value(arg, VAR_CMD, &p1) != NULL) {
	result = TRUE;
    } else {
	result = FALSE;
    }
    if (p1)
	free(p1);
    arg[argLen] = savec;
    return (result);
}

/*-
 *-----------------------------------------------------------------------
 * CondStrMatch --
 *	Front-end for Str_Match so it returns 0 on match and non-zero
 *	on mismatch. Callback function for CondDoMake via Lst_Find
 *
 * Results:
 *	0 if string matches pattern
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
static int
CondStrMatch(ClientData string, ClientData pattern)
{
    return(!Str_Match((char *)string,(char *)pattern));
}

/*-
 *-----------------------------------------------------------------------
 * CondDoMake --
 *	Handle the 'make' function for conditionals.
 *
 * Results:
 *	TRUE if the given target is being made.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static Boolean
CondDoMake(int argLen, char *arg)
{
    char    savec = arg[argLen];
    Boolean result;

    arg[argLen] = '\0';
    if (Lst_Find(create, arg, CondStrMatch) == NILLNODE) {
	result = FALSE;
    } else {
	result = TRUE;
    }
    arg[argLen] = savec;
    return (result);
}

/*-
 *-----------------------------------------------------------------------
 * CondDoExists --
 *	See if the given file exists.
 *
 * Results:
 *	TRUE if the file exists and FALSE if it does not.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static Boolean
CondDoExists(int argLen, char *arg)
{
    char    savec = arg[argLen];
    Boolean result;
    char    *path;

    arg[argLen] = '\0';
    path = Dir_FindFile(arg, dirSearchPath);
    if (path != NULL) {
	result = TRUE;
	free(path);
    } else {
	result = FALSE;
    }
    arg[argLen] = savec;
    if (DEBUG(COND)) {
	fprintf(debug_file, "exists(%s) result is \"%s\"\n",
	       arg, path ? path : "");
    }    
    return (result);
}

/*-
 *-----------------------------------------------------------------------
 * CondDoTarget --
 *	See if the given node exists and is an actual target.
 *
 * Results:
 *	TRUE if the node exists as a target and FALSE if it does not.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static Boolean
CondDoTarget(int argLen, char *arg)
{
    char    savec = arg[argLen];
    Boolean result;
    GNode   *gn;

    arg[argLen] = '\0';
    gn = Targ_FindNode(arg, TARG_NOCREATE);
    if ((gn != NILGNODE) && !OP_NOP(gn->type)) {
	result = TRUE;
    } else {
	result = FALSE;
    }
    arg[argLen] = savec;
    return (result);
}

/*-
 *-----------------------------------------------------------------------
 * CondDoCommands --
 *	See if the given node exists and is an actual target with commands
 *	associated with it.
 *
 * Results:
 *	TRUE if the node exists as a target and has commands associated with
 *	it and FALSE if it does not.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static Boolean
CondDoCommands(int argLen, char *arg)
{
    char    savec = arg[argLen];
    Boolean result;
    GNode   *gn;

    arg[argLen] = '\0';
    gn = Targ_FindNode(arg, TARG_NOCREATE);
    if ((gn != NILGNODE) && !OP_NOP(gn->type) && !Lst_IsEmpty(gn->commands)) {
	result = TRUE;
    } else {
	result = FALSE;
    }
    arg[argLen] = savec;
    return (result);
}

/*-
 *-----------------------------------------------------------------------
 * CondCvtArg --
 *	Convert the given number into a double. If the number begins
 *	with 0x, it is interpreted as a hexadecimal integer
 *	and converted to a double from there. All other strings just have
 *	strtod called on them.
 *
 * Results:
 *	Sets 'value' to double value of string.
 *	Returns NULL if string was fully consumed,
 *	else returns remaining input.
 *
 * Side Effects:
 *	Can change 'value' even if string is not a valid number.
 *
 *
 *-----------------------------------------------------------------------
 */
static char *
CondCvtArg(char *str, double *value)
{
    if ((*str == '0') && (str[1] == 'x')) {
	long i;

	for (str += 2, i = 0; *str; str++) {
	    int x;
	    if (isdigit((unsigned char) *str))
		x  = *str - '0';
	    else if (isxdigit((unsigned char) *str))
		x = 10 + *str - isupper((unsigned char) *str) ? 'A' : 'a';
	    else
		break;
	    i = (i << 4) + x;
	}
	*value = (double) i;
	return *str ? str : NULL;
    } else {
	char *eptr;
	*value = strtod(str, &eptr);
	return *eptr ? eptr : NULL;
    }
}

/*-
 *-----------------------------------------------------------------------
 * CondGetString --
 *	Get a string from a variable reference or an optionally quoted
 *	string.  This is called for the lhs and rhs of string compares.
 *
 * Results:
 *	Sets freeIt if needed,
 *	Sets quoted if string was quoted,
 *	Returns NULL on error,
 *	else returns string - absent any quotes.
 *
 * Side Effects:
 *	Moves condExpr to end of this token.
 *
 *
 *-----------------------------------------------------------------------
 */
/* coverity:[+alloc : arg-*2] */
static char *
CondGetString(Boolean doEval, Boolean *quoted, void **freeIt)
{
    Buffer buf;
    char *cp;
    char *str;
    int	len;
    int qt;
    char *start;

    buf = Buf_Init(0);
    str = NULL;
    *freeIt = NULL;
    *quoted = qt = *condExpr == '"' ? 1 : 0;
    if (qt)
	condExpr++;
    for (start = condExpr; *condExpr && str == NULL; condExpr++) {
	switch (*condExpr) {
	case '\\':
	    if (condExpr[1] != '\0') {
		condExpr++;
		Buf_AddByte(buf, (Byte)*condExpr);
	    }
	    break;
	case '"':
	    if (qt) {
		condExpr++;		/* we don't want the quotes */
		goto got_str;
	    } else
		Buf_AddByte(buf, (Byte)*condExpr); /* likely? */
	    break;
	case ')':
	case '!':
	case '=':
	case '>':
	case '<':
	case ' ':
	case '\t':
	    if (!qt)
		goto got_str;
	    else
		Buf_AddByte(buf, (Byte)*condExpr);
	    break;
	case '$':
	    /* if we are in quotes, then an undefined variable is ok */
	    str = Var_Parse(condExpr, VAR_CMD, (qt ? 0 : doEval),
			    &len, freeIt);
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
	    condExpr += len;
	    /*
	     * If the '$' was first char (no quotes), and we are
	     * followed by space, the operator or end of expression,
	     * we are done.
	     */
	    if ((condExpr == start + len) &&
		(*condExpr == '\0' ||
		 isspace((unsigned char) *condExpr) ||
		 strchr("!=><)", *condExpr))) {
		goto cleanup;
	    }
	    /*
	     * Nope, we better copy str to buf
	     */
	    for (cp = str; *cp; cp++) {
		Buf_AddByte(buf, (Byte)*cp);
	    }
	    if (*freeIt) {
		free(*freeIt);
		*freeIt = NULL;
	    }
	    str = NULL;			/* not finished yet */
	    condExpr--;			/* don't skip over next char */
	    break;
	default:
	    Buf_AddByte(buf, (Byte)*condExpr);
	    break;
	}
    }
 got_str:
    Buf_AddByte(buf, (Byte)'\0');
    str = (char *)Buf_GetAll(buf, NULL);
    *freeIt = str;
 cleanup:
    Buf_Destroy(buf, FALSE);
    return str;
}

/*-
 *-----------------------------------------------------------------------
 * CondToken --
 *	Return the next token from the input.
 *
 * Results:
 *	A Token for the next lexical token in the stream.
 *
 * Side Effects:
 *	condPushback will be set back to None if it is used.
 *
 *-----------------------------------------------------------------------
 */
static Token
CondToken(Boolean doEval)
{
    Token	  t;

    if (condPushBack == None) {
	while (*condExpr == ' ' || *condExpr == '\t') {
	    condExpr++;
	}
	switch (*condExpr) {
	    case '(':
		t = LParen;
		condExpr++;
		break;
	    case ')':
		t = RParen;
		condExpr++;
		break;
	    case '|':
		if (condExpr[1] == '|') {
		    condExpr++;
		}
		condExpr++;
		t = Or;
		break;
	    case '&':
		if (condExpr[1] == '&') {
		    condExpr++;
		}
		condExpr++;
		t = And;
		break;
	    case '!':
		t = Not;
		condExpr++;
		break;
	    case '#':
	    case '\n':
	    case '\0':
		t = EndOfFile;
		break;
	    case '"':
	    case '$': {
		char	*lhs;
		char	*rhs;
		char	*op;
		void	*lhsFree;
		void	*rhsFree;
		Boolean lhsQuoted;
		Boolean rhsQuoted;

		rhs = NULL;
		lhsFree = rhsFree = FALSE;
		lhsQuoted = rhsQuoted = FALSE;
		
		/*
		 * Parse the variable spec and skip over it, saving its
		 * value in lhs.
		 */
		t = Err;
		lhs = CondGetString(doEval, &lhsQuoted, &lhsFree);
		if (!lhs) {
		    if (lhsFree)
			free(lhsFree);
		    return Err;
		}
		/*
		 * Skip whitespace to get to the operator
		 */
		while (isspace((unsigned char) *condExpr))
		    condExpr++;

		/*
		 * Make sure the operator is a valid one. If it isn't a
		 * known relational operator, pretend we got a
		 * != 0 comparison.
		 */
		op = condExpr;
		switch (*condExpr) {
		    case '!':
		    case '=':
		    case '<':
		    case '>':
			if (condExpr[1] == '=') {
			    condExpr += 2;
			} else {
			    condExpr += 1;
			}
			break;
		    default:
			op = UNCONST("!=");
			if (lhsQuoted)
			    rhs = UNCONST("");
			else
			    rhs = UNCONST("0");

			goto do_compare;
		}
		while (isspace((unsigned char) *condExpr)) {
		    condExpr++;
		}
		if (*condExpr == '\0') {
		    Parse_Error(PARSE_WARNING,
				"Missing right-hand-side of operator");
		    goto error;
		}
		rhs = CondGetString(doEval, &rhsQuoted, &rhsFree);
		if (!rhs) {
		    if (lhsFree)
			free(lhsFree);
		    if (rhsFree)
			free(rhsFree);
		    return Err;
		}
do_compare:
		if (rhsQuoted || lhsQuoted) {
do_string_compare:
		    if (((*op != '!') && (*op != '=')) || (op[1] != '=')) {
			Parse_Error(PARSE_WARNING,
		"String comparison operator should be either == or !=");
			goto error;
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
			t = strcmp(lhs, rhs) ? False : True;
		    } else {
			t = strcmp(lhs, rhs) ? True : False;
		    }
		} else {
		    /*
		     * rhs is either a float or an integer. Convert both the
		     * lhs and the rhs to a double and compare the two.
		     */
		    double  	left, right;
		    char	*cp;
		
		    if (CondCvtArg(lhs, &left))
			goto do_string_compare;
		    if ((cp = CondCvtArg(rhs, &right)) &&
			    cp == rhs)
			goto do_string_compare;

		    if (DEBUG(COND)) {
			fprintf(debug_file, "left = %f, right = %f, op = %.2s\n", left,
			       right, op);
		    }
		    switch(op[0]) {
		    case '!':
			if (op[1] != '=') {
			    Parse_Error(PARSE_WARNING,
					"Unknown operator");
			    goto error;
			}
			t = (left != right ? True : False);
			break;
		    case '=':
			if (op[1] != '=') {
			    Parse_Error(PARSE_WARNING,
					"Unknown operator");
			    goto error;
			}
			t = (left == right ? True : False);
			break;
		    case '<':
			if (op[1] == '=') {
			    t = (left <= right ? True : False);
			} else {
			    t = (left < right ? True : False);
			}
			break;
		    case '>':
			if (op[1] == '=') {
			    t = (left >= right ? True : False);
			} else {
			    t = (left > right ? True : False);
			}
			break;
		    }
		}
error:
		if (lhsFree)
		    free(lhsFree);
		if (rhsFree)
		    free(rhsFree);
		break;
	    }
	    default: {
		Boolean (*evalProc)(int, char *);
		Boolean invert = FALSE;
		char	*arg = NULL;
		int	arglen = 0;

		if (istoken(condExpr, "defined", 7)) {
		    /*
		     * Use CondDoDefined to evaluate the argument and
		     * CondGetArg to extract the argument from the 'function
		     * call'.
		     */
		    evalProc = CondDoDefined;
		    condExpr += 7;
		    arglen = CondGetArg(&condExpr, &arg, "defined", TRUE);
		    if (arglen == 0) {
			condExpr -= 7;
			goto use_default;
		    }
		} else if (istoken(condExpr, "make", 4)) {
		    /*
		     * Use CondDoMake to evaluate the argument and
		     * CondGetArg to extract the argument from the 'function
		     * call'.
		     */
		    evalProc = CondDoMake;
		    condExpr += 4;
		    arglen = CondGetArg(&condExpr, &arg, "make", TRUE);
		    if (arglen == 0) {
			condExpr -= 4;
			goto use_default;
		    }
		} else if (istoken(condExpr, "exists", 6)) {
		    /*
		     * Use CondDoExists to evaluate the argument and
		     * CondGetArg to extract the argument from the
		     * 'function call'.
		     */
		    evalProc = CondDoExists;
		    condExpr += 6;
		    arglen = CondGetArg(&condExpr, &arg, "exists", TRUE);
		    if (arglen == 0) {
			condExpr -= 6;
			goto use_default;
		    }
		} else if (istoken(condExpr, "empty", 5)) {
		    /*
		     * Use Var_Parse to parse the spec in parens and return
		     * True if the resulting string is empty.
		     */
		    int	    length;
		    void    *freeIt;
		    char    *val;

		    condExpr += 5;

		    for (arglen = 0; condExpr[arglen] != '\0'; arglen += 1) {
			if (condExpr[arglen] == '(')
			    break;
			if (!isspace((unsigned char)condExpr[arglen]))
			    Parse_Error(PARSE_WARNING,
				"Extra characters after \"empty\"");
		    }

		    if (condExpr[arglen] != '\0') {
			val = Var_Parse(&condExpr[arglen - 1], VAR_CMD,
					FALSE, &length, &freeIt);
			if (val == var_Error) {
			    t = Err;
			} else {
			    /*
			     * A variable is empty when it just contains
			     * spaces... 4/15/92, christos
			     */
			    char *p;
			    for (p = val; *p && isspace((unsigned char)*p); p++)
				continue;
			    t = (*p == '\0') ? True : False;
			}
			if (freeIt) {
			    free(freeIt);
			}
			/*
			 * Advance condExpr to beyond the closing ). Note that
			 * we subtract one from arglen + length b/c length
			 * is calculated from condExpr[arglen - 1].
			 */
			condExpr += arglen + length - 1;
		    } else {
			condExpr -= 5;
			goto use_default;
		    }
		    break;
		} else if (istoken(condExpr, "target", 6)) {
		    /*
		     * Use CondDoTarget to evaluate the argument and
		     * CondGetArg to extract the argument from the
		     * 'function call'.
		     */
		    evalProc = CondDoTarget;
		    condExpr += 6;
		    arglen = CondGetArg(&condExpr, &arg, "target", TRUE);
		    if (arglen == 0) {
			condExpr -= 6;
			goto use_default;
		    }
		} else if (istoken(condExpr, "commands", 8)) {
		    /*
		     * Use CondDoCommands to evaluate the argument and
		     * CondGetArg to extract the argument from the
		     * 'function call'.
		     */
		    evalProc = CondDoCommands;
		    condExpr += 8;
		    arglen = CondGetArg(&condExpr, &arg, "commands", TRUE);
		    if (arglen == 0) {
			condExpr -= 8;
			goto use_default;
		    }
		} else {
		    /*
		     * The symbol is itself the argument to the default
		     * function. We advance condExpr to the end of the symbol
		     * by hand (the next whitespace, closing paren or
		     * binary operator) and set to invert the evaluation
		     * function if condInvert is TRUE.
		     */
		use_default:
		    invert = condInvert;
		    evalProc = condDefProc;
		    arglen = CondGetArg(&condExpr, &arg, "", FALSE);
		}

		/*
		 * Evaluate the argument using the set function. If invert
		 * is TRUE, we invert the sense of the function.
		 */
		t = (!doEval || (* evalProc) (arglen, arg) ?
		     (invert ? False : True) :
		     (invert ? True : False));
		if (arg)
		    free(arg);
		break;
	    }
	}
    } else {
	t = condPushBack;
	condPushBack = None;
    }
    return (t);
}

/*-
 *-----------------------------------------------------------------------
 * CondT --
 *	Parse a single term in the expression. This consists of a terminal
 *	symbol or Not and a terminal symbol (not including the binary
 *	operators):
 *	    T -> defined(variable) | make(target) | exists(file) | symbol
 *	    T -> ! T | ( E )
 *
 * Results:
 *	True, False or Err.
 *
 * Side Effects:
 *	Tokens are consumed.
 *
 *-----------------------------------------------------------------------
 */
static Token
CondT(Boolean doEval)
{
    Token   t;

    t = CondToken(doEval);

    if (t == EndOfFile) {
	/*
	 * If we reached the end of the expression, the expression
	 * is malformed...
	 */
	t = Err;
    } else if (t == LParen) {
	/*
	 * T -> ( E )
	 */
	t = CondE(doEval);
	if (t != Err) {
	    if (CondToken(doEval) != RParen) {
		t = Err;
	    }
	}
    } else if (t == Not) {
	t = CondT(doEval);
	if (t == True) {
	    t = False;
	} else if (t == False) {
	    t = True;
	}
    }
    return (t);
}

/*-
 *-----------------------------------------------------------------------
 * CondF --
 *	Parse a conjunctive factor (nice name, wot?)
 *	    F -> T && F | T
 *
 * Results:
 *	True, False or Err
 *
 * Side Effects:
 *	Tokens are consumed.
 *
 *-----------------------------------------------------------------------
 */
static Token
CondF(Boolean doEval)
{
    Token   l, o;

    l = CondT(doEval);
    if (l != Err) {
	o = CondToken(doEval);

	if (o == And) {
	    /*
	     * F -> T && F
	     *
	     * If T is False, the whole thing will be False, but we have to
	     * parse the r.h.s. anyway (to throw it away).
	     * If T is True, the result is the r.h.s., be it an Err or no.
	     */
	    if (l == True) {
		l = CondF(doEval);
	    } else {
		(void)CondF(FALSE);
	    }
	} else {
	    /*
	     * F -> T
	     */
	    CondPushBack(o);
	}
    }
    return (l);
}

/*-
 *-----------------------------------------------------------------------
 * CondE --
 *	Main expression production.
 *	    E -> F || E | F
 *
 * Results:
 *	True, False or Err.
 *
 * Side Effects:
 *	Tokens are, of course, consumed.
 *
 *-----------------------------------------------------------------------
 */
static Token
CondE(Boolean doEval)
{
    Token   l, o;

    l = CondF(doEval);
    if (l != Err) {
	o = CondToken(doEval);

	if (o == Or) {
	    /*
	     * E -> F || E
	     *
	     * A similar thing occurs for ||, except that here we make sure
	     * the l.h.s. is False before we bother to evaluate the r.h.s.
	     * Once again, if l is False, the result is the r.h.s. and once
	     * again if l is True, we parse the r.h.s. to throw it away.
	     */
	    if (l == False) {
		l = CondE(doEval);
	    } else {
		(void)CondE(FALSE);
	    }
	} else {
	    /*
	     * E -> F
	     */
	    CondPushBack(o);
	}
    }
    return (l);
}

/*-
 *-----------------------------------------------------------------------
 * Cond_EvalExpression --
 *	Evaluate an expression in the passed line. The expression
 *	consists of &&, ||, !, make(target), defined(variable)
 *	and parenthetical groupings thereof.
 *
 * Results:
 *	COND_PARSE	if the condition was valid grammatically
 *	COND_INVALID  	if not a valid conditional.
 *
 *	(*value) is set to the boolean value of the condition
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
Cond_EvalExpression(int dosetup, char *line, Boolean *value, int eprint)
{
    if (dosetup) {
	condDefProc = CondDoDefined;
	condInvert = 0;
    }

    while (*line == ' ' || *line == '\t')
	line++;

    condExpr = line;
    condPushBack = None;

    switch (CondE(TRUE)) {
    case True:
	if (CondToken(TRUE) == EndOfFile) {
	    *value = TRUE;
	    break;
	}
	goto err;
	/*FALLTHRU*/
    case False:
	if (CondToken(TRUE) == EndOfFile) {
	    *value = FALSE;
	    break;
	}
	/*FALLTHRU*/
    case Err:
err:
	if (eprint)
	    Parse_Error(PARSE_FATAL, "Malformed conditional (%s)",
			 line);
	return (COND_INVALID);
    default:
	break;
    }

    return COND_PARSE;
}


/*-
 *-----------------------------------------------------------------------
 * Cond_Eval --
 *	Evaluate the conditional in the passed line. The line
 *	looks like this:
 *	    .<cond-type> <expr>
 *	where <cond-type> is any of if, ifmake, ifnmake, ifdef,
 *	ifndef, elif, elifmake, elifnmake, elifdef, elifndef
 *	and <expr> consists of &&, ||, !, make(target), defined(variable)
 *	and parenthetical groupings thereof.
 *
 * Input:
 *	line		Line to parse
 *
 * Results:
 *	COND_PARSE	if should parse lines after the conditional
 *	COND_SKIP	if should skip lines after the conditional
 *	COND_INVALID  	if not a valid conditional.
 *
 * Side Effects:
 *	None.
 *
 * Note that the states IF_ACTIVE and ELSE_ACTIVE are only different in order
 * to detect splurious .else lines (as are SKIP_TO_ELSE and SKIP_TO_ENDIF)
 * otherwise .else could be treated as '.elif 1'.
 *
 *-----------------------------------------------------------------------
 */
int
Cond_Eval(char *line)
{
    #define	    MAXIF	64	/* maximum depth of .if'ing */
    enum if_states {
	IF_ACTIVE,		/* .if or .elif part active */
	ELSE_ACTIVE,		/* .else part active */
	SEARCH_FOR_ELIF,	/* searching for .elif/else to execute */
	SKIP_TO_ELSE,           /* has been true, but not seen '.else' */
	SKIP_TO_ENDIF		/* nothing else to execute */
    };
    static enum if_states cond_state[MAXIF + 1] = { IF_ACTIVE };

    const struct If *ifp;
    Boolean 	    isElif;
    Boolean 	    value;
    int	    	    level;  	/* Level at which to report errors. */
    enum if_states  state;

    level = PARSE_FATAL;

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
	    return cond_state[cond_depth] <= ELSE_ACTIVE ? COND_PARSE : COND_SKIP;
	}

	/* Quite likely this is 'else' or 'elif' */
	line += 2;
	if (istoken(line, "se", 2)) {
	    /* It is else... */
	    if (cond_depth == cond_min_depth) {
		Parse_Error(level, "if-less else");
		return COND_INVALID;
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
    for (ifp = ifs; ; ifp++) {
	if (ifp->form == NULL)
	    return COND_INVALID;
	if (istoken(ifp->form, line, ifp->formlen)) {
	    line += ifp->formlen;
	    break;
	}
    }

    /* Now we know what sort of 'if' it is... */
    state = cond_state[cond_depth];

    if (isElif) {
	if (cond_depth == cond_min_depth) {
	    Parse_Error(level, "if-less elif");
	    return COND_INVALID;
	}
	if (state == SKIP_TO_ENDIF || state == ELSE_ACTIVE)
	    Parse_Error(PARSE_WARNING, "extra elif");
	if (state != SEARCH_FOR_ELIF) {
	    /* Either just finished the 'true' block, or already SKIP_TO_ELSE */
	    cond_state[cond_depth] = SKIP_TO_ELSE;
	    return COND_SKIP;
	}
    } else {
	if (cond_depth >= MAXIF) {
	    Parse_Error(PARSE_FATAL, "Too many nested if's. %d max.", MAXIF);
	    return COND_INVALID;
	}
	cond_depth++;
	if (state > ELSE_ACTIVE) {
	    /* If we aren't parsing the data, treat as always false */
	    cond_state[cond_depth] = SKIP_TO_ELSE;
	    return COND_SKIP;
	}
    }

    /* Initialize file-global variables for parsing the expression */
    condDefProc = ifp->defProc;
    condInvert = ifp->doNot;

    /* And evaluate the conditional expresssion */
    if (Cond_EvalExpression(0, line, &value, 1) == COND_INVALID) {
	/* Although we get make to reprocess the line, set a state */
	cond_state[cond_depth] = SEARCH_FOR_ELIF;
	return COND_INVALID;
    }

    if (!value) {
	cond_state[cond_depth] = SEARCH_FOR_ELIF;
	return COND_SKIP;
    }
    cond_state[cond_depth] = IF_ACTIVE;
    return COND_PARSE;
}



/*-
 *-----------------------------------------------------------------------
 * Cond_End --
 *	Make sure everything's clean at the end of a makefile.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Parse_Error will be called if open conditionals are around.
 *
 *-----------------------------------------------------------------------
 */
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
