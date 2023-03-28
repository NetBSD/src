/*	$NetBSD: d_c99_union_init1.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_union_init1.c"

/* lint1-extra-flags: -X 351 */

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
