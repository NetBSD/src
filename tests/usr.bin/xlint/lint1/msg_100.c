/*	$NetBSD: msg_100.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_100.c"

// Test for message: unary + is illegal in traditional C [100]

/* lint1-flags: -Stw */

int
unary_plus(int x)
{				/* expect: 270 */
	return +x;		/* expect: 100 */
}
