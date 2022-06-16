/*	$NetBSD: msg_216.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_216.c"

// Test for message: function %s has return (e); and return; [216]

/* expect+2: error: old style declaration; add 'int' [1] */
random(int n)
{
	if (n < 0)
		return -3;
	if (n < 2)
		return;
}
/* expect-1: warning: function random has return (e); and return; [216] */
