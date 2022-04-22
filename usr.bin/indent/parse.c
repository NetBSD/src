/*	$NetBSD: parse.c,v 1.49 2022/04/22 21:21:20 rillig Exp $	*/

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
__RCSID("$NetBSD: parse.c,v 1.49 2022/04/22 21:21:20 rillig Exp $");
#else
__FBSDID("$FreeBSD: head/usr.bin/indent/parse.c 337651 2018-08-11 19:20:06Z pstef $");
#endif

#include <assert.h>
#include <err.h>
#include <stdio.h>

#include "indent.h"

static void reduce(void);

#ifdef debug
static const char *
psym_name(parser_symbol psym)
{
    static const char *const name[] = {
	"semicolon",
	"lbrace",
	"rbrace",
	"decl",
	"stmt",
	"stmt_list",
	"for_exprs",
	"if_expr",
	"if_expr_stmt",
	"if_expr_stmt_else",
	"else",
	"switch_expr",
	"do",
	"do_stmt",
	"while_expr",
    };

    assert(array_length(name) == (int)psym_while_expr + 1);

    return name[psym];
}
#endif

static int
decl_level(void)
{
    int level = 0;
    for (int i = ps.tos - 1; i > 0; i--)
	if (ps.s_sym[i] == psym_decl)
	    level++;
    return level;
}

/*
 * Shift the token onto the parser stack, or reduce it by combining it with
 * previous tokens.
 */
void
parse(parser_symbol psym)
{
    debug_println("parse token: %s", psym_name(psym));

    if (psym != psym_else) {
	while (ps.s_sym[ps.tos] == psym_if_expr_stmt) {
	    ps.s_sym[ps.tos] = psym_stmt;
	    reduce();
	}
    }

    switch (psym) {

    case psym_decl:
	ps.search_stmt = opt.brace_same_line;
	/* indicate that following brace should be on same line */

	if (ps.s_sym[ps.tos] == psym_decl)
	    break;		/* only put one declaration onto stack */

	break_comma = true;	/* while in a declaration, force a newline
				 * after comma */
	ps.s_sym[++ps.tos] = psym_decl;
	ps.s_ind_level[ps.tos] = ps.ind_level_follow;

	if (opt.ljust_decl)
	    ps.ind_level_follow = ps.ind_level = decl_level();
	break;

    case psym_if_expr:
	if (ps.s_sym[ps.tos] == psym_if_expr_stmt_else && opt.else_if) {
	    /*
	     * Reduce "else if" to "if". This saves a lot of stack space in
	     * case of a long "if-else-if ... else-if" sequence.
	     */
	    ps.ind_level_follow = ps.s_ind_level[ps.tos--];
	}
	/* FALLTHROUGH */
    case psym_do:
    case psym_for_exprs:
	ps.s_sym[++ps.tos] = psym;
	ps.s_ind_level[ps.tos] = ps.ind_level = ps.ind_level_follow;
	++ps.ind_level_follow;	/* subsequent statements should be indented 1 */
	ps.search_stmt = opt.brace_same_line;
	break;

    case psym_lbrace:
	break_comma = false;	/* don't break comma in an initializer list */
	if (ps.s_sym[ps.tos] == psym_stmt || ps.s_sym[ps.tos] == psym_decl
		|| ps.s_sym[ps.tos] == psym_stmt_list)
	    ++ps.ind_level_follow;	/* it is a random, isolated stmt group
					 * or a declaration */
	else {
	    if (code.s == code.e) {
		/* it is a group as part of a while, for, etc. */
		--ps.ind_level;

		/*
		 * for a switch, brace should be two levels out from the code
		 */
		if (ps.s_sym[ps.tos] == psym_switch_expr &&
			opt.case_indent >= 1.0F)
		    --ps.ind_level;
	    }
	}

	ps.s_sym[++ps.tos] = psym_lbrace;
	ps.s_ind_level[ps.tos] = ps.ind_level;
	ps.s_sym[++ps.tos] = psym_stmt;
	ps.s_ind_level[ps.tos] = ps.ind_level_follow;
	break;

    case psym_while_expr:
	if (ps.s_sym[ps.tos] == psym_do_stmt) {
	    /* it is matched with do stmt */
	    ps.ind_level = ps.ind_level_follow = ps.s_ind_level[ps.tos];
	    ps.s_sym[++ps.tos] = psym_while_expr;
	    ps.s_ind_level[ps.tos] = ps.ind_level = ps.ind_level_follow;

	} else {		/* it is a while loop */
	    ps.s_sym[++ps.tos] = psym_while_expr;
	    ps.s_ind_level[ps.tos] = ps.ind_level_follow;
	    ++ps.ind_level_follow;
	    ps.search_stmt = opt.brace_same_line;
	}

	break;

    case psym_else:
	if (ps.s_sym[ps.tos] != psym_if_expr_stmt)
	    diag(1, "Unmatched 'else'");
	else {
	    ps.ind_level = ps.s_ind_level[ps.tos];
	    ps.ind_level_follow = ps.ind_level + 1;
	    ps.s_sym[ps.tos] = psym_if_expr_stmt_else;

	    ps.search_stmt = opt.brace_same_line || opt.else_if;
	}
	break;

    case psym_rbrace:
	/* stack should have <lbrace> <stmt> or <lbrace> <stmt_list> */
	if (ps.tos > 0 && ps.s_sym[ps.tos - 1] == psym_lbrace) {
	    ps.ind_level = ps.ind_level_follow = ps.s_ind_level[--ps.tos];
	    ps.s_sym[ps.tos] = psym_stmt;
	} else
	    diag(1, "Statement nesting error");
	break;

    case psym_switch_expr:
	ps.s_sym[++ps.tos] = psym_switch_expr;
	ps.s_case_ind_level[ps.tos] = case_ind;
	ps.s_ind_level[ps.tos] = ps.ind_level_follow;
	case_ind = (float)ps.ind_level_follow + opt.case_indent;
	ps.ind_level_follow += (int)opt.case_indent + 1;
	ps.search_stmt = opt.brace_same_line;
	break;

    case psym_semicolon:	/* a simple statement */
	break_comma = false;	/* don't break after comma in a declaration */
	ps.s_sym[++ps.tos] = psym_stmt;
	ps.s_ind_level[ps.tos] = ps.ind_level;
	break;

    default:
	diag(1, "Unknown code to parser");
	return;
    }

    if (ps.tos >= STACKSIZE - 1)
	errx(1, "Parser stack overflow");

    reduce();			/* see if any reduction can be done */

#ifdef debug
    printf("parse stack:");
    for (int i = 1; i <= ps.tos; ++i)
	printf(" %s %d", psym_name(ps.s_sym[i]), ps.s_ind_level[i]);
    if (ps.tos == 0)
	printf(" empty");
    printf("\n");
#endif
}

void
parse_stmt_head(stmt_head hd)
{
    static const parser_symbol psym[] = {
	[hd_for] = psym_for_exprs,
	[hd_if] = psym_if_expr,
	[hd_switch] = psym_switch_expr,
	[hd_while] = psym_while_expr
    };
    parse(psym[hd]);
}

/*
 * Try to combine the statement on the top of the parse stack with the symbol
 * directly below it, replacing these two symbols with a single symbol.
 */
static bool
reduce_stmt(void)
{
    switch (ps.s_sym[ps.tos - 1]) {

    case psym_stmt:
    case psym_stmt_list:
	ps.s_sym[--ps.tos] = psym_stmt_list;
	return true;

    case psym_do:
	ps.s_sym[--ps.tos] = psym_do_stmt;
	ps.ind_level_follow = ps.s_ind_level[ps.tos];
	return true;

    case psym_if_expr:
	ps.s_sym[--ps.tos] = psym_if_expr_stmt;
	int i = ps.tos - 1;
	while (ps.s_sym[i] != psym_stmt &&
		ps.s_sym[i] != psym_stmt_list &&
		ps.s_sym[i] != psym_lbrace)
	    --i;
	ps.ind_level_follow = ps.s_ind_level[i];
	/*
	 * For the time being, assume that there is no 'else' on this 'if',
	 * and set the indentation level accordingly. If an 'else' is scanned,
	 * it will be fixed up later.
	 */
	return true;

    case psym_switch_expr:
	case_ind = ps.s_case_ind_level[ps.tos - 1];
	/* FALLTHROUGH */
    case psym_decl:		/* finish of a declaration */
    case psym_if_expr_stmt_else:
    case psym_for_exprs:
    case psym_while_expr:
	ps.s_sym[--ps.tos] = psym_stmt;
	ps.ind_level_follow = ps.s_ind_level[ps.tos];
	return true;

    default:
	return false;
    }
}

/*
 * Repeatedly try to reduce the top two symbols on the parse stack to a
 * single symbol, until no more reductions are possible.
 */
static void
reduce(void)
{
again:
    if (ps.s_sym[ps.tos] == psym_stmt && reduce_stmt())
	goto again;
    if (ps.s_sym[ps.tos] == psym_while_expr &&
	    ps.s_sym[ps.tos - 1] == psym_do_stmt) {
	ps.tos -= 2;
	goto again;
    }
}
