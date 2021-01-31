/*	$NetBSD: d_c99_union_cast.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_c99_union_cast.c"

/* union cast */

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
	struct bar *a;

	((union foo)a).a;
}
