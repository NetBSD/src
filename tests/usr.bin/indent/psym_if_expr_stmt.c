/* $NetBSD: psym_if_expr_stmt.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the parser symbol psym_if_expr_stmt, which represents the state
 * after reading the keyword 'if', the controlling expression and the
 * statement for the 'then' branch.  If the next token is 'else', another
 * statement will follow, otherwise the 'if' statement is finished already.
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
