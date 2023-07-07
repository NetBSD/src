/*	$NetBSD: msg_165.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_165.c"

// Test for message: constant truncated by assignment [165]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	unsigned char ch;

	/* expect+2: warning: 'ch' set but not used in function 'example' [191] */
	/* expect+1: warning: constant truncated by assignment [165] */
	ch = 0x1234;
}
