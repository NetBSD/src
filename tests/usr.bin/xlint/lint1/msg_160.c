/*	$NetBSD: msg_160.c,v 1.5 2021/01/31 12:20:00 rillig Exp $	*/
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
