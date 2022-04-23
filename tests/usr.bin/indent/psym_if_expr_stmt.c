/* $NetBSD: psym_if_expr_stmt.c,v 1.3 2022/04/23 09:01:03 rillig Exp $ */

/*
 * Tests for the parser symbol psym_if_expr_stmt, which represents the state
 * after reading the keyword 'if', the controlling expression and the
 * statement for the 'then' branch.
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
