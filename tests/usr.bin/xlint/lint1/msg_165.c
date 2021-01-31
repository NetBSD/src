/*	$NetBSD: msg_165.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_165.c"

// Test for message: constant truncated by assignment [165]

void
example(void)
{
	unsigned char ch;

	ch = 0x1234;		/* expect: 165 */
}
