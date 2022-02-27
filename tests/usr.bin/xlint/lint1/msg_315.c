/*	$NetBSD: msg_315.c,v 1.3 2022/02/27 12:00:27 rillig Exp $	*/
# 3 "msg_315.c"

// Test for message: GCC style struct or union member name in initializer [315]

/* lint1-flags: -Sw */

struct point {
	int x, y;
} p = {
	/* expect+1: warning: GCC style struct or union member name in initializer [315] */
	x: 3,
	/* expect+1: warning: GCC style struct or union member name in initializer [315] */
	y: 4,
};
