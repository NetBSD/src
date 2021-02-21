/*	$NetBSD: d_cvt_constant.c,v 1.4 2021/02/21 09:07:58 rillig Exp $	*/
# 3 "d_cvt_constant.c"

/* the second assignment assumes failed before */
int
main(void)
{
	double x = 1;		/* expect: 191 */
	int foo = 0;
	if (foo)
		x = 1;
}				/* expect: 217 */
// FIXME: Since C99, main may fall off the bottom without returning a value.
//  Therefore there should be no warning 217.
