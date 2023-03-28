/*	$NetBSD: msg_315.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_315.c"

// Test for message: GCC style struct or union member name in initializer [315]

/* lint1-flags: -Sw -X 351 */

struct point {
	int x, y;
} p = {
	/* expect+1: warning: GCC style struct or union member name in initializer [315] */
	x: 3,
	/* expect+1: warning: GCC style struct or union member name in initializer [315] */
	y: 4,
};
