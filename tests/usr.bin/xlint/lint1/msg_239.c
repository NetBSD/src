/*	$NetBSD: msg_239.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_239.c"

// Test for message: constant argument to '!' [239]

/* lint1-extra-flags: -h -X 351 */

_Bool
example(int n)
{
	_Bool b;

	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: warning: constant argument to '!' [239] */
	b = !0;
	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: warning: constant argument to '!' [239] */
	b = !1;
	b = !(n > 1);

	return b;
}
