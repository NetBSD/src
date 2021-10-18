/* $NetBSD: token_if_expr.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

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
