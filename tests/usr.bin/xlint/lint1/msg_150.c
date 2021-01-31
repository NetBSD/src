/*	$NetBSD: msg_150.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_150.c"

// Test for message: argument mismatch: %d arg%s passed, %d expected [150]

int
add2(int, int);

int
example(void)
{
	return add2(2, 3, 5, 7);	/* expect: 150 */
}
