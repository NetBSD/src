/*	$NetBSD: d_c99_union_cast.c,v 1.7 2021/08/03 20:57:06 rillig Exp $	*/
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
	/* expect+1: error: union cast is a GCC extension [328] */
	a = ((union foo)a).a;
	/* expect+1: error: union cast is a GCC extension [328] */
	a = ((union foo)"string");
	a->a++;
}
