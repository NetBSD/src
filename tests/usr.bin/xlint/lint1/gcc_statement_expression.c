/*	$NetBSD: gcc_statement_expression.c,v 1.3 2023/07/15 14:54:31 rillig Exp $	*/
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

	return x;
}
