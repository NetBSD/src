/*	$NetBSD: d_cvt_constant.c,v 1.5 2021/02/21 09:17:55 rillig Exp $	*/
# 3 "d_cvt_constant.c"

/* the second assignment assumes failed before */
int
main(void)
{
	double x = 1;		/* expect: 191 */
	int foo = 0;
	if (foo)
		x = 1;
}
