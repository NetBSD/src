/*	$NetBSD: d_c99_union_cast.c,v 1.6 2021/08/03 20:46:10 rillig Exp $	*/
# 3 "d_c99_union_cast.c"

/* C99 does not define union cast, it is a GCC extension. */

/* lint1-flags: -Sw */

struct bar {
	int a;
	int b;
};

union foo {
	struct bar *a;
	int b;
};

void
foo(struct bar *a)
{
	/* TODO: warn about union casts in general */
	a = ((union foo)a).a;
	/* expect+1: error: type 'pointer to char' is not a member of 'union foo' [329] */
	a = ((union foo)"string");
	a->a++;
}
