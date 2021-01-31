/*	$NetBSD: d_c99_union_init1.c,v 1.3 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_union_init1.c"

/* C99 union initialization */
union {
	int i;
	char *s;
} c[] = {
	{ i: 1 },
	{ s: "foo" }
};
