/*	$NetBSD: msg_321.c,v 1.4 2022/02/27 12:00:27 rillig Exp $	*/
# 3 "msg_321.c"

/* Test for message: array initializer with designators is a C99 feature [321] */

/* lint1-flags: -sw */

int vector[3] = {
	/* expect+1: warning: array initializer with designators is a C99 feature [321] */
	[0] = 3,
	/* expect+1: warning: array initializer with designators is a C99 feature [321] */
	[1] = 5,
};
