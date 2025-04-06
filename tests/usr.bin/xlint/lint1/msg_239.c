/*	$NetBSD: msg_239.c,v 1.9 2025/04/06 20:56:14 rillig Exp $	*/
# 3 "msg_239.c"

// Test for message: constant operand to '!' [239]

/* lint1-extra-flags: -h -X 351 */

_Bool
example(int n)
{
	_Bool b;

	/* expect+1: warning: constant operand to '!' [239] */
	b = !0;
	/* expect+1: warning: constant operand to '!' [239] */
	b = !1;
	b = !(n > 1);

	return b;
}
