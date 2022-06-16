/*	$NetBSD: msg_192.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_192.c"

// Test for message: '%s' unused in function '%s' [192]

/* ARGSUSED */
void
example(int param)
{
	/* expect+1: warning: 'local' unused in function 'example' [192] */
	int local;
}
