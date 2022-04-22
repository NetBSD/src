/* $NetBSD: token_if_expr.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for 'if' followed by a parenthesized expression.
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
