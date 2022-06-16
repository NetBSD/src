/*	$NetBSD: msg_231.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_231.c"

// Test for message: argument '%s' unused in function '%s' [231]

/* expect+2: warning: argument 'param' unused in function 'example' [231] */
void
example(int param)
{
	/* expect+1: warning: 'local' unused in function 'example' [192] */
	int local;
}
