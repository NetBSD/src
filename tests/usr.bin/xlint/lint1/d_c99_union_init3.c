/*	$NetBSD: d_c99_union_init3.c,v 1.4 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_c99_union_init3.c"

/* C99 union initialization */
struct {
	int i[10];
	char *s;
} c[] = {
	{{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, "foo" },
};
