/*	$NetBSD: msg_216.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_216.c"

// Test for message: function '%s' has 'return expr' and 'return' [216]

/* lint1-extra-flags: -X 351 */

/* expect+2: error: old-style declaration; add 'int' [1] */
random(int n)
{
	if (n < 0)
		return -3;
	if (n < 2)
		return;
}
/* expect-1: warning: function 'random' has 'return expr' and 'return' [216] */
