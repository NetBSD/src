/*	$NetBSD: msg_192.c,v 1.2 2021/01/30 17:56:29 rillig Exp $	*/
# 3 "msg_192.c"

// Test for message: %s unused in function %s [192]

void
example(int param)		/* expect: 231 */
{
	int local;		/* expect: 192 */
}
