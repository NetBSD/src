/*	$NetBSD: msg_231.c,v 1.3 2021/04/09 20:12:01 rillig Exp $	*/
# 3 "msg_231.c"

// Test for message: argument '%s' unused in function '%s' [231]

void
example(int param)		/* expect: 231 */
{
	int local;		/* expect: 192 */
}
