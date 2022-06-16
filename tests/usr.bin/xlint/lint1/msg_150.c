/*	$NetBSD: msg_150.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_150.c"

// Test for message: argument mismatch: %d arg%s passed, %d expected [150]

int
add2(int, int);

int
example(void)
{
	/* expect+1: error: argument mismatch: 4 args passed, 2 expected [150] */
	return add2(2, 3, 5, 7);
}
