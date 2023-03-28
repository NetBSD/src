/*	$NetBSD: d_c9x_array_init.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c9x_array_init.c"

/* lint1-extra-flags: -X 351 */

/* GCC-specific array range initializers */
int foo[256] = {
	[2] = 1,
	[3] = 2,
	[4 ... 5] = 3
};
