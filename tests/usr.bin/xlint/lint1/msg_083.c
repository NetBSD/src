/*	$NetBSD: msg_083.c,v 1.4 2021/07/10 18:34:03 rillig Exp $	*/
# 3 "msg_083.c"

// Test for message: storage class after type is obsolescent [83]

void
example(void)
{
	int register x;		/* expect: 83 */
}

struct {
	int member;
} typedef s;
/* expect-1: warning: storage class after type is obsolescent [83] */
