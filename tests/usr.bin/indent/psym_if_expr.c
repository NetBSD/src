/* $NetBSD: psym_if_expr.c,v 1.3 2022/04/23 09:01:03 rillig Exp $ */

/*
 * Tests for the parser symbol psym_if_expr, representing the parser state
 * after reading the keyword 'if' and the controlling expression, now waiting
 * for the statement of the 'then' branch.
 */

#indent input
void function(void) {
	if(cond) stmt();
}
#indent end

#indent run
void
function(void)
{
	if (cond)
		stmt();
}
#indent end
