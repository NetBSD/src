/*	$NetBSD: d_compound_literals1.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_compound_literals1.c"

/* compound literals */

struct p {
	short a, b, c, d;
};

foo()
{
	struct p me = (struct p) {1, 2, 3, 4};
	me.a = me.b;
}
