/*	$NetBSD: d_c99_flex_array_packed.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_flex_array_packed.c"

/* Allow packed c99 flexible arrays */
struct {
	int x;
	char y[0];
} __packed foo;
