/* $NetBSD: token_postfix_op.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the postfix increment and decrement operators '++' and '--'.
 */

#indent input
void
function(void)
{
	counter++;
	++counter;		/* this is a prefix unary operator instead */
	counter--;
	--counter;		/* this is a prefix unary operator instead */
}
#indent end

#indent run-equals-input
