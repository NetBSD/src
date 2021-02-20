/*	$NetBSD: d_c99_anon_union.c,v 1.4 2021/02/20 22:31:20 rillig Exp $	*/
# 3 "d_c99_anon_union.c"

/* struct with only anonymous members */

struct foo {
	union {
		long loo;
		double doo;
	};
};

int printf(const char *, ...);

int
main(void)
{
	struct foo *f = 0;
	printf("%p\n", &f[1]);
	return 0;
}
