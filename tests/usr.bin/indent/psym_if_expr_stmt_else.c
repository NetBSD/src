/* $NetBSD: psym_if_expr_stmt_else.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the parser symbol psym_if_expr_stmt_else, which represents the
 * parser state after reading the keyword 'if', the controlling expression,
 * the statement of the 'then' branch and the keyword 'else'.  If the next
 * token is an 'if', the formatting depends on the option '-ei' or '-nei'.  In
 * all other situations, the next statement will finish the 'if' statement.
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
