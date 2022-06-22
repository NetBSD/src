/*	$NetBSD: msg_091.c,v 1.5 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_091.c"

/* Test for message: declaration of '%s' hides parameter [91] */

/* lint1-flags: -htw */

add(a, b)
	int a, b;
{
	/* expect+1: warning: declaration of 'a' hides parameter [91] */
	int a;

	/* expect+1: warning: 'a' may be used before set [158] */
	return a + b;
}
