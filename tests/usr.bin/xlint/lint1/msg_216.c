/*	$NetBSD: msg_216.c,v 1.2 2021/01/30 17:02:58 rillig Exp $	*/
# 3 "msg_216.c"

// Test for message: function %s has return (e); and return; [216]

/* implicit int */
random(int n)
{
	if (n < 0)
		return -3;
	if (n < 2)
		return;
}				/* expect: 216 */
