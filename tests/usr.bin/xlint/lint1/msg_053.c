/*	$NetBSD: msg_053.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_053.c"

// Test for message: declared argument %s is missing [53]

oldstyle(argument)
	int argument;
	/* expect+1: error: declared argument extra_argument is missing [53] */
	int extra_argument;
{
	return argument;
}
