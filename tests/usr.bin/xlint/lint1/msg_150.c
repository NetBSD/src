/*	$NetBSD: msg_150.c,v 1.7 2023/08/02 18:57:54 rillig Exp $	*/
# 3 "msg_150.c"

// Test for message: argument mismatch: %d %s passed, %d expected [150]

/* lint1-extra-flags: -X 351 */

int add2(int, int);

int
example(void)
{
	/* expect+1: error: argument mismatch: 0 arguments passed, 2 expected [150] */
	int a = add2();
	/* expect+1: error: argument mismatch: 1 argument passed, 2 expected [150] */
	int b = add2(1);
	/* expect+1: error: argument mismatch: 4 arguments passed, 2 expected [150] */
	int c = add2(2, 3, 5, 7);

	return a + b + c;
}
