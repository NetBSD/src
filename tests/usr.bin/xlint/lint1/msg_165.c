/*	$NetBSD: msg_165.c,v 1.7 2024/01/28 08:17:27 rillig Exp $	*/
# 3 "msg_165.c"

// Test for message: constant truncated by assignment [165]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	unsigned char ch;

	/* expect+2: warning: constant truncated by assignment [165] */
	/* expect+1: warning: 'ch' set but not used in function 'example' [191] */
	ch = 0x1234;
}
