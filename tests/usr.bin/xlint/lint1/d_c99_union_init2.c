/*	$NetBSD: d_c99_union_init2.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_union_init2.c"

/* lint1-extra-flags: -X 351 */

/* C99 union initialization */
union {
	int i[10];
	short s;
} c[] = {
	{ s: 2 },
	{ i: { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } },
};
