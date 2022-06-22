/*	$NetBSD: msg_150.c,v 1.5 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_150.c"

// Test for message: argument mismatch: %d %s passed, %d expected [150]

int
add2(int, int);

int
example(void)
{
	/* expect+1: error: argument mismatch: 4 arguments passed, 2 expected [150] */
	return add2(2, 3, 5, 7);
}
