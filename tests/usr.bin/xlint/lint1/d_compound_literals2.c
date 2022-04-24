/*	$NetBSD: d_compound_literals2.c,v 1.4 2022/04/24 20:08:23 rillig Exp $	*/
# 3 "d_compound_literals2.c"

/* compound literals */

struct p {
	short a, b, c, d;
} zz = {
	1, 2, 3, 4
};

struct p *
bar(int i)
{
	static struct p q[10];
	return &q[i];
}

void foo(void)
{
	*bar(1) = (struct p){ 1, 2, 3, 4 };
}
