/*	$NetBSD: msg_192.c,v 1.3 2021/04/09 20:12:01 rillig Exp $	*/
# 3 "msg_192.c"

// Test for message: '%s' unused in function '%s' [192]

void
example(int param)		/* expect: 231 */
{
	int local;		/* expect: 192 */
}
