/*	$NetBSD: msg_213.c,v 1.6 2023/07/09 11:18:55 rillig Exp $	*/
# 3 "msg_213.c"

// Test for message: void function '%s' cannot return value [213]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: parameter 'x' unused in function 'example' [231] */
void
example(int x)
{
	/* expect+1: error: void function 'example' cannot return value [213] */
	return x;
}
