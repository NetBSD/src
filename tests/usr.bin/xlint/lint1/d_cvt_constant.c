/*	$NetBSD: d_cvt_constant.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_cvt_constant.c"

/* the second assignment assumes failed before */
int
main(void) {
    double x = 1;
    int foo = 0;
    if (foo)
	x = 1;
}
