/*	$NetBSD: msg_231.c,v 1.6 2023/07/09 11:18:55 rillig Exp $	*/
# 3 "msg_231.c"

// Test for message: parameter '%s' unused in function '%s' [231]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: parameter 'param' unused in function 'example' [231] */
void
example(int param)
{
	/* expect+1: warning: 'local' unused in function 'example' [192] */
	int local;
}
