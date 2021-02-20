/*	$NetBSD: d_c99_union_init1.c,v 1.4 2021/02/20 22:31:20 rillig Exp $	*/
# 3 "d_c99_union_init1.c"

/* GCC-style and C99-style union initialization */
union {
	int i;
	char *s;
} c[] = {
	{ i: 1 },		/* GCC-style */
	{ s: "foo" },		/* GCC-style */
	{ .i = 1 },		/* C99-style */
	{ .s = "foo" }		/* C99-style */
};
