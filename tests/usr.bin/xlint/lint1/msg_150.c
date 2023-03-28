/*	$NetBSD: msg_150.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_150.c"

// Test for message: argument mismatch: %d %s passed, %d expected [150]

/* lint1-extra-flags: -X 351 */

int
add2(int, int);

int
example(void)
{
	/* expect+1: error: argument mismatch: 4 arguments passed, 2 expected [150] */
	return add2(2, 3, 5, 7);
}
