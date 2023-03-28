/*	$NetBSD: msg_083.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_083.c"

// Test for message: storage class after type is obsolescent [83]

/* lint1-extra-flags: -X 351 */

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
