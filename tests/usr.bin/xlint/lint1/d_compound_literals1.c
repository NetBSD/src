/*	$NetBSD: d_compound_literals1.c,v 1.4 2022/04/24 20:08:23 rillig Exp $	*/
# 3 "d_compound_literals1.c"

/* compound literals */

struct p {
	short a, b, c, d;
};

void foo(void)
{
	struct p me = (struct p){ 1, 2, 3, 4 };
	me.a = me.b;
}
