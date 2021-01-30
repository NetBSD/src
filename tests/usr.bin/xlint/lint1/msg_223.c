/*	$NetBSD: msg_223.c,v 1.2 2021/01/30 17:02:58 rillig Exp $	*/
# 3 "msg_223.c"

// Test for message: end-of-loop code not reached [223]

void
example(int n)
{
	for (int i = 0; i < n; i++)	/* expect: 223 */
		break;
}
