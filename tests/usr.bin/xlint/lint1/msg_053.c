/*	$NetBSD: msg_053.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_053.c"

// Test for message: declared argument '%s' is missing [53]

/* lint1-extra-flags: -X 351 */

/* expect+2: error: old-style declaration; add 'int' [1] */
oldstyle(argument)
	int argument;
	/* expect+1: error: declared argument 'extra_argument' is missing [53] */
	int extra_argument;
{
	return argument;
}
