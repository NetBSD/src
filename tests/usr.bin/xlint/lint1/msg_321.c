/*	$NetBSD: msg_321.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_321.c"

/* Test for message: array initializer with designators is a C99 feature [321] */

/* lint1-flags: -sw -X 351 */

int vector[3] = {
	/* expect+1: warning: array initializer with designators is a C99 feature [321] */
	[0] = 3,
	/* expect+1: warning: array initializer with designators is a C99 feature [321] */
	[1] = 5,
};
