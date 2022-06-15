/*	$NetBSD: msg_100.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_100.c"

/* Test for message: unary + is illegal in traditional C [100] */

/* lint1-flags: -tw */

int
unary_plus(x)
	int x;
{
	/* expect+1: warning: unary + is illegal in traditional C [100] */
	return +x;
}
