/*	$NetBSD: msg_193.c,v 1.2 2021/01/30 17:56:29 rillig Exp $	*/
# 3 "msg_193.c"

// Test for message: statement not reached [193]

void example(void)
{
	return;
	return;			/* expect: 193 */
}
