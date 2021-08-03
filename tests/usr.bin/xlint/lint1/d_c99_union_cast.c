/*	$NetBSD: d_c99_union_cast.c,v 1.5 2021/08/03 20:34:23 rillig Exp $	*/
# 3 "d_c99_union_cast.c"

/* C99 does not define union cast, it is a GCC extension. */

struct bar {
	int a;
	int b;
};

union foo {
	struct bar *a;
	int b;
};

void
foo(void)
{
	struct bar *a;		/* expect: 192 */

	((union foo)a).a;
}
