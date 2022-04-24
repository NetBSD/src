/* $NetBSD: lsym_unary_op.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the token lsym_unary_op, which represents a unary operator.
 *
 * In an expression, a unary operator is written without blank next to its
 * argument.
 *
 * In a type name, the "unary operator" '*' represents the derivation of a
 * pointer type.
 */

//indent input
void
unary_operators(void)
{
	/* In the order of appearance in C11 6.5. */
	function(a++, a--, ++a, --a, &a, *a, +a, -a, ~a, !a);
}
//indent end

//indent run-equals-input
