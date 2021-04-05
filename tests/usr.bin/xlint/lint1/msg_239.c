/*	$NetBSD: msg_239.c,v 1.5 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_239.c"

// Test for message: constant argument to '!' [239]

/* lint1-extra-flags: -h */

_Bool
example(int n)
{
	_Bool b;

	b = !0;		/* expect: constant in conditional context *//* expect: 239 */
	b = !1;		/* expect: constant in conditional context *//* expect: 239 */
	b = !(n > 1);

	return b;
}
