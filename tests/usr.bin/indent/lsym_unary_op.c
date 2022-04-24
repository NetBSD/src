/* $NetBSD: lsym_unary_op.c,v 1.5 2022/04/24 10:36:37 rillig Exp $ */

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


/*
 * The unary operators '+' and '-' can occur in long chains.  In these chains,
 * adjacent '+' must not be merged to '++' since that would be a different
 * token.  The same applies to '&', but that case is irrelevant in practice
 * since the address of an address cannot be taken.
 */
//indent input
int var=+3;
int mixed=+-+-+-+-+-+-+-+-+-+-+-+-+-3;
int count=~-~-~-~-~-~-~-~-~-~-~-~-~-3;
int same = + + + + + - - - - - 3;
//indent end

//indent run -di0
int var = +3;
int mixed = +-+-+-+-+-+-+-+-+-+-+-+-+-3;
int count = ~-~-~-~-~-~-~-~-~-~-~-~-~-3;
int same = + + + + +- - - - -3;
//indent end


/*
 * A special kind of unary operator is '->', which additionally suppresses the
 * next space.
 */
//indent input
int var = p -> member;
//indent end

//indent run -di0
int var = p->member;
//indent end
