/*	$NetBSD: msg_125.c,v 1.6 2023/08/26 10:43:53 rillig Exp $	*/
# 3 "msg_125.c"

// Test for message: pointers to functions can only be compared for equality [125]

/* lint1-extra-flags: -s -X 351 */

typedef void (*action)(void);

int
less(action a, action b)
{
	/* expect+1: warning: pointers to functions can only be compared for equality [125] */
	return a < b;
}
