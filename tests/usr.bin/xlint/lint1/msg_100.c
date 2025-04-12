/*	$NetBSD: msg_100.c,v 1.7 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_100.c"

/* Test for message: unary '+' is invalid in traditional C [100] */

/* lint1-flags: -tw */

int
unary_plus(x)
	int x;
{
	/* expect+1: warning: unary '+' is invalid in traditional C [100] */
	return +x;
}
