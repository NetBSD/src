/*	$NetBSD: msg_313.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_313.c"

/* Test for message: struct or union member name in initializer is a C99 feature [313] */

/* lint1-flags: -sw -X 351 */

struct point {
	int x, y;
} p = {
	/* expect+1: warning: struct or union member name in initializer is a C99 feature [313] */
	.x = 3,
	/* expect+1: warning: struct or union member name in initializer is a C99 feature [313] */
	.y = 4,
};
