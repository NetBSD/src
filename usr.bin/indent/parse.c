/*	$NetBSD: parse.c,v 1.69 2023/06/07 15:46:12 rillig Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: parse.c,v 1.69 2023/06/07 15:46:12 rillig Exp $");

#include <err.h>

#include "indent.h"

/*
 * Try to combine the statement on the top of the parse stack with the symbol
 * directly below it, replacing these two symbols with a single symbol.
 */
static bool
psyms_reduce_stmt(struct psym_stack *psyms)
{
	switch (psyms->sym[psyms->top - 1]) {

	case psym_stmt:
	case psym_stmt_list:
		psyms->sym[--psyms->top] = psym_stmt_list;
		return true;

	case psym_do:
		psyms->sym[--psyms->top] = psym_do_stmt;
		ps.ind_level_follow = psyms->ind_level[psyms->top];
		return true;

	case psym_if_expr:
		psyms->sym[--psyms->top] = psym_if_expr_stmt;
		int i = psyms->top - 1;
		while (psyms->sym[i] != psym_stmt &&
		    psyms->sym[i] != psym_stmt_list &&
		    psyms->sym[i] != psym_lbrace_block)
			--i;
		ps.ind_level_follow = psyms->ind_level[i];
		/* For the time being, assume that there is no 'else' on this
		 * 'if', and set the indentation level accordingly. If an
		 * 'else' is scanned, it will be fixed up later. */
		return true;

	case psym_switch_expr:
	case psym_decl:
	case psym_if_expr_stmt_else:
	case psym_for_exprs:
	case psym_while_expr:
		psyms->sym[--psyms->top] = psym_stmt;
		ps.ind_level_follow = psyms->ind_level[psyms->top];
		return true;

	default:
		return false;
	}
}

static int
decl_level(void)
{
	int level = 0;
	for (int i = ps.psyms.top - 1; i > 0; i--)
		if (ps.psyms.sym[i] == psym_decl)
			level++;
	return level;
}

static void
ps_push(parser_symbol psym)
{
	ps.psyms.sym[++ps.psyms.top] = psym;
	ps.psyms.ind_level[ps.psyms.top] = ps.ind_level;
}

static void
ps_push_follow(parser_symbol psym)
{
	ps.psyms.sym[++ps.psyms.top] = psym;
	ps.psyms.ind_level[ps.psyms.top] = ps.ind_level_follow;
}

/*
 * Repeatedly try to reduce the top two symbols on the parse stack to a single
 * symbol, until no more reductions are possible.
 */
static void
psyms_reduce(struct psym_stack *psyms)
{
again:
	if (psyms->sym[psyms->top] == psym_stmt && psyms_reduce_stmt(psyms))
		goto again;
	if (psyms->sym[psyms->top] == psym_while_expr &&
	    psyms->sym[psyms->top - 1] == psym_do_stmt) {
		psyms->top -= 2;
		goto again;
	}
}

static bool
is_lbrace(parser_symbol psym)
{
	return psym == psym_lbrace_block
	    || psym == psym_lbrace_struct
	    || psym == psym_lbrace_union
	    || psym == psym_lbrace_enum;
}

/*
 * Shift the token onto the parser stack, or reduce it by combining it with
 * previous tokens.
 */
void
parse(parser_symbol psym)
{
	debug_blank_line();
	debug_println("parse token: %s", psym_name[psym]);

	struct psym_stack *psyms = &ps.psyms;
	if (psym != psym_else) {
		while (psyms->sym[psyms->top] == psym_if_expr_stmt) {
			psyms->sym[psyms->top] = psym_stmt;
			psyms_reduce(&ps.psyms);
		}
	}

	switch (psym) {

	case psym_decl:
		if (psyms->sym[psyms->top] == psym_decl)
			break;	/* only put one declaration onto stack */

		ps.break_after_comma = true;
		ps_push_follow(psym_decl);

		if (opt.left_justify_decl)
			ps.ind_level_follow = ps.ind_level = decl_level();
		break;

	case psym_if_expr:
		if (psyms->sym[psyms->top] == psym_if_expr_stmt_else
		    && opt.else_if_in_same_line)
			ps.ind_level_follow = psyms->ind_level[psyms->top--];
		/* FALLTHROUGH */
	case psym_do:
	case psym_for_exprs:
		ps.ind_level = ps.ind_level_follow++;
		ps_push(psym);
		break;

	case psym_lbrace_block:
	case psym_lbrace_struct:
	case psym_lbrace_union:
	case psym_lbrace_enum:
		ps.break_after_comma = false;
		if (psyms->sym[psyms->top] == psym_stmt
		    || psyms->sym[psyms->top] == psym_decl
		    || psyms->sym[psyms->top] == psym_stmt_list)
			++ps.ind_level_follow;	/* it is a random, isolated
						 * stmt group or a declaration
						 */
		else {
			if (code.len == 0) {
				/* it is a group as part of a while, for, etc.
				 */
				--ps.ind_level;

				/* for a switch, brace should be two levels out
				 * from the code */
				if (psyms->sym[psyms->top] == psym_switch_expr
				    && opt.case_indent >= 1.0F)
					--ps.ind_level;
			}
		}

		ps_push(psym);
		ps_push_follow(psym_stmt);
		break;

	case psym_while_expr:
		if (psyms->sym[psyms->top] == psym_do_stmt) {
			ps.ind_level =
			    ps.ind_level_follow = psyms->ind_level[psyms->top];
			ps_push(psym_while_expr);
		} else {
			ps_push_follow(psym_while_expr);
			++ps.ind_level_follow;
		}

		break;

	case psym_else:
		if (psyms->sym[psyms->top] != psym_if_expr_stmt) {
			diag(1, "Unmatched 'else'");
			break;
		}
		ps.ind_level = psyms->ind_level[psyms->top];
		ps.ind_level_follow = ps.ind_level + 1;
		psyms->sym[psyms->top] = psym_if_expr_stmt_else;
		break;

	case psym_rbrace:
		/* stack should have <lbrace> <stmt> or <lbrace> <stmt_list> */
		if (!(psyms->top > 0
		    && is_lbrace(psyms->sym[psyms->top - 1]))) {
			diag(1, "Statement nesting error");
			break;
		}
		ps.ind_level = ps.ind_level_follow =
		    psyms->ind_level[--psyms->top];
		psyms->sym[psyms->top] = psym_stmt;
		break;

	case psym_switch_expr:
		ps_push_follow(psym_switch_expr);
		ps.ind_level_follow += (int)opt.case_indent + 1;
		break;

	case psym_stmt:
		ps.break_after_comma = false;
		ps_push(psym_stmt);
		break;

	default:
		diag(1, "Unknown code to parser");
		return;
	}

	if (psyms->top >= STACKSIZE - 1)
		errx(1, "Parser stack overflow");

	debug_parse_stack("before reduction");
	psyms_reduce(&ps.psyms);
	debug_parse_stack("after reduction");
}
