/*	$NetBSD: msg_083.c,v 1.8 2024/01/28 08:17:27 rillig Exp $	*/
# 3 "msg_083.c"

// Test for message: storage class after type is obsolescent [83]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+2: warning: storage class after type is obsolescent [83] */
	/* expect+1: warning: 'x' unused in function 'example' [192] */
	int register x;
}

struct {
	int member;
} typedef s;
/* expect-1: warning: storage class after type is obsolescent [83] */
