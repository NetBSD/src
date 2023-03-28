/*	$NetBSD: d_c99_union_init3.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_union_init3.c"

/* lint1-extra-flags: -X 351 */

/* C99 struct initialization */
struct {
	int i[10];
	char *s;
} c[] = {
	{
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
		"foo"
	},
};
