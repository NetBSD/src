/* $NetBSD: token_if_expr_stmt_else.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for 'if' followed by a parenthesized expression, a statement and the
 * keyword 'else'.
 *
 * At this point, the statement needs to be completed with another statement.
 */

#indent input
void
function(void)
{
	if (cond)
		stmt();
	else
		stmt();
}
#indent end

#indent run-equals-input
