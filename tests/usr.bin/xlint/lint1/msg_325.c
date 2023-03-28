/*	$NetBSD: msg_325.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_325.c"

/* Test for message: variable declaration in for loop [325] */

/* lint1-flags: -sw -X 351 */

int printf(const char *, ...);

void example(void)
{
	/* expect+1: error: variable declaration in for loop [325] */
	for (int i = 0; i < 10; i++)
		printf("%d\n", i);
}
