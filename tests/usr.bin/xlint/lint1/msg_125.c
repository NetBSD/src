/*	$NetBSD: msg_125.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_125.c"

// Test for message: ANSI C forbids ordered comparisons of pointers to functions [125]

/* lint1-extra-flags: -s -X 351 */

typedef void (*action)(void);

int
less(action a, action b)
{
	/* expect+1: warning: ANSI C forbids ordered comparisons of pointers to functions [125] */
	return a < b;
}
