/*	$NetBSD: msg_325.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_325.c"

/* Test for message: variable declaration in for loop [325] */

/* lint1-flags: -sw */

int printf(const char *, ...);

void example(void)
{
	/* expect+1: error: variable declaration in for loop [325] */
	for (int i = 0; i < 10; i++)
		printf("%d\n", i);
}
