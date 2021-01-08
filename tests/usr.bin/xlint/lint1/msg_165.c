/*	$NetBSD: msg_165.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_165.c"

// Test for message: constant truncated by assignment [165]

void
example(void)
{
	unsigned char ch;

	ch = 0x1234;
}
