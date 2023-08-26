/*	$NetBSD: msg_274.c,v 1.5 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_274.c"

/* Test for message: C90 or later forbid comparison of %s with %s [274] */

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

	/* expect+1: warning: C90 or later forbid comparison of function pointer with 'void *' [274] */
	if (function_pointer == void_pointer)
		return;
}
