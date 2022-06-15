/*	$NetBSD: msg_083.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_083.c"

// Test for message: storage class after type is obsolescent [83]

void
example(void)
{
	/* expect+1: warning: storage class after type is obsolescent [83] */
	int register x;
}

struct {
	int member;
} typedef s;
/* expect-1: warning: storage class after type is obsolescent [83] */
