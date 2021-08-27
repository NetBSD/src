/*	$NetBSD: msg_052.c,v 1.3 2021/08/27 20:16:50 rillig Exp $	*/
# 3 "msg_052.c"

// Test for message: cannot initialize parameter: %s [52]

int
definition(i)
	/* expect+1: error: cannot initialize parameter: i [52] */
	int i = 3;
{
	return i;
}
