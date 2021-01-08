/*	$NetBSD: msg_150.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_150.c"

// Test for message: argument mismatch: %d arg%s passed, %d expected [150]

int
add2(int, int);

int
example(void)
{
	return add2(2, 3, 5, 7);
}
