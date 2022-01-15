/*	$NetBSD: d_cvt_constant.c,v 1.6 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_cvt_constant.c"

/* the second assignment assumes failed before */
int
main(void)
{
	/* expect+1: warning: 'x' set but not used in function 'main' [191] */
	double x = 1;
	int foo = 0;
	if (foo)
		x = 1;
}
