/*	$NetBSD: msg_223.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_223.c"

// Test for message: end-of-loop code not reached [223]

void
example(int n)
{
	/* expect+1: warning: end-of-loop code not reached [223] */
	for (int i = 0; i < n; i++)
		break;
}
