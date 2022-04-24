/* $NetBSD: token_postfix_op.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the postfix increment and decrement operators '++' and '--'.
 */

//indent input
void
function(void)
{
	counter++;
	++counter;		/* this is a prefix unary operator instead */
	counter--;
	--counter;		/* this is a prefix unary operator instead */
}
//indent end

//indent run-equals-input
