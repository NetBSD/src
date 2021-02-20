/*	$NetBSD: d_c99_union_init3.c,v 1.5 2021/02/20 22:31:20 rillig Exp $	*/
# 3 "d_c99_union_init3.c"

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
