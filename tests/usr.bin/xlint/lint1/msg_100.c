/*	$NetBSD: msg_100.c,v 1.6 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_100.c"

/* Test for message: unary '+' is illegal in traditional C [100] */

/* lint1-flags: -tw */

int
unary_plus(x)
	int x;
{
	/* expect+1: warning: unary '+' is illegal in traditional C [100] */
	return +x;
}
