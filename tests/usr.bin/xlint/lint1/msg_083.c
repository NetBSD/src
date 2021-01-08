/*	$NetBSD: msg_083.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_083.c"

// Test for message: storage class after type is obsolescent [83]

void
example(void)
{
	int register x;
}
