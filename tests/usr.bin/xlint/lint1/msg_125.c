/*	$NetBSD: msg_125.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_125.c"

// Test for message: ANSI C forbids ordered comparisons of pointers to functions [125]

/* lint1-extra-flags: -s */

typedef void (*action)(void);

int
less(action a, action b)
{
	return a < b;		/* expect: 125 */
}
