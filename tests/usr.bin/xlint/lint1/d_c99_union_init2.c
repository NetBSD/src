/*	$NetBSD: d_c99_union_init2.c,v 1.3 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_union_init2.c"

/* C99 union initialization */
union {
	int i[10];
	short s;
} c[] = {
	{ s: 2 },
	{ i: { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },
};
