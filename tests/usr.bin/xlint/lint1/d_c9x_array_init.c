/*	$NetBSD: d_c9x_array_init.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c9x_array_init.c"

/* C9X array initializers */
int foo[256] = {
	[2] = 1,
	[3] = 2,
	[4 ... 5] = 3
};
