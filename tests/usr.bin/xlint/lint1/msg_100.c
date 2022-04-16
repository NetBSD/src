/*	$NetBSD: msg_100.c,v 1.4 2022/04/16 13:25:27 rillig Exp $	*/
# 3 "msg_100.c"

/* Test for message: unary + is illegal in traditional C [100] */

/* lint1-flags: -tw */

int
unary_plus(int x)
{				/* expect: 270 */
	return +x;		/* expect: 100 */
}
