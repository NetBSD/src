/*	$NetBSD: d_compound_literals1.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_compound_literals1.c"

/* compound literals */

/* lint1-extra-flags: -X 351 */

struct p {
	short a, b, c, d;
};

void foo(void)
{
	struct p me = (struct p){ 1, 2, 3, 4 };
	me.a = me.b;
}
