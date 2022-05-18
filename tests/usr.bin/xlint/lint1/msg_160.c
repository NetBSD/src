/*	$NetBSD: msg_160.c,v 1.8 2022/05/18 20:10:11 rillig Exp $	*/
# 3 "msg_160.c"

// Test for message: operator '==' found where '=' was expected [160]

/* lint1-extra-flags: -h */

_Bool
both_equal_or_unequal(int a, int b, int c, int d)
{
	/*
	 * Before tree.c 1.201 from 2021-01-31, lint warned about each of
	 * the '==' subexpressions even though there is nothing surprising
	 * about them.
	 */
	return (a == b) == (c == d);
}

void
eval(_Bool);

void
unparenthesized(int a, int b, int c, _Bool z)
{
	/*
	 * This one might be legitimate since the second '==' has _Bool
	 * on both sides.  Parenthesizing its left-hand operand doesn't
	 * hurt though.
	 */
	eval(a == b == z);		/* expect: 160 */

	/*
	 * Before tree.c 1.201 from 2021-01-31, lint warned about the
	 * parenthesized '==' subexpression even though there is nothing
	 * surprising about it.
	 */
	eval((a == b) == z);

	/*
	 * This one is definitely wrong.  C, unlike Python, does not chain
	 * comparison operators in the way mathematicians are used to.
	 */
	eval(a == b == c);		/* expect: 160 */

	/* Parenthesizing one of the operands makes it obvious enough. */
	/*
	 * Before tree.c 1.201 from 2021-01-31, lint warned about the
	 * parenthesized '==' subexpression even though there is nothing
	 * surprising about it.
	 */
	eval((a == b) == c);
	/*
	 * Before tree.c 1.201 from 2021-01-31, lint warned about the
	 * parenthesized '==' subexpression even though there is nothing
	 * surprising about it.
	 */
	eval(a == (b == c));
}

void
assignment_in_comma_expression(int len)
{

	/*
	 * No extra parentheses, just a comma operator.
	 *
	 * The usual interpretation is that the left-hand operand of the
	 * comma is a preparation, most often an assignment, and the
	 * right-hand operand of the comma is the actual condition.
	 */
	if (len = 3 * len + 1, len == 0)
		return;

	/* Seen in bin/csh/dir.c 1.35 from 2020-08-09, line 223. */
	/*
	 * The extra parentheses are typically used to inform the compiler
	 * that an assignment using '=' is intentional, in particular it is
	 * not a typo of the comparison operator '=='.
	 *
	 * The comma operator in a condition is seldom used, which makes it
	 * reasonable to assume that the code author selected the operators
	 * on purpose.
	 *
	 * In this case the parentheses are redundant, it's quite possible
	 * that they come from a macro expansion though.
	 */
	if ((len = 3 * len + 1, len == 0))
		return;

	/*
	 * If the comma expression is part of a larger expression, the
	 * parentheses are required to mark the operator precedence.  The
	 * parentheses must therefore not be interpreted as changing the
	 * intention from a condition to an assignment.
	 */
	if ((len = 3 * len + 1, len == 0) && len < 2)
		return;
}
