/*	$NetBSD: msg_231.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_231.c"

// Test for message: argument '%s' unused in function '%s' [231]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: argument 'param' unused in function 'example' [231] */
void
example(int param)
{
	/* expect+1: warning: 'local' unused in function 'example' [192] */
	int local;
}
