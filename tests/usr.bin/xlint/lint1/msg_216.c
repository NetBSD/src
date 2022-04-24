/*	$NetBSD: msg_216.c,v 1.3 2022/04/24 20:08:23 rillig Exp $	*/
# 3 "msg_216.c"

// Test for message: function %s has return (e); and return; [216]

/* expect+2: error: old style declaration; add 'int' [1] */
random(int n)
{
	if (n < 0)
		return -3;
	if (n < 2)
		return;
}				/* expect: 216 */
