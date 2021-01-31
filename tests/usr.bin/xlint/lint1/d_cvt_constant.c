/*	$NetBSD: d_cvt_constant.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_cvt_constant.c"

/* the second assignment assumes failed before */
int
main(void)
{
	double x = 1;
	int foo = 0;
	if (foo)
		x = 1;
}
