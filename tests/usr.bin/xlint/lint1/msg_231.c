/*	$NetBSD: msg_231.c,v 1.2 2021/01/30 17:56:29 rillig Exp $	*/
# 3 "msg_231.c"

// Test for message: argument %s unused in function %s [231]

void
example(int param)		/* expect: 231 */
{
	int local;		/* expect: 192 */
}
