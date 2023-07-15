/*	$NetBSD: gcc_statement_expression.c,v 1.2 2023/07/15 13:51:36 rillig Exp $	*/
# 3 "gcc_statement_expression.c"

/*
 * Tests for the GCC extension 'statement expressions', which allows a block of
 * statements to occur as part of an expression.
 */


// Ensure that the inner types are accessible from outside the block.
// Depending on the memory management strategy, the inner types might be freed
// too early.
static inline int
use_inner_type_from_outside(void)
{
	int x = ({
		struct outer {
			struct inner {
				int member;
			} inner;
		} outer = { { 3 } };
		outer;
	}).inner.member;
	/* expect-1: error: type 'struct outer' does not have member 'inner' [101] */
	/* expect-2: error: type 'int' does not have member 'member' [101] */
	/*
	 * FIXME: The above types must not be removed from the symbol table
	 * yet; at least, their member names must still be known.
	 */

	return x;
}
