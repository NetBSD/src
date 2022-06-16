/*	$NetBSD: msg_125.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_125.c"

// Test for message: ANSI C forbids ordered comparisons of pointers to functions [125]

/* lint1-extra-flags: -s */

typedef void (*action)(void);

int
less(action a, action b)
{
	/* expect+1: warning: ANSI C forbids ordered comparisons of pointers to functions [125] */
	return a < b;
}
