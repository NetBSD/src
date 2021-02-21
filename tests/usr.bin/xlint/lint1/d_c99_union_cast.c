/*	$NetBSD: d_c99_union_cast.c,v 1.4 2021/02/21 09:07:58 rillig Exp $	*/
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
	struct bar *a;		/* expect: 192 */

	((union foo)a).a;
}
