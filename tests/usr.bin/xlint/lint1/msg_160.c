/*	$NetBSD: msg_160.c,v 1.2 2021/01/09 15:32:06 rillig Exp $	*/
# 3 "msg_160.c"

// Test for message: operator '==' found where '=' was expected [160]

/* lint1-extra-flags: -h */

_Bool
both_equal_or_unequal(int a, int b, int c, int d)
{
	/* XXX: Why shouldn't this be legitimate? */
	return (a == b) == (c == d);
}
