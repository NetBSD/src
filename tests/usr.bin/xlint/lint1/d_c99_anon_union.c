/*	$NetBSD: d_c99_anon_union.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_c99_anon_union.c"

/* struct with only anonymous members */

struct foo {
	union {
		long loo;
		double doo;
	};
};

int
main(void)
{
	struct foo *f = 0;
	printf("%p\n", &f[1]);
	return 0;
}
