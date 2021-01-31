/*	$NetBSD: msg_160.c,v 1.4 2021/01/31 11:59:56 rillig Exp $	*/
# 3 "msg_160.c"

// Test for message: operator '==' found where '=' was expected [160]

/* lint1-extra-flags: -h */

_Bool
both_equal_or_unequal(int a, int b, int c, int d)
{
	/* XXX: Why shouldn't this be legitimate? */
	return (a == b) == (c == d);	/* expect: 160, 160 */
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

	eval((a == b) == z);		/*FIXME*//* expect: 160 */

	/*
	 * This one is definitely wrong.  C, unlike Python, does not chain
	 * comparison operators in the way mathematicians are used to.
	 */
	eval(a == b == c);		/* expect: 160 */

	/* Parenthesizing one of the operands makes it obvious enough. */
	eval((a == b) == c);		/*FIXME*//* expect: 160 */
	eval(a == (b == c));		/*FIXME*//* expect: 160 */
}
