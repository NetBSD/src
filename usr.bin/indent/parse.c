/*	$NetBSD: parse.c,v 1.29 2021/10/05 06:55:24 rillig Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#if 0
static char sccsid[] = "@(#)parse.c	8.1 (Berkeley) 6/6/93";
#endif

#include <sys/cdefs.h>
#if defined(__NetBSD__)
__RCSID("$FreeBSD$");
#else
__FBSDID("$FreeBSD: head/usr.bin/indent/parse.c 337651 2018-08-11 19:20:06Z pstef $");
#endif

#include <err.h>
#include <stdio.h>

#include "indent.h"

static void reduce(void);

void
parse(token_type ttype)
{

#ifdef debug
    printf("parse token: '%s' \"%s\"\n", token_type_name(ttype), token.s);
#endif

    while (ps.p_stack[ps.tos] == if_expr_stmt && ttype != keyword_else) {
	/* true if we have an if without an else */
	ps.p_stack[ps.tos] = stmt;	/* apply the if(..) stmt ::= stmt
					 * reduction */
	reduce();		/* see if this allows any reduction */
    }


    switch (ttype) {		/* go on and figure out what to do with the
				 * input */

    case decl:			/* scanned a declaration word */
	ps.search_brace = opt.btype_2;
	/* indicate that following brace should be on same line */
	if (ps.p_stack[ps.tos] != decl) {	/* only put one declaration
						 * onto stack */
	    break_comma = true;	/* while in declaration, newline should be
				 * forced after comma */
	    ps.p_stack[++ps.tos] = decl;
	    ps.il[ps.tos] = ps.ind_level_follow;

	    if (opt.ljust_decl) {	/* only do if we want left justified
					 * declarations */
		ps.ind_level = 0;
		for (int i = ps.tos - 1; i > 0; --i)
		    if (ps.p_stack[i] == decl)
			++ps.ind_level;	/* indentation is number of
					 * declaration levels deep we are */
		ps.ind_level_follow = ps.ind_level;
	    }
	}
	break;

    case if_expr:		/* 'if' '(' <expr> ')' */
	if (ps.p_stack[ps.tos] == if_expr_stmt_else && opt.else_if) {
	    /*
	     * Note that the stack pointer here is decremented, effectively
	     * reducing "else if" to "if". This saves a lot of stack space in
	     * case of a long "if-else-if ... else-if" sequence.
	     */
	    ps.ind_level_follow = ps.il[ps.tos--];
	}
	/* the rest is the same as for keyword_do and for_exprs */
	/* FALLTHROUGH */
    case keyword_do:		/* 'do' */
    case for_exprs:		/* 'for' (...) */
	ps.p_stack[++ps.tos] = ttype;
	ps.il[ps.tos] = ps.ind_level = ps.ind_level_follow;
	++ps.ind_level_follow;	/* subsequent statements should be indented 1 */
	ps.search_brace = opt.btype_2;
	break;

    case lbrace:		/* scanned { */
	break_comma = false;	/* don't break comma in an initial list */
	if (ps.p_stack[ps.tos] == stmt || ps.p_stack[ps.tos] == decl
		|| ps.p_stack[ps.tos] == stmt_list)
	    ++ps.ind_level_follow;	/* it is a random, isolated stmt
				 * group or a declaration */
	else {
	    if (code.s == code.e) {
		/*
		 * only do this if there is nothing on the line
		 */
		--ps.ind_level;
		/*
		 * it is a group as part of a while, for, etc.
		 */
		if (ps.p_stack[ps.tos] == switch_expr && opt.case_indent >= 1)
		    --ps.ind_level;
		/*
		 * for a switch, brace should be two levels out from the code
		 */
	    }
	}

	ps.p_stack[++ps.tos] = lbrace;
	ps.il[ps.tos] = ps.ind_level;
	ps.p_stack[++ps.tos] = stmt;
	/* allow null stmt between braces */
	ps.il[ps.tos] = ps.ind_level_follow;
	break;

    case while_expr:		/* 'while' '(' <expr> ')' */
	if (ps.p_stack[ps.tos] == do_stmt) {
	    /* it is matched with do stmt */
	    ps.ind_level = ps.ind_level_follow = ps.il[ps.tos];
	    ps.p_stack[++ps.tos] = while_expr;
	    ps.il[ps.tos] = ps.ind_level = ps.ind_level_follow;
	} else {		/* it is a while loop */
	    ps.p_stack[++ps.tos] = while_expr;
	    ps.il[ps.tos] = ps.ind_level_follow;
	    ++ps.ind_level_follow;
	    ps.search_brace = opt.btype_2;
	}

	break;

    case keyword_else:
	if (ps.p_stack[ps.tos] != if_expr_stmt)
	    diag(1, "Unmatched 'else'");
	else {
	    ps.ind_level = ps.il[ps.tos];	/* indentation for else should
						 * be same as for if */
	    ps.ind_level_follow = ps.ind_level + 1;	/* everything following
							 * should be 1 level
							 * deeper */
	    ps.p_stack[ps.tos] = if_expr_stmt_else;
	    /* remember if with else */
	    ps.search_brace = opt.btype_2 | opt.else_if;
	}
	break;

    case rbrace:		/* scanned a } */
	/* stack should have <lbrace> <stmt> or <lbrace> <stmt_list> */
	if (ps.tos > 0 && ps.p_stack[ps.tos - 1] == lbrace) {
	    ps.ind_level = ps.ind_level_follow = ps.il[--ps.tos];
	    ps.p_stack[ps.tos] = stmt;
	} else
	    diag(1, "Statement nesting error");
	break;

    case switch_expr:		/* had switch (...) */
	ps.p_stack[++ps.tos] = switch_expr;
	ps.cstk[ps.tos] = case_ind;
	/* save current case indent level */
	ps.il[ps.tos] = ps.ind_level_follow;
	/* cases should be one level deeper than the switch */
	case_ind = (float)ps.ind_level_follow + opt.case_indent;
	/* statements should be two levels deeper */
	ps.ind_level_follow += (int)opt.case_indent + 1;
	ps.search_brace = opt.btype_2;
	break;

    case semicolon:		/* this indicates a simple stmt */
	break_comma = false;	/* turn off flag to break after commas in a
				 * declaration */
	ps.p_stack[++ps.tos] = stmt;
	ps.il[ps.tos] = ps.ind_level;
	break;

    default:			/* this is an error */
	diag(1, "Unknown code to parser");
	return;


    }				/* end of switch */

    if (ps.tos >= STACKSIZE - 1)
	errx(1, "Parser stack overflow");

    reduce();			/* see if any reduction can be done */

#ifdef debug
    printf("parse stack:");
    for (int i = 1; i <= ps.tos; ++i)
	printf(" ('%s' at %d)", token_type_name(ps.p_stack[i]), ps.il[i]);
    if (ps.tos == 0)
	printf(" empty");
    printf("\n");
#endif
}

/*----------------------------------------------*\
|   REDUCTION PHASE				 |
\*----------------------------------------------*/

/*
 * Try to combine the statement on the top of the parse stack with the symbol
 * directly below it, replacing these two symbols with a single symbol.
 */
static bool
reduce_stmt(void)
{
    switch (ps.p_stack[ps.tos - 1]) {

    case stmt:			/* stmt stmt */
    case stmt_list:		/* stmt_list stmt */
	ps.p_stack[--ps.tos] = stmt_list;
	return true;

    case keyword_do:		/* 'do' <stmt> */
	ps.p_stack[--ps.tos] = do_stmt;
	ps.ind_level_follow = ps.il[ps.tos];
	return true;

    case if_expr:		/* 'if' '(' <expr> ')' <stmt> */
	ps.p_stack[--ps.tos] = if_expr_stmt;
	int i = ps.tos - 1;
	while (ps.p_stack[i] != stmt &&
	       ps.p_stack[i] != stmt_list &&
	       ps.p_stack[i] != lbrace)
	    --i;
	ps.ind_level_follow = ps.il[i];
	/*
	 * for the time being, we will assume that there is no else on this
	 * if, and set the indentation level accordingly. If an 'else' is
	 * scanned, it will be fixed up later
	 */
	return true;

    case switch_expr:		/* 'switch' '(' <expr> ')' <stmt> */
	case_ind = ps.cstk[ps.tos - 1];
	/* FALLTHROUGH */
    case decl:			/* finish of a declaration */
    case if_expr_stmt_else:	/* 'if' '(' <expr> ')' <stmt> 'else' <stmt> */
    case for_exprs:		/* 'for' '(' ... ')' <stmt> */
    case while_expr:		/* 'while' '(' <expr> ')' <stmt> */
	ps.p_stack[--ps.tos] = stmt;
	ps.ind_level_follow = ps.il[ps.tos];
	return true;

    default:			/* <anything else> <stmt> */
	return false;
    }
}

/*
 * Repeatedly try to reduce the top two symbols on the parse stack to a
 * single symbol, until no more reductions are possible.
 *
 * On each reduction, ps.i_l_follow (the indentation for the following line)
 * is set to the indentation level associated with the old TOS.
 */
static void
reduce(void)
{
again:
    if (ps.p_stack[ps.tos] == stmt) {
	if (reduce_stmt())
	    goto again;
    } else if (ps.p_stack[ps.tos] == while_expr) {
	if (ps.p_stack[ps.tos - 1] == do_stmt) {
	    ps.tos -= 2;
	    goto again;
	}
    }
}
