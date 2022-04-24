/* $NetBSD: psym_if_expr.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the parser symbol psym_if_expr, representing the parser state
 * after reading the keyword 'if' and the controlling expression, now waiting
 * for the statement of the 'then' branch.
 */

//indent input
void function(void) {
	if(cond) stmt();
}
//indent end

//indent run
void
function(void)
{
	if (cond)
		stmt();
}
//indent end
