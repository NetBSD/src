/*	$NetBSD: msg_160.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_160.c"

// Test for message: operator '==' found where '=' was expected [160]

/* lint1-extra-flags: -h */

_Bool
both_equal_or_unequal(int a, int b, int c, int d)
{
	/* XXX: Why shouldn't this be legitimate? */
	return (a == b) == (c == d);		/* expect: 160, 160 */
}
