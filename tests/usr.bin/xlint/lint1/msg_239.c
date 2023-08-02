/*	$NetBSD: msg_239.c,v 1.8 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_239.c"

// Test for message: constant operand to '!' [239]

/* lint1-extra-flags: -h -X 351 */

_Bool
example(int n)
{
	_Bool b;

	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: warning: constant operand to '!' [239] */
	b = !0;
	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: warning: constant operand to '!' [239] */
	b = !1;
	b = !(n > 1);

	return b;
}
