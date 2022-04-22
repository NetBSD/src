/* $NetBSD: token_if_expr_stmt.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for 'if' followed by a parenthesized expression and a statement.
 *
 * At this point, the 'if' statement is not necessarily complete, it can be
 * completed with the keyword 'else' followed by a statement.
 *
 * Any token other than 'else' completes the 'if' statement.
 */

#indent input
void
function(void)
{
	if (cond)
		stmt();
	if (cond)
		stmt();
	else			/* belongs to the second 'if' */
		stmt();
}
#indent end

#indent run-equals-input
