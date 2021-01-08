/*	$NetBSD: msg_100.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_100.c"

// Test for message: unary + is illegal in traditional C [100]

/* lint1-flags: -Stw */

int
unary_plus(int x)
{
	return +x;
}
