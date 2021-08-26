/*	$NetBSD: msg_149.c,v 1.3 2021/08/26 19:23:25 rillig Exp $	*/
# 3 "msg_149.c"

// Test for message: illegal function (type %s) [149]

void
example(int i)
{
	i++;
	/* expect+1: error: illegal function (type int) [149] */
	i(3);
}
