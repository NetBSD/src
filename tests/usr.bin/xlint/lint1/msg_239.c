/*	$NetBSD: msg_239.c,v 1.3 2021/04/02 22:38:42 rillig Exp $	*/
# 3 "msg_239.c"

// Test for message: constant argument to NOT [239]

/* lint1-extra-flags: -h */

_Bool
example(int n)
{
	_Bool b;

	b = !0;		/* expect: constant in conditional context, 239 */
	b = !1;		/* expect: constant in conditional context, 239 */
	b = !(n > 1);

	return b;
}
