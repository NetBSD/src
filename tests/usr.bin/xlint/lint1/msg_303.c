/*	$NetBSD: msg_303.c,v 1.3 2021/08/16 18:51:58 rillig Exp $	*/
# 3 "msg_303.c"

/* Test for message: ANSI C forbids conversion of %s to %s [303] */

/* lint1-flags: -sw */

void take_void_pointer(void *);

void *
to_void_pointer(void)
{
	/* expect+1: warning: ANSI C forbids conversion of function pointer to 'void *' [303] */
	return to_void_pointer;
}

void (*to_function_pointer(void *arg))(void)
{
	/* expect+1: warning: ANSI C forbids conversion of 'void *' to function pointer [303] */
	return arg;
}
