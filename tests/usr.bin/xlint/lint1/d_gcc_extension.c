/*	$NetBSD: d_gcc_extension.c,v 1.6 2021/05/25 19:22:18 rillig Exp $	*/
# 3 "d_gcc_extension.c"

/*
 * Test that the GCC '__extension__' and '__typeof' are recognized.
 */

_Bool dbl_isinf(double);

/* extension */
void
a(void)
{
	double __logbw = 1;
	if (__extension__(({
		__typeof((__logbw)) x_ = (__logbw);
		!dbl_isinf((x_));
	})))
		__logbw = 1;
}
