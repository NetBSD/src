/*	$NetBSD: msg_274.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_274.c"

/* Test for message: ANSI C forbids comparison of %s with %s [274] */

/* lint1-flags: -sw -X 351 */

void
example(void (*function_pointer)(void), void *void_pointer)
{

	/* Comparing a function pointer with a null pointer is OK. */
	if (function_pointer == (void *)0)
		return;

	/* Comparing a function pointer with a null pointer is OK. */
	if (function_pointer == (const void *)0)
		return;

	/* expect+1: warning: ANSI C forbids comparison of function pointer with 'void *' [274] */
	if (function_pointer == void_pointer)
		return;
}
