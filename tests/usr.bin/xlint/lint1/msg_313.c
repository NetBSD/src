/*	$NetBSD: msg_313.c,v 1.4 2022/02/27 12:00:27 rillig Exp $	*/
# 3 "msg_313.c"

/* Test for message: struct or union member name in initializer is a C99 feature [313] */

/* lint1-flags: -sw */

struct point {
	int x, y;
} p = {
	/* expect+1: warning: struct or union member name in initializer is a C99 feature [313] */
	.x = 3,
	/* expect+1: warning: struct or union member name in initializer is a C99 feature [313] */
	.y = 4,
};
