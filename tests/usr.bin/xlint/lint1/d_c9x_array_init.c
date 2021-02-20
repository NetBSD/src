/*	$NetBSD: d_c9x_array_init.c,v 1.3 2021/02/20 22:31:20 rillig Exp $	*/
# 3 "d_c9x_array_init.c"

/* GCC-specific array range initializers */
int foo[256] = {
	[2] = 1,
	[3] = 2,
	[4 ... 5] = 3
};
