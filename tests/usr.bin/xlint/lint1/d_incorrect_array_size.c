/*	$NetBSD: d_incorrect_array_size.c,v 1.4 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_incorrect_array_size.c"

struct foo {
	/* expect+1: error: negative array dimension (-1) [20] */
	int a[-1];
};
