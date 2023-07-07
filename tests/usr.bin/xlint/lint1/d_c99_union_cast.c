/*	$NetBSD: d_c99_union_cast.c,v 1.8 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_c99_union_cast.c"

/* C99 does not define union cast, it is a GCC extension. */

/* lint1-flags: -Sw -X 351 */

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
