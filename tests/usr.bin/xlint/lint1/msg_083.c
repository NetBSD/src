/*	$NetBSD: msg_083.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_083.c"

// Test for message: storage class after type is obsolescent [83]

void
example(void)
{
	int register x;		/* expect: 83 */
}
