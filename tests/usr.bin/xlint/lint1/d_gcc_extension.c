/*	$NetBSD: d_gcc_extension.c,v 1.7 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_gcc_extension.c"

/*
 * Test that the GCC '__extension__' and '__typeof' are recognized.
 */

/* lint1-extra-flags: -X 351 */

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
