/*	$NetBSD: msg_192.c,v 1.6 2023/07/09 11:18:55 rillig Exp $	*/
# 3 "msg_192.c"

// Test for message: '%s' unused in function '%s' [192]

/* lint1-extra-flags: -X 351 */

void
/* expect+1: warning: parameter 'param' unused in function 'example' [231] */
example(int param)
{
	/* expect+1: warning: 'local' unused in function 'example' [192] */
	int local;
}
