/*	$NetBSD: msg_052.c,v 1.4 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_052.c"

// Test for message: cannot initialize parameter '%s' [52]

int
definition(i)
	/* expect+1: error: cannot initialize parameter 'i' [52] */
	int i = 3;
{
	return i;
}
