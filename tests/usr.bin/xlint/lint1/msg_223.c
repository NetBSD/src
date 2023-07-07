/*	$NetBSD: msg_223.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_223.c"

// Test for message: end-of-loop code not reached [223]

/* lint1-extra-flags: -X 351 */

void
example(int n)
{
	/* expect+1: warning: end-of-loop code not reached [223] */
	for (int i = 0; i < n; i++)
		break;
}
