/*	$NetBSD: d_compound_literals2.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_compound_literals2.c"

/* compound literals */

/* lint1-extra-flags: -X 351 */

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
