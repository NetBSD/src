/*	$NetBSD: msg_304.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_304.c"

/* Test for message: ANSI C forbids conversion of %s to %s, arg #%d [304] */

/* lint1-flags: -sw -X 351 */

void take_void_pointer(void *);
void take_function_pointer(void (*)(void));

void
caller(void *arg)
{
	/* expect+1: warning: ANSI C forbids conversion of function pointer to 'void *', arg #1 [304] */
	take_void_pointer(caller);

	/* expect+1: warning: ANSI C forbids conversion of 'void *' to function pointer, arg #1 [304] */
	take_function_pointer(arg);
}
