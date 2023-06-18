/*	$NetBSD: parse.c,v 1.79 2023/06/18 06:56:32 rillig Exp $	*/

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
__RCSID("$NetBSD: parse.c,v 1.79 2023/06/18 06:56:32 rillig Exp $");

#include <stdlib.h>

#include "indent.h"

/*
 * Try to combine the statement on the top of the parse stack with the symbol
 * directly below it, replacing these two symbols with a single symbol.
 */
static bool
psyms_reduce_stmt(void)
{
	switch (ps.psyms.sym[ps.psyms.len - 2]) {

	case psym_stmt:
		ps.psyms.len--;
		ps.psyms.sym[ps.psyms.len - 1] = psym_stmt;
		return true;

	case psym_do:
		ps.psyms.len--;
		ps.psyms.sym[ps.psyms.len - 1] = psym_do_stmt;
		ps.ind_level_follow = ps.psyms.ind_level[ps.psyms.len - 1];
		return true;

	case psym_if_expr:
		ps.psyms.len--;
		ps.psyms.sym[ps.psyms.len - 1] = psym_if_expr_stmt;
		/* For the time being, assume that there is no 'else' on this
		 * 'if', and set the indentation level accordingly. If an
		 * 'else' is scanned, it will be fixed up later. */
		size_t i = ps.psyms.len - 2;
		while (ps.psyms.sym[i] != psym_stmt
		    && ps.psyms.sym[i] != psym_lbrace_block)
			i--;
		ps.ind_level_follow = ps.psyms.ind_level[i];
		return true;

	case psym_switch_expr:
	case psym_decl:
	case psym_if_expr_stmt_else:
	case psym_for_exprs:
	case psym_while_expr:
		ps.psyms.len--;
		ps.psyms.sym[ps.psyms.len - 1] = psym_stmt;
		ps.ind_level_follow = ps.psyms.ind_level[ps.psyms.len - 1];
		return true;

	default:
		return false;
	}
}

static int
left_justify_decl_level(void)
{
	int level = 0;
	for (size_t i = ps.psyms.len - 2; i > 0; i--)
		if (ps.psyms.sym[i] == psym_decl)
			level++;
	return level;
}

void
ps_push(parser_symbol psym, bool follow)
{
	if (ps.psyms.len == ps.psyms.cap) {
		ps.psyms.cap += 16;
		ps.psyms.sym = nonnull(realloc(ps.psyms.sym,
			sizeof(ps.psyms.sym[0]) * ps.psyms.cap));
		ps.psyms.ind_level = nonnull(realloc(ps.psyms.ind_level,
			sizeof(ps.psyms.ind_level[0]) * ps.psyms.cap));
	}
	ps.psyms.len++;
	ps.psyms.sym[ps.psyms.len - 1] = psym;
	ps.psyms.ind_level[ps.psyms.len - 1] =
	    follow ? ps.ind_level_follow : ps.ind_level;
}

/*
 * Repeatedly try to reduce the top two symbols on the parse stack to a single
 * symbol, until no more reductions are possible.
 */
static void
psyms_reduce(void)
{
again:
	if (ps.psyms.len >= 2 && ps.psyms.sym[ps.psyms.len - 1] == psym_stmt
	    && psyms_reduce_stmt())
		goto again;
	if (ps.psyms.sym[ps.psyms.len - 1] == psym_while_expr &&
	    ps.psyms.sym[ps.psyms.len - 2] == psym_do_stmt) {
		ps.psyms.len -= 2;
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
 * Shift the token onto the parser stack, then try to reduce it by combining it with
 * previous tokens.
 */
void
parse(parser_symbol psym)
{
	debug_blank_line();
	debug_println("parse token: %s", psym_name[psym]);

	if (psym != psym_else) {
		while (ps.psyms.sym[ps.psyms.len - 1] == psym_if_expr_stmt) {
			ps.psyms.sym[ps.psyms.len - 1] = psym_stmt;
			psyms_reduce();
		}
	}

	switch (psym) {

	case psym_lbrace_block:
	case psym_lbrace_struct:
	case psym_lbrace_union:
	case psym_lbrace_enum:
		ps.break_after_comma = false;
		if (ps.psyms.sym[ps.psyms.len - 1] == psym_decl
		    || ps.psyms.sym[ps.psyms.len - 1] == psym_stmt)
			ps.ind_level_follow++;
		else if (code.len == 0) {
			/* It is part of a while, for, etc. */
			ps.ind_level--;

			/* for a switch, brace should be two levels out from
			 * the code */
			if (ps.psyms.sym[ps.psyms.len - 1] == psym_switch_expr
			    && opt.case_indent >= 1.0F)
				ps.ind_level--;
		}

		ps_push(psym, false);
		ps_push(psym_stmt, true);
		break;

	case psym_rbrace:
		/* stack should have <lbrace> <stmt> or <lbrace> <decl> */
		if (!(ps.psyms.len >= 2
			&& is_lbrace(ps.psyms.sym[ps.psyms.len - 2]))) {
			diag(1, "Statement nesting error");
			break;
		}
		ps.psyms.len--;
		ps.ind_level = ps.psyms.ind_level[ps.psyms.len - 1];
		ps.ind_level_follow = ps.ind_level;
		ps.psyms.sym[ps.psyms.len - 1] = psym_stmt;
		break;

	case psym_decl:
		if (ps.psyms.sym[ps.psyms.len - 1] == psym_decl)
			break;	/* only put one declaration onto stack */

		ps.break_after_comma = true;
		ps_push(psym_decl, true);

		if (opt.left_justify_decl) {
			ps.ind_level = left_justify_decl_level();
			ps.ind_level_follow = ps.ind_level;
		}
		break;

	case psym_stmt:
		ps.break_after_comma = false;
		ps_push(psym_stmt, false);
		break;

	case psym_if_expr:
		if (ps.psyms.sym[ps.psyms.len - 1] == psym_if_expr_stmt_else
		    && opt.else_if_in_same_line) {
			ps.ind_level_follow =
			    ps.psyms.ind_level[ps.psyms.len - 1];
			ps.psyms.len--;
		}
		/* FALLTHROUGH */
	case psym_do:
	case psym_for_exprs:
		ps.ind_level = ps.ind_level_follow;
		ps.ind_level_follow = ps.ind_level + 1;
		ps_push(psym, false);
		break;

	case psym_else:
		if (ps.psyms.sym[ps.psyms.len - 1] != psym_if_expr_stmt) {
			diag(1, "Unmatched 'else'");
			break;
		}
		ps.ind_level = ps.psyms.ind_level[ps.psyms.len - 1];
		ps.ind_level_follow = ps.ind_level + 1;
		ps.psyms.sym[ps.psyms.len - 1] = psym_if_expr_stmt_else;
		break;

	case psym_switch_expr:
		ps_push(psym_switch_expr, true);
		ps.ind_level_follow += (int)opt.case_indent + 1;
		break;

	case psym_while_expr:
		if (ps.psyms.sym[ps.psyms.len - 1] == psym_do_stmt) {
			ps.ind_level = ps.psyms.ind_level[ps.psyms.len - 1];
			ps.ind_level_follow = ps.ind_level;
			ps_push(psym_while_expr, false);
		} else {
			ps_push(psym_while_expr, true);
			ps.ind_level_follow++;
		}
		break;

	default:
		diag(1, "Unknown code to parser");
		return;
	}

	debug_psyms_stack("before reduction");
	psyms_reduce();
	debug_psyms_stack("after reduction");
}
