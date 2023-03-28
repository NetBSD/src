/*	$NetBSD: d_c99_anon_union.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_anon_union.c"

/* struct with only anonymous members */

/* lint1-extra-flags: -X 351 */

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
