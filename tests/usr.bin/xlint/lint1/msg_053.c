/*	$NetBSD: msg_053.c,v 1.5 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_053.c"

// Test for message: declared argument '%s' is missing [53]

/* expect+2: error: old style declaration; add 'int' [1] */
oldstyle(argument)
	int argument;
	/* expect+1: error: declared argument 'extra_argument' is missing [53] */
	int extra_argument;
{
	return argument;
}
