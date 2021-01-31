/*	$NetBSD: d_c99_union_init4.c,v 1.3 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_c99_union_init4.c"

/* test .data.l[x] */
typedef struct {
	int type;
	union {
		char b[20];
		short s[10];
		long l[5];
	} data;
} foo;


foo bar = {
	.type = 3,
	.data.l[0] = 4
};
