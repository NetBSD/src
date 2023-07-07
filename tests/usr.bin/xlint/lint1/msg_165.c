/*	$NetBSD: msg_165.c,v 1.5 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_165.c"

// Test for message: constant truncated by assignment [165]

void
example(void)
{
	unsigned char ch;

	/* expect+2: warning: 'ch' set but not used in function 'example' [191] */
	/* expect+1: warning: constant truncated by assignment [165] */
	ch = 0x1234;
}
