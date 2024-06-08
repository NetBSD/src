/*	$NetBSD: expr.c,v 1.1 2024/06/08 09:09:20 rillig Exp $	*/
# 3 "expr.c"

/*
 * Miscellaneous tests for expressions.
 */

/* lint1-extra-flags: -X 351 */

struct bit_fields {
	unsigned u32: 32;
};

unsigned long
expr_cond_cvt(unsigned long ul)
{
	struct bit_fields bits = { 0 };
	// Combining 'unsigned:32' and 'unsigned long' in the ':' operator
	// results in 'unsigned long'.
	return bits.u32 < ul ? bits.u32 : ul;
}

// Before tree.c 1.76 from 2014-04-17, lint crashed with an internal error,
// due to an uninitialized right operand of a tree node.
void
crash_in_assignment(void)
{
	/* expect+1: warning: 'x' set but not used in function 'crash_in_assignment' [191] */
	double x = 1;
	int foo = 0;
	if (foo)
		x = 1;
}
