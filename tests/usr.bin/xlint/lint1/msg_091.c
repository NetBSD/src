/*	$NetBSD: msg_091.c,v 1.3 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_091.c"

/* Test for message: declaration hides parameter: %s [91] */

/* lint1-flags: -htw */

add(a, b)
	int a, b;
{
	/* expect+1: warning: declaration hides parameter: a [91] */
	int a;

	/* expect+1: warning: a may be used before set [158] */
	return a + b;
}
