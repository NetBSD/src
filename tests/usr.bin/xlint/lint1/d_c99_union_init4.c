/*	$NetBSD: d_c99_union_init4.c,v 1.4 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_union_init4.c"

/* lint1-extra-flags: -X 351 */

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
