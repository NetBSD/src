/*	$NetBSD: msg_192.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_192.c"

// Test for message: '%s' unused in function '%s' [192]

/* lint1-extra-flags: -X 351 */

void
/* expect+1: warning: argument 'param' unused in function 'example' [231] */
example(int param)
{
	/* expect+1: warning: 'local' unused in function 'example' [192] */
	int local;
}
