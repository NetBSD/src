/*	$NetBSD: msg_239.c,v 1.10 2025/04/10 20:16:30 rillig Exp $	*/
# 3 "msg_239.c"

// Test for message: constant operand to '!' [239]
// This message is not used.
// Its purpose is unclear, as a constant condition is not a bug by itself.
// See msg_382.c for a similar pattern that catches real bugs.

/* lint1-extra-flags: -h -X 351 */

_Bool
example(int n)
{
	_Bool b;

	/* was: warning: constant operand to '!' [239] */
	b = !0;
	/* was: warning: constant operand to '!' [239] */
	b = !1;
	b = !(n > 1);

	return b;
}
