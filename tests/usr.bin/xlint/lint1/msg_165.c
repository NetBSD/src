/*	$NetBSD: msg_165.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_165.c"

// Test for message: constant truncated by assignment [165]

void
example(void)
{
	unsigned char ch;

	/* expect+1: warning: constant truncated by assignment [165] */
	ch = 0x1234;
}
