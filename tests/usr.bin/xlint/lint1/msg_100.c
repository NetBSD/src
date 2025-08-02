/*	$NetBSD: msg_100.c,v 1.6.4.1 2025/08/02 05:58:16 perseant Exp $	*/
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
