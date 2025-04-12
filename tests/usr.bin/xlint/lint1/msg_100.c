/*	$NetBSD: msg_100.c,v 1.8 2025/04/12 15:57:26 rillig Exp $	*/
# 3 "msg_100.c"

/* Test for message: unary '+' requires C90 or later [100] */

/* lint1-flags: -tw */

int
unary_plus(x)
	int x;
{
	/* expect+1: warning: unary '+' requires C90 or later [100] */
	return +x;
}
