/*	$NetBSD: msg_305.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_305.c"

/* Test for message: ANSI C forbids conversion of %s to %s, op %s [305] */

/* lint1-flags: -sw -X 351 */

void take_void_pointer(void *);

typedef void (*function)(void);

void
caller(void **void_pointer, function *function_pointer)
{
	/* expect+1: warning: ANSI C forbids conversion of function pointer to 'void *', op = [305] */
	*void_pointer = *function_pointer;

	/* expect+1: warning: ANSI C forbids conversion of 'void *' to function pointer, op = [305] */
	*function_pointer = *void_pointer;
}
