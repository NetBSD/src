/*	$NetBSD: msg_083.c,v 1.7 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_083.c"

// Test for message: storage class after type is obsolescent [83]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+2: warning: 'x' unused in function 'example' [192] */
	/* expect+1: warning: storage class after type is obsolescent [83] */
	int register x;
}

struct {
	int member;
} typedef s;
/* expect-1: warning: storage class after type is obsolescent [83] */
