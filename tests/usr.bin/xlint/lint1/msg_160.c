/*	$NetBSD: msg_160.c,v 1.6 2021/10/09 21:25:39 rillig Exp $	*/
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

/* Seen in bin/csh/dir.c 1.35 from 2020-08-09, line 223. */
void
assignment_in_comma_expression(void)
{
	int len;

	/* FIXME: The following code is totally fine. */
	/* expect+1: warning: operator '==' found where '=' was expected [160] */
	if ((len = 3, len == 0))
		return;
}
